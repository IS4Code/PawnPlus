#include "natives.h"
#include "context.h"
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
		return reinterpret_cast<cell>(task::add().get());
	}

	// native bool:task_delete(Task:task);
	static cell AMX_NATIVE_CALL task_delete(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		return tasks::remove(task);
	}

	// native Task:task_keep(Task:task, bool:keep=true);
	static cell AMX_NATIVE_CALL task_keep(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			task->keep(params[2]);
			return params[1];
		}
		return 0;
	}

	// native bool:task_valid(Task:task);
	static cell AMX_NATIVE_CALL task_valid(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		return tasks::contains(task);
	}

	// native task_set_result(Task:task, AnyTag:result);
	static cell AMX_NATIVE_CALL task_set_result(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			task->set_completed(params[2]);
			return 1;
		}
		return 0;
	}

	// native task_get_result(Task:task);
	static cell AMX_NATIVE_CALL task_get_result(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
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
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
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
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			task->set_faulted(params[2]);
			return 1;
		}
		return 0;
	}

	// native amx_err:task_get_error(Task:task);
	static cell AMX_NATIVE_CALL task_get_error(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			return task->error();
		}
		return 0;
	}

	// native bool:task_completed(Task:task);
	static cell AMX_NATIVE_CALL task_completed(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			return task->completed();
		}
		return 0;
	}

	// native bool:task_faulted(Task:task);
	static cell AMX_NATIVE_CALL task_faulted(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			return task->faulted();
		}
		return 0;
	}

	// native task_state:task_state(Task:task);
	static cell AMX_NATIVE_CALL task_state(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		if(tasks::contains(task))
		{
			return task->state();
		}
		return 0;
	}

	// native Task:task_ticks(ticks);
	static cell AMX_NATIVE_CALL task_ticks(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(task::add_tick_task(params[1]).get());
	}

	// native Task:task_ms(interval);
	static cell AMX_NATIVE_CALL task_ms(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(task::add_timer_task(params[1]).get());
	}

	// native Task:task_any(Task:...);
	static cell AMX_NATIVE_CALL task_any(AMX *amx, cell *params)
	{
		auto created = task::add();
		cell num = params[0] / sizeof(cell);
		auto list = std::shared_ptr<std::vector<std::pair<tasks::task::handler_iterator, std::weak_ptr<tasks::task>>>>(new std::vector<std::pair<tasks::task::handler_iterator, std::weak_ptr<tasks::task>>>());
		for(cell i = 1; i <= num; i++)
		{
			cell *addr;
			amx_GetAddr(amx, params[i], &addr);
			auto task = reinterpret_cast<tasks::task*>(*addr);
			if(auto lock = tasks::find(task))
			{
				auto it = lock->register_handler([created, list](tasks::task &t)
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
					created->set_completed(reinterpret_cast<cell>(&t));
				});
				list->push_back(std::make_pair(it, lock));
			}
		}
		return reinterpret_cast<cell>(created.get());
	}

	// native Task:task_all(Task:...);
	static cell AMX_NATIVE_CALL task_all(AMX *amx, cell *params)
	{
		auto created = task::add();
		cell num = params[0] / sizeof(cell);
		auto list = std::shared_ptr<std::vector<std::weak_ptr<tasks::task>>>(new std::vector<std::weak_ptr<tasks::task>>());
		for(cell i = 1; i <= num; i++)
		{
			cell *addr;
			amx_GetAddr(amx, params[i], &addr);
			auto task = reinterpret_cast<tasks::task*>(*addr);
			if(auto lock = tasks::find(task))
			{
				lock->register_handler([created, list](tasks::task &t)
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
						created->set_completed(reinterpret_cast<cell>(&t));
					}
				});
				list->push_back(lock);
			}
		}
		return reinterpret_cast<cell>(created.get());
	}

	// native task_state:task_wait(Task:task);
	static cell AMX_NATIVE_CALL task_wait(AMX *amx, cell *params)
	{
		auto task = reinterpret_cast<tasks::task*>(params[1]);
		
		if(auto lock = tasks::find(task))
		{
			if(!lock->completed() && !lock->faulted())
			{
				amx::object owner;
				auto &info = tasks::get_extra(amx, owner);
				info.awaited_task = lock;

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
};

int RegisterTasksNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
