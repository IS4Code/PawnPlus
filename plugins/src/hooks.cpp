#include "hooks.h"
#include "main.h"
#include "context.h"
#include "amxinfo.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/events.h"
#include "modules/threads.h"
#include "modules/amxutils.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

#include <cstring>
#include <unordered_map>

using namespace tasks;

extern void *pAMXFunctions;

extern int ExecLevel;

subhook_t amx_Init_h;
subhook_t amx_Exec_h;
subhook_t amx_GetAddr_h;
subhook_t amx_StrLen_h;
subhook_t amx_FindPublic_h;
subhook_t amx_Register_h;

bool hook_ref_args = false;

// Automatically destroys the AMX machine when the info instance is destroyed (i.e. the AMX becomes unused)
struct forked_amx_holder : public amx::extra
{
	forked_amx_holder(AMX *amx) : amx::extra(amx)
	{

	}

	virtual ~forked_amx_holder() override
	{
		delete[] _amx->base;
		delete _amx;
	}
};

// Holds the original code of the program (i.e. before relocation)
struct amx_code_info : public amx::extra
{
	std::unique_ptr<unsigned char[]> code;

	amx_code_info(AMX *amx) : amx::extra(amx)
	{
		auto amxhdr = (AMX_HEADER*)amx->base;
		code = std::make_unique<unsigned char[]>(amxhdr->size - amxhdr->cod);
		std::memcpy(code.get(), amx->base + amxhdr->cod, amxhdr->size - amxhdr->cod);
	}
};

struct forked_context : public amx::extra
{
	forked_context(AMX *amx) : amx::extra(amx)
	{

	}
};

int AMXAPI amx_InitOrig(AMX *amx, void *program)
{
	if(subhook_is_installed(amx_Init_h))
	{
		return reinterpret_cast<decltype(&amx_Init)>(subhook_get_trampoline(amx_Init_h))(amx, program);
	} else {
		return amx_Init(amx, program);
	}
}

int AMXAPI amx_ExecOrig(AMX *amx, cell *retval, int index)
{
	if(subhook_is_installed(amx_Exec_h))
	{
		return reinterpret_cast<decltype(&amx_Exec)>(subhook_get_trampoline(amx_Exec_h))(amx, retval, index);
	}else{
		return amx_Exec(amx, retval, index);
	}
}

int AMXAPI amx_FindPublicOrig(AMX *amx, const char *funcname, int *index)
{
	if(subhook_is_installed(amx_FindPublic_h))
	{
		int ret = reinterpret_cast<decltype(&amx_FindPublicOrig)>(subhook_get_trampoline(amx_FindPublic_h))(amx, funcname, index);
		return ret;
	}else{
		return amx_FindPublic(amx, funcname, index);
	}
}

int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, amx::reset *reset, bool forked)
{
	if(index <= -3)
	{
		auto name = events::invoke_callback(-3 - index, amx, retval);
		if(name != nullptr)
		{
			int err = amx_FindPublicOrig(amx, name, &index);
			if(err) return err;
		}
	}

	Threads::PauseThreads(amx);
	Threads::JoinThreads(amx);

	amx::reset *old = nullptr;
	if(restore && amx::has_context(amx))
	{
		old = new amx::reset(amx, true);
	}

	amx::push(amx, index);
	if(reset != nullptr)
	{
		reset->restore();
	}

	if(forked)
	{
		amx::object owner;
		auto &ctx = amx::get_context(amx, owner);
		ctx.get_extra<forked_context>();
	}
	
	int ret;
	while(true)
	{
		ret = amx_ExecOrig(amx, retval, index);
		if(ret == AMX_ERR_SLEEP)
		{
			index = AMX_EXEC_CONT;

			amx::object owner;
			auto &ctx = amx::get_context(amx, owner);
			tasks::extra info = tasks::get_extra(amx, owner);

			switch(amx->pri & SleepReturnTypeMask)
			{
				case SleepReturnAwait:
				{
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(auto task = info.awaited_task.lock())
					{
						if(retval != nullptr) *retval = info.result;
						task->register_reset(amx::reset(amx, true));
						info.awaited_task = {};
					}
				}
				break;
				case SleepReturnWaitTicks:
				{
					cell ticks = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					task::register_tick(ticks, amx::reset(amx, true));
				}
				break;
				case SleepReturnWaitMs:
				{
					cell interval = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					task::register_timer(interval, amx::reset(amx, true));
				}
				break;
				case SleepReturnWaitInf:
				{
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
				}
				break;
				case SleepReturnDetach:
				{
					auto flags = static_cast<Threads::SyncFlags>(SleepReturnValueMask & amx->pri);
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					Threads::DetachThread(amx, flags);
				}
				break;
				case SleepReturnAttach:
				{
					amx->pri = 0;
					logwarn(amx, "[PP] thread_attach was called in a non-threaded code.");
					continue;
				}
				break;
				case SleepReturnSync:
				{
					amx->pri = 0;
					logwarn(amx, "[PP] thread_sync was called in a non-threaded code.");
					continue;
				}
				break;
				case SleepReturnFork:
				{
					cell result_addr = owner->get_extra<fork_info_extra>().result_address;

					cell flags = SleepReturnValueMask & amx->pri;

					auto amxhdr = (AMX_HEADER*)amx->base;
					if(flags & SleepReturnForkFlagsClone)
					{
						AMX *amx_fork = new AMX();
						auto lock = amx::clone_lock(amx, amx_fork);

						auto code = owner->get_extra<amx_code_info>().code.get();

						amx_fork->base = new unsigned char[amxhdr->stp];
						std::memcpy(amx_fork->base, amx->base, amxhdr->cod); // copy header
						std::memcpy(amx_fork->base + amxhdr->cod, code, amxhdr->size - amxhdr->cod); // copy original code
						int initret = amx_Init(amx_fork, amx_fork->base);
						if(initret != AMX_ERR_NONE)
						{
							delete[] amx_fork->base;
							delete amx_fork;
							amx::unload(amx_fork);
							logwarn(amx, "[PP] amx_fork: couldn't create the fork (error %d).", initret);
							continue;
						}
						if(flags & SleepReturnForkFlagsCopyData)
						{
							std::memcpy(amx_fork->base + amxhdr->dat, amx->base + amxhdr->dat, amxhdr->hea - amxhdr->dat); // copy the data
						}

						amx::reset reset(amx, false);
						reset.amx = lock;
						reset.restore_no_context(); // copy the heap/stack

						amx_fork->pri = 1;
						amx_fork->error = AMX_ERR_NONE;
						amx_fork->callback = amx_Callback;
						amx_fork->flags |= AMX_FLAG_NTVREG;

						lock->get_extra<forked_amx_holder>();
						cell *result;
						amx_GetAddr(amx, result_addr, &result);
						amx_ExecContext(amx_fork, result, AMX_EXEC_CONT, false, nullptr, true);

						if(amx_fork->error == AMX_ERR_SLEEP && (amx_fork->pri & SleepReturnTypeMask) == SleepReturnForkCommit)
						{
							amx::reset reset(amx_fork, false);
							reset.amx = owner;
							reset.restore_no_context();
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx->base + amxhdr->dat, amx_fork->base + amxhdr->dat, amxhdr->hea - amxhdr->dat);
							}
						}

						amx::unload(amx_fork);
					}else{
						amx::reset reset(amx, true);

						unsigned char *orig_data;
						if(flags & SleepReturnForkFlagsCopyData)
						{
							orig_data = new unsigned char[amxhdr->hea - amxhdr->dat];
							std::memcpy(orig_data, amx->base + amxhdr->dat, amxhdr->hea - amxhdr->dat); // backup the data
						}

						amx->pri = 1;
						amx->error = AMX_ERR_NONE;

						cell *result;
						amx_GetAddr(amx, result_addr, &result);
						amx_ExecContext(amx, result, AMX_EXEC_CONT, false, nullptr, true);

						if(amx->error == AMX_ERR_SLEEP && (amx->pri & SleepReturnTypeMask) == SleepReturnForkCommit)
						{

						}else{
							reset.restore();
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx->base + amxhdr->dat, orig_data, amxhdr->hea - amxhdr->dat);
							}
						}
						if(flags & SleepReturnForkFlagsCopyData)
						{
							delete[] orig_data;
						}
					}

					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;

					continue;
				}
				break;
				case SleepReturnForkCommit:
				{
					if(!forked)
					{
						amx->pri = 0;
						if(ctx.has_extra<forked_context>())
						{
							logwarn(amx, "[PP] amx_commit: the original context is no longer present.");
						}else{
							logwarn(amx, "[PP] amx_commit was called from a non-forked code.");
						}
						continue;
					}
				}
				break;
				case SleepReturnForkEnd:
				{
					if(!forked && !ctx.has_extra<forked_context>())
					{
						amx->pri = 0;
						logwarn(amx, "[PP] amx_fork_end was called from a non-forked code.");
						continue;
					}else{
						if(retval != nullptr) *retval = info.result;
						amx->error = ret = AMX_ERR_NONE;
					}
				}
				break;
			}
		}
		break;
	}
	amx::pop(amx);
	if(old != nullptr)
	{
		old->restore();
		delete old;
	}
	Threads::ResumeThreads(amx);
	return ret;
}

namespace Hooks
{
	int AMXAPI amx_Init(AMX *amx, void *program)
	{
		amx->base = (unsigned char*)program;
		amx::load_lock(amx)->get_extra<amx_code_info>();
		amx->base = nullptr;
		int ret = amx_InitOrig(amx, program);
		if(ret != AMX_ERR_NONE)
		{
			amx::unload(amx);
		}
		return ret;
	}

	int AMXAPI amx_Exec(AMX *amx, cell *retval, int index)
	{
		return amx_ExecContext(amx, retval, index, false, nullptr);
	}

	int AMXAPI amx_GetAddr(AMX *amx, cell amx_addr, cell **phys_addr)
	{
		int ret = reinterpret_cast<decltype(&amx_GetAddr)>(subhook_get_trampoline(amx_GetAddr_h))(amx, amx_addr, phys_addr);

		if(ret == AMX_ERR_MEMACCESS)
		{
			if(strings::pool.is_null_address(amx, amx_addr))
			{
				strings::null_value1[0] = 0;
				*phys_addr = strings::null_value1;
				return AMX_ERR_NONE;
			}
			auto ptr = strings::pool.get(amx, amx_addr);
			if(ptr != nullptr)
			{
				strings::pool.set_cache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}else if(ret == 0 && hook_ref_args)
		{
			// Variadic functions pass all arguments by ref
			// so checking the actual cell value is necessary,
			// but there is a chance that it will interpret
			// a number as a string. Better have it disabled by default.
			if(strings::pool.is_null_address(amx, **phys_addr))
			{
				strings::null_value2[0] = 1;
				strings::null_value2[1] = 0;
				*phys_addr = strings::null_value2;
				return AMX_ERR_NONE;
			}
			auto ptr = strings::pool.get(amx, **phys_addr);
			if(ptr != nullptr)
			{
				strings::pool.set_cache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}
		return ret;
	}

	int AMXAPI amx_StrLen(const cell *cstring, int *length)
	{
		auto str = strings::pool.find_cache(cstring);
		if(str != nullptr)
		{
			*length = str->size();
			return AMX_ERR_NONE;
		}

		return reinterpret_cast<decltype(&amx_StrLen)>(subhook_get_trampoline(amx_StrLen_h))(cstring, length);
	}

	int AMXAPI amx_FindPublic(AMX *amx, const char *funcname, int *index)
	{
		int id = events::get_callback_id(amx, funcname);
		if(id != -1)
		{
			*index = -3 - id;
			return AMX_ERR_NONE;
		}

		return amx_FindPublicOrig(amx, funcname, index);
	}

	int AMXAPI amx_Register(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
	{
		int ret = reinterpret_cast<decltype(&amx_Register)>(subhook_get_trampoline(amx_Register_h))(amx, nativelist, number);
		amx::register_natives(amx, nativelist, number);
		return ret;
	}
}

template <class Func>
void RegisterAmxHook(subhook_t &hook, int index, Func *fnptr)
{
	hook = subhook_new(reinterpret_cast<void*>(((Func**)pAMXFunctions)[index]), reinterpret_cast<void*>(fnptr), {});
	subhook_install(hook);
}

void Hooks::register_callback()
{
	RegisterAmxHook(amx_Init_h, PLUGIN_AMX_EXPORT_Init, &Hooks::amx_Init);
	RegisterAmxHook(amx_Exec_h, PLUGIN_AMX_EXPORT_Exec, &Hooks::amx_Exec);
	RegisterAmxHook(amx_GetAddr_h, PLUGIN_AMX_EXPORT_GetAddr, &Hooks::amx_GetAddr);
	RegisterAmxHook(amx_StrLen_h, PLUGIN_AMX_EXPORT_StrLen, &Hooks::amx_StrLen);
	RegisterAmxHook(amx_FindPublic_h, PLUGIN_AMX_EXPORT_FindPublic, &Hooks::amx_FindPublic);
	RegisterAmxHook(amx_Register_h, PLUGIN_AMX_EXPORT_Register, &Hooks::amx_Register);
}

void UnregisterHook(subhook_t hook)
{
	subhook_remove(hook);
	subhook_free(hook);
}

void Hooks::Unregister()
{
	UnregisterHook(amx_Init_h);
	UnregisterHook(amx_Exec_h);
	UnregisterHook(amx_GetAddr_h);
	UnregisterHook(amx_StrLen_h);
	UnregisterHook(amx_FindPublic_h);
	UnregisterHook(amx_Register_h);
}

void Hooks::ToggleStrLen(bool toggle)
{
	if(toggle)
	{
		subhook_install(amx_StrLen_h);
	}else{
		subhook_remove(amx_StrLen_h);
	}
}

void Hooks::ToggleRefArgs(bool toggle)
{
	hook_ref_args = toggle;
}
