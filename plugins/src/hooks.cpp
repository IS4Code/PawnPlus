#include "hooks.h"
#include "main.h"
#include "context.h"
#include "amxinfo.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/events.h"
#include "modules/threads.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

#include <cstring>
#include <unordered_map>

extern void *pAMXFunctions;

extern int ExecLevel;

subhook_t amx_Exec_h;
subhook_t amx_GetAddr_h;
subhook_t amx_StrLen_h;
subhook_t amx_FindPublic_h;
subhook_t amx_Register_h;

bool hook_ref_args = false;

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

int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, AMX_RESET *reset)
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

	AMX_RESET *old = nullptr;
	if(restore && Context::IsPresent(amx))
	{
		old = new AMX_RESET(amx, true);
	}

	Context::Push(amx);
	if(reset != nullptr)
	{
		reset->restore();
	}
	int ret;
	while(true)
	{
		ret = amx_ExecOrig(amx, retval, index);
		if(ret == AMX_ERR_SLEEP)
		{
			index = AMX_EXEC_CONT;

			auto &ctx = Context::Get(amx);
			switch(amx->pri & SleepReturnTypeMask)
			{
				case SleepReturnAwait:
				{
					task_id task = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = ctx.result;
					TaskPool::Get(task)->register_callback(AMX_RESET(amx, true));
				}
				break;
				case SleepReturnWaitTicks:
				{
					cell ticks = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = ctx.result;
					TaskPool::RegisterTicks(ticks, AMX_RESET(amx, true));
				}
				break;
				case SleepReturnWaitMs:
				{
					cell interval = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = ctx.result;
					TaskPool::RegisterTimer(interval, AMX_RESET(amx, true));
				}
				break;
				case SleepReturnWaitInf:
				{
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = ctx.result;
				}
				break;
				case SleepReturnDetach:
				{
					auto flags = static_cast<Threads::SyncFlags>(SleepReturnValueMask & amx->pri);
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = ctx.result;
					Threads::DetachThread(amx, flags);
				}
				break;
				case SleepReturnAttach:
				{
					logwarn(amx, "[PP] thread_attach was called in a non-threaded code.");
					continue;
				}
				break;
				case SleepReturnSync:
				{
					logwarn(amx, "[PP] thread_sync was called in a non-threaded code.");
					continue;
				}
				break;
			}
		}
		break;
	}
	Context::Pop(amx);
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
