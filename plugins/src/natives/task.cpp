#include "natives.h"
#include "context.h"
#include "exec.h"
#include "modules/tasks.h"
#include <algorithm>

using namespace tasks;

namespace Natives
{
	// native Task:wait_ticks(ticks);
	static cell AMX_NATIVE_CALL wait_ticks(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		if(params[1] < 0)
		{
			return SleepReturnWaitInf;
		}else{
			return SleepReturnWaitTicks | (SleepReturnValueMask & params[1]);
		}
	}

	// native Task:wait_ms(interval);
	static cell AMX_NATIVE_CALL wait_ms(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		if(params[1] < 0)
		{
			return SleepReturnWaitInf;
		}else{
			return SleepReturnWaitMs | (SleepReturnValueMask & params[1]);
		}
	}

	// native Task:task_new();
	static cell AMX_NATIVE_CALL task_new(AMX *amx, cell *params)
	{
		return tasks::get_id(tasks::add().get());
	}

	// native bool:task_delete(Task:task);
	static cell AMX_NATIVE_CALL task_delete(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return tasks::remove(task);
		}
		return 0;
	}

	// native Task:task_keep(Task:task, bool:keep=true);
	static cell AMX_NATIVE_CALL task_keep(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			task->keep(optparam(2, 1));
			return params[1];
		}
		return 0;
	}

	// native bool:task_valid(Task:task);
	static cell AMX_NATIVE_CALL task_valid(AMX *amx, cell *params)
	{
		task *task;
		return tasks::get_by_id(params[1], task);
	}

	// native task_set_result(Task:task, AnyTag:result);
	static cell AMX_NATIVE_CALL task_set_result(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			task->set_completed(params[2]);
			return 1;
		}
		return 0;
	}

	// native task_get_result(Task:task);
	static cell AMX_NATIVE_CALL task_get_result(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			cell result;
			if(task->result_or_error(amx, result))
			{
				return result;
			}
		}
		return 0;
	}

	// native task_reset(Task:task);
	static cell AMX_NATIVE_CALL task_reset(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			task->reset();
			return 1;
		}
		return 0;
	}

	// native task_set_error(Task:task, amx_err:error);
	static cell AMX_NATIVE_CALL task_set_error(AMX *amx, cell *params)
	{
		if(params[2] == AMX_ERR_SLEEP || params[2] == AMX_ERR_NONE)
		{
			return 0;
		}
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			task->set_faulted(params[2]);
			return 1;
		}
		return 0;
	}

	// native amx_err:task_get_error(Task:task);
	static cell AMX_NATIVE_CALL task_get_error(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->error();
		}
		return 0;
	}

	// native bool:task_completed(Task:task);
	static cell AMX_NATIVE_CALL task_completed(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->completed();
		}
		return 0;
	}

	// native bool:task_faulted(Task:task);
	static cell AMX_NATIVE_CALL task_faulted(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->faulted();
		}
		return 0;
	}

	// native task_state:task_state(Task:task);
	static cell AMX_NATIVE_CALL task_state(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->state();
		}
		return 0;
	}

	// native Task:task_ticks(ticks);
	static cell AMX_NATIVE_CALL task_ticks(AMX *amx, cell *params)
	{
		return tasks::get_id(add_tick_task(params[1]).get());
	}

	// native Task:task_ms(interval);
	static cell AMX_NATIVE_CALL task_ms(AMX *amx, cell *params)
	{
		return tasks::get_id(add_timer_task(params[1]).get());
	}

	// native Task:task_any(Task:...);
	static cell AMX_NATIVE_CALL task_any(AMX *amx, cell *params)
	{
		auto created = tasks::add();
		cell num = params[0] / sizeof(cell);
		auto list = std::make_shared<std::vector<std::pair<tasks::task::handler_iterator, std::weak_ptr<tasks::task>>>>();
		for(cell i = 1; i <= num; i++)
		{
			cell *addr;
			amx_GetAddr(amx, params[i], &addr);
			std::shared_ptr<task> task;
			if(tasks::get_by_id(*addr, task))
			{
				auto it = task->register_handler([created, list](tasks::task &t)
				{
					for(auto &reg : *list)
					{
						if(auto lock2 = reg.second.lock())
						{
							if(lock2.get() != &t)
							{
								lock2->unregister_handler(reg.first);
							}
						}
					}
					list->clear();
					created->set_completed(tasks::get_id(&t));
				});
				list->push_back(std::make_pair(it, task));
			}
		}
		return tasks::get_id(created.get());
	}

	// native Task:task_all(Task:...);
	static cell AMX_NATIVE_CALL task_all(AMX *amx, cell *params)
	{
		auto created = tasks::add();
		cell num = params[0] / sizeof(cell);
		auto list = std::make_shared<std::vector<std::weak_ptr<tasks::task>>>();
		for(cell i = 1; i <= num; i++)
		{
			cell *addr;
			amx_GetAddr(amx, params[i], &addr);
			std::shared_ptr<task> task;
			if(tasks::get_by_id(*addr, task))
			{
				task->register_handler([created, list](tasks::task &t)
				{
					if(std::all_of(list->begin(), list->end(), [](std::weak_ptr<tasks::task> &ptr)
					{
						if(auto lock = ptr.lock())
						{
							return lock->completed();
						}
						return true;
					}))
					{
						list->clear();
						created->set_completed(tasks::get_id(&t));
					}
				});
				list->push_back(task);
			}
		}
		return tasks::get_id(created.get());
	}

	// native task_state:task_wait(Task:task);
	static cell AMX_NATIVE_CALL task_wait(AMX *amx, cell *params)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			if(!task->completed() && !task->faulted())
			{
				amx::object owner;
				auto &info = tasks::get_extra(amx, owner);
				info.awaited_task = task;

				amx_RaiseError(amx, AMX_ERR_SLEEP);
				return SleepReturnAwait;
			}else{
				return task->state();
			}
		}
		return 0;
	}

	// native task_yield(AnyTag:value);
	static cell AMX_NATIVE_CALL task_yield(AMX *amx, cell *params)
	{
		amx::object owner;
		auto &info = tasks::get_extra(amx, owner);
		info.result = params[1];
		if(auto task = info.bound_task.lock())
		{
			task->set_completed(info.result);
			return 1;
		}
		return 0;
	}

	// native bool:task_bind(Task:task, const function[], const format[], AnyTag:...);
	static cell AMX_NATIVE_CALL task_bind(AMX *amx, cell *params)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			char *fname;
			amx_StrParam(amx, params[2], fname);

			char *format;
			amx_StrParam(amx, params[3], format);

			if(fname == nullptr) return 0;

			int numargs = format == nullptr ? 0 : std::strlen(format);
			if(numargs > 0 && format[numargs - 1] == '+')
			{
				numargs--;
			}

			if(params[0] < (3 + numargs) * static_cast<int>(sizeof(cell)))
			{
				logerror(amx, "[PP] task_bind: not enough arguments");
				return 0;
			}

			int pubindex = 0;

			if(amx_FindPublic(amx, fname, &pubindex) == AMX_ERR_NONE)
			{
				if(format[numargs] == '+')
				{
					for(int i = (params[0] / sizeof(cell)) - 1; i >= numargs; i--)
					{
						amx_Push(amx, params[params[4 + i]]);
					}
				}

				for(int i = numargs - 1; i >= 0; i--)
				{
					cell param = params[4 + i];
					cell *addr;

					switch(format[i])
					{
						case 'a':
						case 's':
						case '*':
						{
							amx_Push(amx, param);
							break;
						}
						case '+':
						{
							logerror(amx, "[PP] task_bind: + must be the last specifier");
							return 0;
						}
						default:
						{
							amx_GetAddr(amx, param, &addr);
							param = *addr;
							amx_Push(amx, param);
							break;
						}
					}
				}

				amx::reset reset(amx, false);
				reset.context.get_extra<tasks::extra>().bound_task = task;

				cell result;
				amx_ExecContext(amx, &result, pubindex, true, &reset);
				amx->error = AMX_ERR_NONE;
				return 1;
			}
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(wait_ticks),
	AMX_DECLARE_NATIVE(wait_ms),
	AMX_DECLARE_NATIVE(task_new),
	AMX_DECLARE_NATIVE(task_delete),
	AMX_DECLARE_NATIVE(task_valid),
	AMX_DECLARE_NATIVE(task_keep),
	AMX_DECLARE_NATIVE(task_set_result),
	AMX_DECLARE_NATIVE(task_get_result),
	AMX_DECLARE_NATIVE(task_reset),
	AMX_DECLARE_NATIVE(task_completed),
	AMX_DECLARE_NATIVE(task_faulted),
	AMX_DECLARE_NATIVE(task_state),
	AMX_DECLARE_NATIVE(task_set_error),
	AMX_DECLARE_NATIVE(task_get_error),
	AMX_DECLARE_NATIVE(task_ticks),
	AMX_DECLARE_NATIVE(task_ms),
	AMX_DECLARE_NATIVE(task_any),
	AMX_DECLARE_NATIVE(task_all),
	AMX_DECLARE_NATIVE(task_wait),
	AMX_DECLARE_NATIVE(task_yield),
	AMX_DECLARE_NATIVE(task_bind),
};

int RegisterTasksNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
