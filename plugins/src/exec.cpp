#include "exec.h"

#include "main.h"
#include "context.h"
#include "hooks.h"
#include "pools.h"

#include "modules/threads.h"
#include "modules/events.h"
#include "modules/tasks.h"
#include "modules/amxutils.h"
#include "fixes/linux.h"

#include <cstring>

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

struct forked_context : public amx::extra
{
	bool cloned = false;

	forked_context(AMX *amx) : amx::extra(amx)
	{

	}

	virtual ~forked_context() override
	{
		if(cloned)
		{
			amx::unload(_amx);
		}
	}
};

amx_code_info::amx_code_info(AMX *amx) : amx::extra(amx)
{
	auto amxhdr = (AMX_HEADER*)amx->base;
	code = std::make_unique<unsigned char[]>(amxhdr->size - amxhdr->cod);
	std::memcpy(code.get(), amx->base + amxhdr->cod, amxhdr->size - amxhdr->cod);
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

	if(!forked)
	{
		Threads::PauseThreads(amx);
		Threads::JoinThreads(amx);
	}

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
		ctx.get_extra<forked_context>().cloned = owner->has_extra<forked_amx_holder>();
	}

	int ret;
	while(true)
	{
		ret = amx_ExecOrig(amx, retval, index);
		if(ret == AMX_ERR_SLEEP || amx->error == AMX_ERR_SLEEP)
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
					tasks::register_tick(ticks, amx::reset(amx, true));
				}
				break;
				case SleepReturnWaitMs:
				{
					cell interval = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					tasks::register_timer(interval, amx::reset(amx, true));
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
					auto &extra = ctx.get_extra<fork_info_extra>();
					cell result_addr = extra.result_address;
					cell error_addr = extra.error_address;

					cell flags = SleepReturnValueMask & amx->pri;

					cell method = flags & SleepReturnForkFlagsMethodMask;

					auto amxhdr = (AMX_HEADER*)amx->base;
					if(method == 0)
					{
						amx->pri = 1;
						amx->error = AMX_ERR_NONE;

						cell cip = amx->cip, stk = amx->stk, hea = amx->hea, frm = amx->frm;

						cell result, error;
						error = amx_ExecContext(amx, &result, AMX_EXEC_CONT, false, nullptr, true);

						if(amx->error == AMX_ERR_SLEEP && (amx->pri & SleepReturnTypeMask) == SleepReturnForkCommit)
						{

						} else {
							amx->cip = cip;
							amx->stk = stk;
							amx->hea = hea;
							amx->frm = frm;

							cell *result_var, *error_var;
							amx_GetAddr(amx, result_addr, &result_var);
							amx_GetAddr(amx, error_addr, &error_var);
							if(result_var) *result_var = result;
							if(error_var) *error_var = error;
						}
					} else if(method == 1)
					{
						amx::reset reset(amx, true);

						unsigned char *orig_data;
						if(flags & SleepReturnForkFlagsCopyData)
						{
							orig_data = new unsigned char[amxhdr->hea - amxhdr->dat];
							std::memcpy(orig_data, amx->base + amxhdr->dat, amxhdr->hea - amxhdr->dat); // backup the data
						}

						amx->pri = 1;
						amx->error = AMX_ERR_NONE;

						cell result, error;
						error = amx_ExecContext(amx, &result, AMX_EXEC_CONT, false, nullptr, true);

						if(amx->error == AMX_ERR_SLEEP && (amx->pri & SleepReturnTypeMask) == SleepReturnForkCommit)
						{
							if(amx->pri & SleepReturnValueMask)
							{
								reset = amx::reset(amx, true);
								amx::pop(amx);
								reset.context.remove_extra<forked_context>();
								reset.restore();
							} else {
								amx::pop(amx);
								reset.restore();
							}
						} else {
							reset.restore();
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx->base + amxhdr->dat, orig_data, amxhdr->hea - amxhdr->dat);
							}
							cell *result_var, *error_var;
							amx_GetAddr(amx, result_addr, &result_var);
							amx_GetAddr(amx, error_addr, &error_var);
							if(result_var) *result_var = result;
							if(error_var) *error_var = error;
						}
						if(flags & SleepReturnForkFlagsCopyData)
						{
							delete[] orig_data;
						}
					} else if(method == 2)
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
						cell *result, *error;
						amx_GetAddr(amx, result_addr, &result);
						amx_GetAddr(amx, error_addr, &error);
						*error = amx_ExecContext(amx_fork, result, AMX_EXEC_CONT, false, nullptr, true);

						if(amx_fork->error == AMX_ERR_SLEEP && (amx_fork->pri & SleepReturnTypeMask) == SleepReturnForkCommit)
						{
							if(amx_fork->pri & SleepReturnValueMask)
							{
								amx::reset reset(amx_fork, true);
								amx::pop(amx_fork);
								reset.context.remove_extra<forked_context>();
								reset.amx = owner;
								reset.restore();
							} else {
								amx::reset reset(amx_fork, false);
								amx::pop(amx_fork);
								reset.amx = owner;
								reset.restore_no_context();
							}
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx->base + amxhdr->dat, amx_fork->base + amxhdr->dat, amxhdr->hea - amxhdr->dat);
							}
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
						} else {
							logwarn(amx, "[PP] amx_commit was called from a non-forked code.");
						}
						continue;
					} else {
						return ret;
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
					} else {
						if(retval != nullptr) *retval = info.result;
						amx->error = ret = AMX_ERR_NONE;
					}
				}
				break;
				case SleepReturnAllocVar:
				case SleepReturnAllocVarZero:
				{
					cell size = SleepReturnValueMask & amx->pri;
					cell amx_addr, *addr;
					if(amx_Allot(amx, size, &amx_addr, &addr) == AMX_ERR_NONE)
					{
						auto var = amx_var_pool.add(amx_var_info(amx, amx_addr, size));
						if((amx->pri & SleepReturnTypeMask) == SleepReturnAllocVarZero)
						{
							var->fill(0);
						}
						amx->pri = reinterpret_cast<cell>(var);
					} else {
						amx->pri = 0;
					}
					amx->error = ret = AMX_ERR_NONE;
					continue;
				}
				break;
				case SleepReturnFreeVar:
				{
					amx_Release(amx, SleepReturnValueMask & amx->pri);
					amx->pri = 1;
					amx->error = ret = AMX_ERR_NONE;
					continue;
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
	if(!forked)
	{
		Threads::ResumeThreads(amx);
	}
	return ret;
}
