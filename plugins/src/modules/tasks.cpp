#include "tasks.h"
#include "hooks.h"

#include "utils/shared_set_pool.h"
#include "sdk/amx/amx.h"
#include <utility>
#include <chrono>
#include <list>
#include <unordered_set>

namespace tasks
{
	aux::shared_set_pool<task> pool;

	std::list<std::pair<ucell, std::shared_ptr<task>>> tickTasks;
	std::list<std::pair<std::chrono::system_clock::time_point, std::shared_ptr<task>>> timerTasks;
	std::list<std::pair<ucell, amx::reset>> tickResets;
	std::list<std::pair<std::chrono::system_clock::time_point, amx::reset>> timerResets;

	tasks::extra &tasks::get_extra(AMX *amx, amx::object &owner)
	{
		auto &ctx = amx::get_context(amx, owner);
		return ctx.get_extra<extra>();
	}

	std::shared_ptr<task> add()
	{
		return pool.add();
	}

	void task::reset_handler::invoke(task &t)
	{
		if(auto obj = _reset.amx.lock())
		{
			AMX *amx = obj->get();

			cell retval;
			_reset.pri = t.result();
			amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &_reset);
		}
	}

	void task::func_handler::invoke(task &t)
	{
		_func(t);
	}

	void task::set_completed(cell result)
	{
		_completed = true;
		_result = result;

		for(auto &handler : handlers)
		{
			handler->invoke(*this);
		}
		handlers.clear();
	}

	task::handler_iterator task::register_reset(amx::reset &&reset)
	{
		handlers.push_back(std::unique_ptr<reset_handler>(new reset_handler(std::move(reset))));
		auto it = handlers.end();
		return --it;
	}

	task::handler_iterator task::register_handler(const std::function<void(task&)> &func)
	{
		handlers.push_back(std::unique_ptr<func_handler>(new func_handler(func)));
		auto it = handlers.end();
		return --it;
	}

	task::handler_iterator task::register_handler(std::function<void(task&)> &&func)
	{
		handlers.push_back(std::unique_ptr<func_handler>(new func_handler(std::move(func))));
		auto it = handlers.end();
		return --it;
	}

	void task::unregister_handler(const handler_iterator &it)
	{
		handlers.erase(it);
	}

	std::shared_ptr<task> add_tick_task(cell ticks)
	{
		auto task = add();
		if(ticks <= 0)
		{
			if(ticks == 0)
			{
				task->set_completed(1);
			}
		} else {
			tickTasks.push_back(std::make_pair(ticks, task));
		}
		return task;
	}

	std::shared_ptr<task> add_timer_task(cell interval)
	{
		auto task = add();
		if(interval <= 0)
		{
			if(interval == 0)
			{
				task->set_completed(1);
			}
		} else {
			auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
			timerTasks.push_back(std::make_pair(time, task));
		}
		return task;
	}

	void register_tick(cell ticks, amx::reset &&reset)
	{
		tickResets.push_back(std::make_pair(ticks, std::move(reset)));
	}

	void register_timer(cell interval, amx::reset &&reset)
	{
		auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
		timerResets.push_back(std::make_pair(time, std::move(reset)));
	}

	bool contains(const task *ptr)
	{
		return pool.contains(ptr);
	}

	bool remove(task *ptr)
	{
		return pool.remove(ptr);
	}

	std::shared_ptr<task> find(task *ptr)
	{
		return pool.get(ptr);
	}

	size_t size()
	{
		return pool.size();
	}

	void tick()
	{
		{
			auto it = tickTasks.begin();
			while(it != tickTasks.end())
			{
				auto &pair = *it;

				pair.first--;
				if(pair.first == 0)
				{
					auto task = pair.second;
					it = tickTasks.erase(it);
					task->set_completed(1);
				} else {
					it++;
				}
			}
		}
		{
			auto it = tickResets.begin();
			while(it != tickResets.end())
			{
				auto &pair = *it;

				pair.first--;
				if(pair.first == 0)
				{
					amx::reset reset = std::move(pair.second);
					it = tickResets.erase(it);

					auto obj = reset.amx.lock();
					if(obj)
					{
						AMX *amx = obj->get();

						cell retval;
						amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
					}
				} else {
					it++;
				}
			}
		}

		auto now = std::chrono::system_clock::now();
		{
			auto it = timerTasks.begin();
			while(it != timerTasks.end())
			{
				auto &pair = *it;

				if(now >= pair.first)
				{
					auto task = pair.second;
					it = timerTasks.erase(it);
					task->set_completed(1);
				} else {
					it++;
				}
			}
		}
		{
			auto it = timerResets.begin();
			while(it != timerResets.end())
			{
				auto &pair = *it;

				if(now >= pair.first)
				{
					amx::reset reset = std::move(pair.second);
					it = timerResets.erase(it);

					auto obj = reset.amx.lock();
					if(obj)
					{
						AMX *amx = obj->get();

						cell retval;
						amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
					}
				} else {
					it++;
				}
			}
		}
	}
}
