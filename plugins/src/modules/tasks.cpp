#include "tasks.h"
#include "exec.h"

#include "utils/shared_id_set_pool.h"
#include "sdk/amx/amx.h"
#include <utility>
#include <chrono>
#include <list>
#include <unordered_set>
#include <queue>

namespace tasks
{
	class reset_handler : public handler
	{
		amx::reset _reset;
		amx::object owner;

	public:
		reset_handler(amx::reset &&reset) : _reset(std::move(reset))
		{
			owner = _reset.amx.lock();
		}

		virtual cell set_completed(task &t) override;
		virtual cell set_faulted(task &t) override;
	};

	class task_handler : public handler
	{
	protected:
		std::weak_ptr<task> _task;

	public:
		task_handler(std::weak_ptr<task> task) : _task(task)
		{

		}

		virtual cell set_completed(task &t) override;
		virtual cell set_faulted(task &t) override;
	};

	class task_result_handler : public task_handler
	{
		union{
			cell error;
			dyn_object result;
		};
		bool iserror;

	public:
		task_result_handler(std::weak_ptr<task> task, cell error) : task_handler(task), error(error), iserror(true)
		{

		}

		task_result_handler(std::weak_ptr<task> task, dyn_object &&result) : task_handler(task), result(std::move(result)), iserror(false)
		{

		}

		virtual cell set_completed(task &t) override;
		virtual cell set_faulted(task &t) override;

		virtual ~task_result_handler()
		{
			if(!iserror)
			{
				result.~dyn_object();
				iserror = true;
			}
		}
	};

	aux::shared_id_set_pool<task> pool;

	ucell tick_count = 0;
	std::list<std::pair<ucell, std::unique_ptr<handler>>> tick_handlers;
	std::list<std::pair<std::chrono::system_clock::time_point, std::unique_ptr<handler>>> timer_handlers;

	std::queue<std::unique_ptr<handler>> pending_handlers;

	static task &auto_result()
	{
		static task result(dyn_object(1, tags::find_tag(tags::tag_cell)));
		return result;
	}

	tasks::extra &get_extra(AMX *amx, amx::object &owner)
	{
		auto &ctx = amx::get_context(amx, owner);
		return ctx.get_extra<extra>();
	}

	std::shared_ptr<task> add()
	{
		return pool.add();
	}

	cell reset_handler::set_completed(task &t)
	{
		if(auto obj = _reset.amx.lock())
		{
			if(obj->valid())
			{
				AMX *amx = obj->get();

				cell retval;
				_reset.pri = t.state();
				int old_error = amx->error;
				amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &_reset);
				amx->error = old_error;
				return retval;
			}
		}
		return 0;
	}

	cell reset_handler::set_faulted(task &t)
	{
		return set_completed(t);
	}

	cell task_handler::set_completed(task &t)
	{
		if(auto task = _task.lock())
		{
			return task->set_completed(t.result());
		}
		return 0;
	}

	cell task_handler::set_faulted(task &t)
	{
		if(auto task = _task.lock())
		{
			return task->set_faulted(t.error());
		}
		return 0;
	}

	cell task_result_handler::set_completed(task &t)
	{
		if(auto task = _task.lock())
		{
			if(iserror)
			{
				return task->set_faulted(error);
			}else{
				return task->set_completed(dyn_object(result));
			}
		}
		return 0;
	}

	cell task_result_handler::set_faulted(task &t)
	{
		return set_completed(t);
	}

	cell task::set_completed(dyn_object &&result)
	{
		if(_state != 1)
		{
			new (&_value) dyn_object();
		}
		_state = 1;
		_value = result;

		auto handlers = std::move(this->handlers);

		// prevent deletion of the task inside a continuation
		auto handle = pool.get(this);

		cell val = 0;

		for(auto &handler : handlers)
		{
			val = handler->set_completed(*this);
		}

		if(!_keep && _state)
		{
			pool.remove(this);
		}

		return val;
	}

	cell task::set_faulted(cell error)
	{
		if(_state == 1)
		{
			_value.~dyn_object();
		}
		_state = 2;
		_error = error;

		auto handlers = std::move(this->handlers);

		// prevent deletion of the task inside a continuation
		auto handle = pool.get(this);

		cell val = 0;

		for(auto &handler : handlers)
		{
			val = handler->set_faulted(*this);
		}

		if(!_keep && _state)
		{
			pool.remove(this);
		}

		return val;
	}

	task::handler_iterator task::register_reset(amx::reset &&reset)
	{
		handlers.push_back(std::unique_ptr<reset_handler>(new reset_handler(std::move(reset))));
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
	
	std::shared_ptr<task> add_tick_task(cell ticks)
	{
		auto task = add();
		add_tick_task(task, ticks);
		return task;
	}

	std::shared_ptr<task> add_timer_task(cell interval)
	{
		auto task = add();
		add_timer_task(task, interval);
		return task;
	}

	void add_tick_task(std::unique_ptr<handler> &&handler, cell ticks)
	{
		if(ticks > 0)
		{
			ucell time = tick_count + (ucell)ticks;
			insert_sorted(tick_handlers, time, std::move(handler));
		}else if(ticks == 0)
		{
			pending_handlers.push(std::move(handler));
		}
	}

	void add_timer_task(std::unique_ptr<handler> &&handler, cell interval)
	{
		if(interval > 0)
		{
			auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
			insert_sorted(timer_handlers, time, std::move(handler));
		}else if(interval == 0)
		{
			pending_handlers.push(std::move(handler));
		}
	}
	
	void add_tick_task(const std::shared_ptr<task> &task, cell ticks)
	{
		add_tick_task(std::unique_ptr<handler>(new task_handler(task)), ticks);
	}

	void add_timer_task(const std::shared_ptr<task> &task, cell interval)
	{
		add_timer_task(std::unique_ptr<handler>(new task_handler(task)), interval);
	}

	void add_tick_task_result(const std::shared_ptr<task> &task, cell ticks, dyn_object &&result)
	{
		add_tick_task(std::unique_ptr<handler>(new task_result_handler(task, std::move(result))), ticks);
	}

	void add_timer_task_result(const std::shared_ptr<task> &task, cell interval, dyn_object &&result)
	{
		add_timer_task(std::unique_ptr<handler>(new task_result_handler(task, std::move(result))), interval);
	}

	void add_tick_task_error(const std::shared_ptr<task> &task, cell ticks, cell error)
	{
		add_tick_task(std::unique_ptr<handler>(new task_result_handler(task, error)), ticks);
	}

	void add_timer_task_error(const std::shared_ptr<task> &task, cell interval, cell error)
	{
		add_timer_task(std::unique_ptr<handler>(new task_result_handler(task, error)), interval);
	}
	
	void run_pending()
	{
		while(!pending_handlers.empty())
		{
			auto handler = std::move(pending_handlers.front());
			pending_handlers.pop();
			handler->set_completed(auto_result());
		}
	}

	std::shared_ptr<task> get(task *ptr)
	{
		return pool.get(ptr);
	}

	cell get_id(const task *ptr)
	{
		return pool.get_id(ptr);
	}

	bool get_by_id(cell id, task *&ptr)
	{
		return pool.get_by_id(id, ptr);
	}

	bool get_by_id(cell id, std::shared_ptr<task> &ptr)
	{
		return pool.get_by_id(id, ptr);
	}

	void register_tick(cell ticks, amx::reset &&reset)
	{
		if(ticks > 0)
		{
			ucell time = tick_count + (ucell)ticks;
			insert_sorted(tick_handlers, time, std::unique_ptr<handler>(new reset_handler(std::move(reset))));
		}else if(ticks == 0)
		{
			pending_handlers.push(std::unique_ptr<handler>(new reset_handler(std::move(reset))));
		}
	}

	void register_timer(cell interval, amx::reset &&reset)
	{
		if(interval > 0)
		{
			auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
			insert_sorted(timer_handlers, time, std::unique_ptr<handler>(new reset_handler(std::move(reset))));
		}else if(interval == 0)
		{
			pending_handlers.push(std::unique_ptr<handler>(new reset_handler(std::move(reset))));
		}
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
					handler->set_completed(auto_result());
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

				if(pair.first <= now)
				{
					auto handler = std::move(pair.second);
					it = timer_handlers.erase(it);
					handler->set_completed(auto_result());
				}else{
					break;
				}
			}
		}
	}

	void clear()
	{
		tick_handlers.clear();
		timer_handlers.clear();
		if(!pending_handlers.empty())
		{
			std::queue<std::unique_ptr<handler>>().swap(pending_handlers);
		}
		pool.clear();
	}

	extra::~extra()
	{
		if(auto task = bound_task.lock())
		{
			if(task->state() == 0 && !task->is_keep())
			{
				pool.remove(task.get());
			}
		}
	}
}
