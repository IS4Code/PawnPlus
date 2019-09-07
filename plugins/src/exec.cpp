#include "exec.h"

#include "main.h"
#include "context.h"
#include "hooks.h"

#include "modules/threads.h"
#include "modules/events.h"
#include "modules/tasks.h"
#include "modules/amxutils.h"
#include "modules/debug.h"
#include "modules/containers.h"
#include "fixes/linux.h"

#include <cstring>
#include <limits>

int maxRecursionLevel = std::numeric_limits<int>::max();

// Automatically destroys the AMX machine when the info instance is destroyed (i.e. the AMX becomes unused)
struct forked_amx_holder : public amx::extra
{
	forked_amx_holder(AMX *amx) : amx::extra(amx)
	{

	}

	virtual ~forked_amx_holder() override
	{
		amx_Cleanup(_amx);
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

struct parallel_context : public amx::extra
{
	AMX_DEBUG old_debug = nullptr;
	bool on_break = false;
	int count = 0;
	int max_count = 1;

	parallel_context(AMX *amx) : amx::extra(amx)
	{

	}
};

amx_code_info::amx_code_info(AMX *amx) : amx::extra(amx)
{
	auto amxhdr = (AMX_HEADER*)amx->base;
	code = std::make_unique<unsigned char[]>(amxhdr->size - amxhdr->cod);
	std::memcpy(code.get(), amx->base + amxhdr->cod, amxhdr->size - amxhdr->cod);
}

int AMXAPI amx_on_debug(AMX *amx)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &extra = ctx.get_extra<parallel_context>();
	if(extra.old_debug)
	{
		int ret = extra.old_debug(amx);
		if(ret != AMX_ERR_NONE) return ret;
	}
	if(++extra.count == extra.max_count)
	{
		extra.on_break = true;
		extra.count = 0;
		return AMX_ERR_SLEEP;
	}
	return AMX_ERR_NONE;
}

int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, amx::reset *reset, bool forked)
{
	bool main_thread = is_main_thread;

	amx::reset *old = nullptr;
	bool restore_context = false;
	AMX_DEBUG old_debug;
	bool restore_debug = false;
	if(main_thread)
	{
		if(amx::context_level >= maxRecursionLevel)
		{
			return logerror(amx, AMX_ERR_GENERAL, "[PawnPlus] native recursion depth %d too high (limit is %d, use pp_max_recursion to increase it)", amx::context_level, maxRecursionLevel);
		}

		if(events::invoke_callbacks(amx, index, retval)) return amx->error;

		if(!forked)
		{
			Threads::PauseThreads(amx);
			Threads::JoinThreads(amx);
		}

		if(restore)
		{
			if(amx::has_context(amx))
			{
				restore_context = true;
				old = new amx::reset(amx, true);
			}else{
				old = new amx::reset(amx, false);
			}
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

		if(restore)
		{
			amx::object owner;
			auto &ctx = amx::get_context(amx, owner);
			if(ctx.has_extra<parallel_context>())
			{
				ctx.get_extra<parallel_context>().old_debug = old_debug = amx->debug;
				amx->debug = amx_on_debug;
				restore_debug = true;
			}
		}else if(amx->debug == amx_on_debug && amx::has_parent_context(amx))
		{
			amx::object owner;
			auto &ctx = amx::get_parent_context(amx, owner);
			if(ctx.has_extra<parallel_context>())
			{
				auto &extra = ctx.get_extra<parallel_context>();
				amx->debug = extra.old_debug;
				old_debug = amx_on_debug;
				restore_debug = true;
			}
		}
	}

	int ret;
	while(true)
	{
		cell old_hea, old_stk;
		if(index == AMX_EXEC_CONT)
		{
			old_hea = amx->reset_hea;
			old_stk = amx->reset_stk;
		}else{
			old_hea = amx->hea;
			old_stk = amx->stk + sizeof(cell) * amx->paramcount;
		}
		ret = amx_ExecOrig(amx, retval, index);
		bool handled = false;
		if(ret == AMX_ERR_SLEEP || amx->error == AMX_ERR_SLEEP)
		{
			index = AMX_EXEC_CONT;

			handled = true;

			if(old_hea != amx->reset_hea)
			{
				cell new_hea = amx->reset_hea;
				logwarn(amx, "[PawnPlus] HEA will be restored to 0x%X (originally 0x%X).", new_hea, old_hea);
			}
			if(old_stk != amx->reset_stk)
			{
				cell new_stk = amx->reset_stk;
				logwarn(amx, "[PawnPlus] STK will be restored to 0x%X (originally 0x%X).", new_stk, old_stk);
			}

			amx::object owner;
			auto &ctx = amx::get_context(amx, owner);
			tasks::extra &info = tasks::get_extra(amx, owner);

			if(ctx.has_extra<parallel_context>())
			{
				auto &extra = ctx.get_extra<parallel_context>();
				if(extra.on_break)
				{
					extra.on_break = false;
					extra.old_debug = nullptr;

					if(restore_debug)
					{
						amx->debug = old_debug;
						restore_debug = false;
					}

					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					tasks::register_tick(1, amx::reset(amx, true, info.restore_heap, info.restore_stack));

					amx->stk = amx->reset_stk;
					amx->hea = amx->reset_hea;
					break;
				}
			}

			switch(amx->pri & SleepReturnTypeMask)
			{
				default:
				{
					handled = false;
				}
				break;
				case SleepReturnAwait:
				{
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(auto task = info.awaited_task.lock())
					{
						if(retval != nullptr) *retval = info.result;
						info.awaited_task = {};
						task->register_reset(amx::reset(amx, true, info.restore_heap, info.restore_stack));
					}
				}
				break;
				case SleepReturnWaitTicks:
				{
					cell ticks = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					tasks::register_tick(ticks, amx::reset(amx, true, info.restore_heap, info.restore_stack));
				}
				break;
				case SleepReturnWaitMs:
				{
					cell interval = SleepReturnValueMask & amx->pri;
					amx->pri = 0;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					tasks::register_timer(interval, amx::reset(amx, true, info.restore_heap, info.restore_stack));
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
					logwarn(amx, "[PawnPlus] thread_attach was called in a non-threaded code.");
					continue;
				}
				break;
				case SleepReturnSync:
				{
					amx->pri = 0;
					logwarn(amx, "[PawnPlus] thread_sync was called in a non-threaded code.");
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

						}else{
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
					}else if(method == 1)
					{
						amx::reset reset(amx, true);

						unsigned char *orig_data;
						if(flags & SleepReturnForkFlagsCopyData)
						{
							orig_data = new unsigned char[amxhdr->hea - amxhdr->dat];
							std::memcpy(orig_data, amx_GetData(amx), amxhdr->hea - amxhdr->dat); // backup the data
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
							}else{
								amx::pop(amx);
								reset.restore();
							}
						}else{
							reset.restore();
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx_GetData(amx), orig_data, amxhdr->hea - amxhdr->dat);
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
					}else if(method == 2)
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
							logwarn(amx, "[PawnPlus] amx_fork: couldn't create the fork (error %d).", initret);
							continue;
						}
						if(flags & SleepReturnForkFlagsCopyData)
						{
							std::memcpy(amx_fork->base + amxhdr->dat, amx_GetData(amx), amxhdr->hea - amxhdr->dat); // copy the data
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
							}else{
								amx::reset reset(amx_fork, false);
								amx::pop(amx_fork);
								reset.amx = owner;
								reset.restore_no_context();
							}
							if(flags & SleepReturnForkFlagsCopyData)
							{
								std::memcpy(amx_GetData(amx), amx_fork->base + amxhdr->dat, amxhdr->hea - amxhdr->dat);
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
							logwarn(amx, "[PawnPlus] amx_commit: the original context is no longer present.");
						}else{
							logwarn(amx, "[PawnPlus] amx_commit was called from a non-forked code.");
						}
						continue;
					}else{
						return ret;
					}
				}
				break;
				case SleepReturnForkEnd:
				{
					if(!forked && !ctx.has_extra<forked_context>())
					{
						amx->pri = 0;
						logwarn(amx, "[PawnPlus] amx_fork_end was called from a non-forked code.");
						continue;
					}else{
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
					try{
						amx_AllotSafe(amx, size, &amx_addr, &addr);
						auto var = amx_var_pool.add(amx_var_info(amx, amx_addr, size));
						if((amx->pri & SleepReturnTypeMask) == SleepReturnAllocVarZero)
						{
							var->fill(0);
						}
						amx->pri = amx_var_pool.get_id(var);
					}catch(const errors::amx_error &)
					{
						amx->pri = 0;
					}
					amx->error = ret = AMX_ERR_NONE;
					continue;
				}
				break;
				case SleepReturnFreeVar:
				{
					amx_Release(amx, (SleepReturnValueMask & amx->pri) * sizeof(cell) + amx->hlw);
					amx->pri = 1;
					amx->error = ret = AMX_ERR_NONE;
					continue;
				}
				break;
				case SleepReturnParallel:
				{
					if(ctx.has_extra<parallel_context>())
					{
						amx->pri = 0;
						logwarn(amx, "[PawnPlus] amx_parallel_begin was called from a parallel code.");
						continue;
					}
					auto &extra = ctx.get_extra<parallel_context>();
					extra.max_count = SleepReturnValueMask & amx->pri;

					amx->pri = 1;
					amx->error = ret = AMX_ERR_NONE;
					if(retval != nullptr) *retval = info.result;
					tasks::register_tick(1, amx::reset(amx, true, info.restore_heap, info.restore_stack));
				}
				break;
				case SleepReturnParallelEnd:
				{
					if(!ctx.has_extra<parallel_context>())
					{
						logwarn(amx, "[PawnPlus] amx_parallel_end was called from a non-parallel code.");
					}else{
						auto &extra = ctx.get_extra<parallel_context>();
						amx->debug = extra.old_debug;
						restore_debug = false;
						ctx.remove_extra<parallel_context>();
					}
					amx->pri = 0;
					continue;
				}
				break;
				case SleepReturnDebugCall:
				{
					cell index = SleepReturnValueMask & amx->pri;
					auto dbg = amx::load_lock(amx)->dbg.get();
					if(dbg)
					{
						auto data = amx_GetData(amx);
						auto stk = reinterpret_cast<cell*>(data + amx->stk);
						cell num = *stk - sizeof(cell);
						amx->stk -= num + 2 * sizeof(cell);
						for(cell i = 0; i < num; i += sizeof(cell))
						{
							cell val = *reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + num + sizeof(cell));
							*--stk = val;
						}
						*--stk = num;

						auto sym = dbg->symboltbl[index];
						for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
						{
							auto vsym = dbg->symboltbl[i];
							if(vsym->ident == iVARIABLE && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
							{
								cell addr = vsym->address - 2 * sizeof(cell);
								if(addr > 0)
								{
									auto arg = reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + addr);
									cell val = *arg;
									*arg = *reinterpret_cast<cell*>(data + val);
								}
							}
						}

						*--stk = amx->cip;
						amx->cip = sym->address;
						amx->error = ret = AMX_ERR_NONE;
						amx->pri = 0;
						continue;
					}
				}
				break;
				case SleepReturnDebugCallList:
				{
					cell index = SleepReturnValueMask & amx->pri;
					auto dbg = amx::load_lock(amx)->dbg.get();
					if(dbg)
					{
						auto data = amx_GetData(amx);
						auto stk = reinterpret_cast<cell*>(data + amx->stk);

						list_t *list;
						if(list_pool.get_by_id(stk[2], list))
						{
							cell num = list->size() * sizeof(cell);
							amx->stk -= num + 2 * sizeof(cell);
							for(cell i = list->size() - 1; i >= 0; i--)
							{
								cell val = (*list)[i].store(amx);
								*--stk = val;
							}
							*--stk = num;
							auto sym = dbg->symboltbl[index];
							*--stk = amx->cip;
							amx->cip = sym->address;
							amx->error = ret = AMX_ERR_NONE;
							amx->pri = 0;
							continue;
						}
					}
				}
				break;
				case SleepReturnTaskDetach:
				{
					amx->error = ret = AMX_ERR_NONE;
					amx->pri = 1;
					auto data = amx_GetData(amx);
					auto frame = reinterpret_cast<cell*>(data + amx->frm);
					cell frm = frame[0];
					cell cip = frame[1];
					cell reset_stk = amx->reset_stk;
					frame[0] = frame[1] = 0;
					amx->reset_stk = amx->frm + 3 * sizeof(cell) + frame[2];

					cell retval = 0;
					amx_Exec(amx, &retval, AMX_EXEC_CONT);
					amx->pri = retval;

					amx->stk = amx->reset_stk;
					amx->reset_stk = reset_stk;
					amx->frm = frm;
					amx->cip = cip;
					continue;
				}
				break;
				case SleepReturnTaskDetachBound:
				{
					std::shared_ptr<tasks::task> task;
					auto data = amx_GetData(amx);
					if(tasks::get_by_id(reinterpret_cast<cell*>(data + amx->stk)[1], task))
					{
						amx->error = ret = AMX_ERR_NONE;
						amx->pri = 1;
						auto frame = reinterpret_cast<cell*>(data + amx->frm);
						cell frm = frame[0];
						cell cip = frame[1];
						cell reset_stk = amx->reset_stk;
						frame[0] = frame[1] = 0;
						amx->reset_stk = amx->frm + 3 * sizeof(cell) + frame[2];

						amx::reset reset(amx, false);
						reset.context.get_extra<tasks::extra>().bound_task = task;
						cell retval = 0;
						amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
						amx->pri = retval;

						amx->stk = amx->reset_stk;
						amx->reset_stk = reset_stk;
						amx->frm = frm;
						amx->cip = cip;
					}
					continue;
				}
				break;
				case SleepReturnThreadFix:
				{
					amx->error = ret = AMX_ERR_NONE;
					amx->pri = 1;
					Threads::QueueAndWait(amx, *retval, ret);
				}
				break;
				case SleepReturnTailCall:
				{
					amx->error = ret = AMX_ERR_NONE;
					auto data = amx_GetData(amx);
					auto frame = reinterpret_cast<cell*>(data + amx->frm);
					if(frame[1])
					{
						amx->pri = 1;
						cell top = amx->frm + 3 * sizeof(cell) + frame[2];
						auto oldframe = reinterpret_cast<cell*>(data + frame[0]);
						cell oldtop = frame[0] + 3 * sizeof(cell) + oldframe[2];
						cell offset = oldtop - top;
						frame[0] = oldframe[0];
						frame[1] = oldframe[1];
						std::memmove(data + amx->stk + offset, data + amx->stk, top - amx->stk);
						amx->stk += offset;
						amx->frm += offset;
					}else{
						amx->pri = 0;
					}
					continue;
				}
				break;
			}

			if(handled)
			{
				amx->stk = amx->reset_stk;
				amx->hea = amx->reset_hea;
			}
		}
		logdebug("[PawnPlus] [debug] amx_Exec(0x%X, %d) == %d (%s)", reinterpret_cast<intptr_t>(amx), index, ret, amx::StrError(ret));

		if(restore_debug)
		{
			amx->debug = old_debug;
		}

		if(main_thread)
		{
			if(restore && !handled && ret != AMX_ERR_SLEEP)
			{
				amx::object owner;
				tasks::extra &info = tasks::get_extra(amx, owner);
				if(auto task = info.bound_task.lock())
				{
					if(ret == AMX_ERR_NONE)
					{
						task->set_completed(dyn_object(*retval, tags::find_tag(tags::tag_cell)));
					}else{
						task->set_faulted(ret);
					}
				}
			}
		}

		break;
	}

	if(main_thread)
	{
		amx::pop(amx);
		if(old != nullptr)
		{
			if(restore_context)
			{
				old->restore();
			} else {
				old->restore_no_context();
			}
			delete old;
		}
		if(!forked)
		{
			Threads::ResumeThreads(amx);
		}
	}
	return ret;
}
