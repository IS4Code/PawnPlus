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

	ucell tick_count = 0;
	std::list<std::pair<ucell, std::unique_ptr<task::handler>>> tick_handlers;
	std::list<std::pair<std::chrono::system_clock::time_point, std::unique_ptr<task::handler>>> timer_handlers;

	tasks::extra &get_extra(AMX *amx, amx::object &owner)
	{
		auto &ctx = amx::get_context(amx, owner);
		return ctx.get_extra<extra>();
	}

	std::shared_ptr<task> task::add()
	{
		return pool.add();
	}

	void task::reset_handler::invoke(task &t)
	{
		if(auto obj = _reset.amx.lock())
		{
			if(obj->valid())
			{
				AMX *amx = obj->get();

				cell retval;
				_reset.pri = t.result();
				amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &_reset);
			}
		}
	}

	void task::func_handler::invoke(task &t)
	{
		_func(t);
	}

	void task::task_handler::invoke(task &t)
	{
		if(auto task = _task.lock())
		{
			task->set_completed(t.result());
		}
	}

	void task::set_completed(cell result)
	{
		_completed = true;
		_result = result;

		auto handlers = std::move(this->handlers);

		for(auto &handler : handlers)
		{
			handler->invoke(*this);
		}

		if(!_keep)
		{
			pool.remove(this);
		}
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

	template <class Ord, class Obj>
	typename std::list<std::pair<Ord, Obj>>::iterator insert_sorted(std::list<std::pair<Ord, Obj>> &list, const Ord &ord, Obj &&obj)
	{
		for(auto it = list.begin();; it++)
		{
			if(it == list.end() || it->first > ord)
			{
				return list.insert(it, std::make_pair(ord, std::forward<Obj>(obj)));
			}
		}
	}

	std::shared_ptr<task> task::add_tick_task(cell ticks)
	{
		if(ticks == 0)
		{
			return nullptr;
		}
		auto task = add();
		if(ticks > 0)
		{
			ucell time = tick_count + (ucell)ticks;
			insert_sorted(tick_handlers, time, std::unique_ptr<handler>(new task_handler(task)));
		}
		return task;
	}

	std::shared_ptr<task> task::add_timer_task(cell interval)
	{
		if(interval == 0)
		{
			return nullptr;
		}
		auto task = add();
		if(interval > 0)
		{
			auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
			insert_sorted(timer_handlers, time, std::unique_ptr<handler>(new task_handler(task)));
		}
		return task;
	}

	void task::register_tick(cell ticks, amx::reset &&reset)
	{
		ucell time = tick_count + (ucell)ticks;
		insert_sorted(tick_handlers, time, std::unique_ptr<handler>(new reset_handler(std::move(reset))));
	}

	void task::register_timer(cell interval, amx::reset &&reset)
	{
		auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
		insert_sorted(timer_handlers, time, std::unique_ptr<handler>(new reset_handler(std::move(reset))));
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
		task auto_result(1);

		tick_count++;
		{
			auto it = tick_handlers.begin();
			while(it != tick_handlers.end())
			{
				auto &pair = *it;

				if(pair.first <= tick_count)
				{
					auto handler = std::move(pair.second);
					it = tick_handlers.erase(it);
					handler->invoke(auto_result);
				}else{
					break;
				}
			}
		}
		if(tick_handlers.empty())
		{
			tick_count = 0;
		}

		auto now = std::chrono::system_clock::now();
		{
			auto it = timer_handlers.begin();
			while(it != timer_handlers.end())
			{
				auto &pair = *it;

				if(now >= pair.first)
				{
					auto handler = std::move(pair.second);
					it = timer_handlers.erase(it);
					handler->invoke(auto_result);
				}else{
					it++;
				}
			}
		}
	}
}