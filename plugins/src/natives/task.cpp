#include "natives.h"
#include "context.h"
#include "exec.h"
#include "modules/tasks.h"
#include "modules/variants.h"
#include <algorithm>

using namespace tasks;

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native task_set_result(Task:task, result, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL task_set_result(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			task->set_completed(Factory(amx, params[Indices]...));
			return 1;
		}
		return 0;
	}

	// native task_get_result(Task:task, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL task_get_result(AMX *amx, cell *params)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			if(task->check_error(amx))
			{
				return Factory(amx, task->result(), params[Indices]...);
			}
			return 0;
		}
		return 0;
	}

	// native task_set_result_ms(Task:task, result, interval, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL task_set_result_ms(AMX *amx, cell *params)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			tasks::add_timer_task_result(task, params[3], Factory(amx, params[Indices]...));
			return 1;
		}
		return 0;
	}

	// native task_set_result_ticks(Task:task, result, interval, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL task_set_result_ticks(AMX *amx, cell *params)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			tasks::add_tick_task_result(task, params[3], Factory(amx, params[Indices]...));
			return 1;
		}
		return 0;
	}
};

namespace Natives
{
	// native Task:wait_ticks(ticks);
	AMX_DEFINE_NATIVE(wait_ticks, 1)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		if(params[1] < 0)
		{
			return SleepReturnWaitInf;
		}else if(params[1] & ~SleepReturnValueMask)
		{
			amx::object owner;
			auto &info = tasks::get_extra(amx, owner);
			info.awaited_task = tasks::add_tick_task(params[1]);
			return SleepReturnAwait;
		}else{
			return SleepReturnWaitTicks | (SleepReturnValueMask & params[1]);
		}
	}

	// native Task:wait_ms(interval);
	AMX_DEFINE_NATIVE(wait_ms, 1)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		if(params[1] < 0)
		{
			return SleepReturnWaitInf;
		}else if(params[1] & ~SleepReturnValueMask)
		{
			amx::object owner;
			auto &info = tasks::get_extra(amx, owner);
			info.awaited_task = tasks::add_timer_task(params[1]);
			return SleepReturnAwait;
		}else{
			return SleepReturnWaitMs | (SleepReturnValueMask & params[1]);
		}
	}

	// native Task:task_new();
	AMX_DEFINE_NATIVE(task_new, 0)
	{
		return tasks::get_id(tasks::add().get());
	}

	// native bool:task_delete(Task:task);
	AMX_DEFINE_NATIVE(task_delete, 1)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return tasks::remove(task);
		}
		return 0;
	}

	// native Task:task_keep(Task:task, bool:keep=true);
	AMX_DEFINE_NATIVE(task_keep, 1)
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
	AMX_DEFINE_NATIVE(task_valid, 1)
	{
		task *task;
		return tasks::get_by_id(params[1], task);
	}

	// native task_set_result(Task:task, AnyTag:result, TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result, 3)
	{
		return value_at<2, 3>::task_set_result<dyn_func>(amx, params);
	}

	// native task_set_result_arr(Task:task, const AnyTag:result[], size=sizeof(result), TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result_arr, 4)
	{
		return value_at<2, 3, 4>::task_set_result<dyn_func_arr>(amx, params);
	}

	// native task_set_result_str(Task:task, const result[]);
	AMX_DEFINE_NATIVE(task_set_result_str, 2)
	{
		return value_at<2>::task_set_result<dyn_func_str>(amx, params);
	}

	// native task_set_result_var(Task:task, ConstVariantTag:result);
	AMX_DEFINE_NATIVE(task_set_result_var, 2)
	{
		return value_at<2>::task_set_result<dyn_func_var>(amx, params);
	}

	// native task_get_result(Task:task, offset=0);
	AMX_DEFINE_NATIVE(task_get_result, 2)
	{
		return value_at<2>::task_get_result<dyn_func>(amx, params);
	}

	// native task_get_result_arr(Task:task, AnyTag:result[], size=sizeof(result));
	AMX_DEFINE_NATIVE(task_get_result_arr, 3)
	{
		return value_at<2, 3>::task_get_result<dyn_func_arr>(amx, params);
	}

	// native Variant:task_get_result_var(Task:task);
	AMX_DEFINE_NATIVE(task_get_result_var, 1)
	{
		return value_at<>::task_get_result<dyn_func_var>(amx, params);
	}

	// native bool:task_get_result_safe(Task:task, &AnyTag:result, offset=0, TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_get_result_safe, 4)
	{
		return value_at<2, 3, 4>::task_get_result<dyn_func>(amx, params);
	}

	// native task_get_result_arr_safe(Task:task, AnyTag:result[], size=sizeof(result), TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_get_result_arr_safe, 4)
	{
		return value_at<2, 3, 4>::task_get_result<dyn_func_arr>(amx, params);
	}

	// native task_reset(Task:task);
	AMX_DEFINE_NATIVE(task_reset, 1)
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
	AMX_DEFINE_NATIVE(task_set_error, 2)
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
	AMX_DEFINE_NATIVE(task_get_error, 1)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->error();
		}
		return 0;
	}

	// native bool:task_completed(Task:task);
	AMX_DEFINE_NATIVE(task_completed, 1)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->completed();
		}
		return 0;
	}

	// native bool:task_faulted(Task:task);
	AMX_DEFINE_NATIVE(task_faulted, 1)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->faulted();
		}
		return 0;
	}

	// native task_state:task_state(Task:task);
	AMX_DEFINE_NATIVE(task_state, 1)
	{
		task *task;
		if(tasks::get_by_id(params[1], task))
		{
			return task->state();
		}
		return 0;
	}

	// native Task:task_ticks(ticks);
	AMX_DEFINE_NATIVE(task_ticks, 1)
	{
		return tasks::get_id(add_tick_task(params[1]).get());
	}

	// native Task:task_ms(interval);
	AMX_DEFINE_NATIVE(task_ms, 1)
	{
		return tasks::get_id(add_timer_task(params[1]).get());
	}

	// native Task:task_any(Task:...);
	AMX_DEFINE_NATIVE(task_any, 0)
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
					created->set_completed(dyn_object(tasks::get_id(&t), tags::find_tag(tags::tag_task)));
				});
				list->push_back(std::make_pair(it, task));
			}
		}
		return tasks::get_id(created.get());
	}

	// native Task:task_all(Task:...);
	AMX_DEFINE_NATIVE(task_all, 0)
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
						created->set_completed(dyn_object(tasks::get_id(&t), tags::find_tag(tags::tag_task)));
					}
				});
				list->push_back(task);
			}
		}
		return tasks::get_id(created.get());
	}

	// native task_state:task_wait(Task:task);
	AMX_DEFINE_NATIVE(task_wait, 1)
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

	// native task_yield(AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(task_yield, 1)
	{
		amx::object owner;
		auto &info = tasks::get_extra(amx, owner);
		info.result = params[1];
		if(auto task = info.bound_task.lock())
		{
			task->set_completed(dyn_object(amx, params[1], optparam(2, 0)));
			return 1;
		}
		return 0;
	}

	// native bool:task_bind(Task:task, const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(task_bind, 3)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			char *fname;
			amx_StrParam(amx, params[2], fname);

			char *format;
			amx_StrParam(amx, params[3], format);

			if(fname == nullptr) return 0;

			if(format == nullptr) format = "";
			int numargs = std::strlen(format);
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

				cell result = 1;
				amx_ExecContext(amx, &result, pubindex, true, &reset);
				amx->error = AMX_ERR_NONE;
				return result;
			}
		}
		return 0;
	}

	// native task_set_result_ms(Task:task, AnyTag:result, interval, TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result_ms, 4)
	{
		return value_at<2, 4>::task_set_result_ms<dyn_func>(amx, params);
	}

	// native task_set_result_ms_arr(Task:task, const AnyTag:result[], interval, size=sizeof(result), TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result_ms_arr, 5)
	{
		return value_at<2, 4, 5>::task_set_result_ms<dyn_func_arr>(amx, params);
	}

	// native task_set_result_ms_str(Task:task, const result[], interval);
	AMX_DEFINE_NATIVE(task_set_result_ms_str, 3)
	{
		return value_at<2>::task_set_result_ms<dyn_func_str>(amx, params);
	}

	// native task_set_result_ms_var(Task:task, ConstVariantTag:result, interval);
	AMX_DEFINE_NATIVE(task_set_result_ms_var, 3)
	{
		return value_at<2>::task_set_result_ms<dyn_func_var>(amx, params);
	}

	// native task_set_result_ticks(Task:task, AnyTag:result, interval, TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result_ticks, 4)
	{
		return value_at<2, 4>::task_set_result_ticks<dyn_func>(amx, params);
	}

	// native task_set_result_ticks_arr(Task:task, const AnyTag:result[], interval, size=sizeof(result), TagTag:tag_id=tagof(result));
	AMX_DEFINE_NATIVE(task_set_result_ticks_arr, 5)
	{
		return value_at<2, 4, 5>::task_set_result_ticks<dyn_func_arr>(amx, params);
	}

	// native task_set_result_ticks_str(Task:task, const result[], interval);
	AMX_DEFINE_NATIVE(task_set_result_ticks_str, 3)
	{
		return value_at<2>::task_set_result_ticks<dyn_func_str>(amx, params);
	}

	// native task_set_result_ticks_var(Task:task, ConstVariantTag:result, interval);
	AMX_DEFINE_NATIVE(task_set_result_ticks_var, 3)
	{
		return value_at<2>::task_set_result_ticks<dyn_func_var>(amx, params);
	}

	// native task_set_error_ms(Task:task, amx_err : error, interval);
	AMX_DEFINE_NATIVE(task_set_error_ms, 3)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			tasks::add_timer_task_error(task, params[3], params[2]);
			return 1;
		}
		return 0;
	}

	// native task_set_error_ticks(Task:task, amx_err : error, ticks);
	AMX_DEFINE_NATIVE(task_set_error_ticks, 3)
	{
		std::shared_ptr<task> task;
		if(tasks::get_by_id(params[1], task))
		{
			tasks::add_tick_task_error(task, params[3], params[2]);
			return 1;
		}
		return 0;
	}

	// native task_config(task_restore:heap=task_restore_full, task_restore:stack=task_restore_full);
	AMX_DEFINE_NATIVE(task_config, 0)
	{
		amx::object owner;
		auto &info = tasks::get_extra(amx, owner);
		info.restore_heap = static_cast<amx::restore_range>(optparam(1, 3));
		info.restore_stack = static_cast<amx::restore_range>(optparam(2, 3));
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
	AMX_DECLARE_NATIVE(task_set_result_arr),
	AMX_DECLARE_NATIVE(task_set_result_str),
	AMX_DECLARE_NATIVE(task_set_result_var),
	AMX_DECLARE_NATIVE(task_get_result),
	AMX_DECLARE_NATIVE(task_get_result_arr),
	AMX_DECLARE_NATIVE(task_get_result_var),
	AMX_DECLARE_NATIVE(task_get_result_safe),
	AMX_DECLARE_NATIVE(task_get_result_arr_safe),
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
	AMX_DECLARE_NATIVE(task_set_result_ms),
	AMX_DECLARE_NATIVE(task_set_result_ms_arr),
	AMX_DECLARE_NATIVE(task_set_result_ms_str),
	AMX_DECLARE_NATIVE(task_set_result_ms_var),
	AMX_DECLARE_NATIVE(task_set_result_ticks),
	AMX_DECLARE_NATIVE(task_set_result_ticks_arr),
	AMX_DECLARE_NATIVE(task_set_result_ticks_str),
	AMX_DECLARE_NATIVE(task_set_result_ticks_var),
	AMX_DECLARE_NATIVE(task_set_error_ms),
	AMX_DECLARE_NATIVE(task_set_error_ticks),
	AMX_DECLARE_NATIVE(task_config),
};

int RegisterTasksNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
