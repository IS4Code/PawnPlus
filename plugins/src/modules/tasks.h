#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"
#include <list>
#include <memory>
#include <functional>

namespace tasks
{
	class handler
	{
		friend class task;
		friend void tick();
		friend void run_pending();

		virtual void set_completed(class task &t) = 0;
		virtual void set_faulted(class task &t) = 0;

	public:
		virtual ~handler() = default;
	};

	class task
	{
		cell _value = 0;
		unsigned char _state = 0;
		bool _keep = false;
		std::list<std::unique_ptr<handler>> handlers;

	public:
		task()
		{

		}
		task(cell result) : _state(1), _value(result)
		{

		}
		void keep(bool keep)
		{
			_keep = keep;
		}
		bool faulted() const
		{
			return _state == 2;
		}
		bool completed() const
		{
			return _state == 1;
		}
		cell result() const
		{
			if(completed()) return _value;
			return 0;
		}
		cell error() const
		{
			if(faulted()) return _value;
			return 0;
		}
		cell state() const
		{
			return _state;
		}
		bool result_or_error(AMX *amx, cell &result) const
		{
			if(completed())
			{
				result = _value;
				return true;
			}else if(faulted())
			{
				amx_RaiseError(amx, _value);
				return false;
			}else{
				return false;
			}
		}
		void reset()
		{
			_value = 0;
			_state = 0;
			handlers.clear();
		}

		typedef std::list<std::unique_ptr<handler>>::iterator handler_iterator;

		void set_completed(cell result);
		void set_faulted(cell error);
		handler_iterator register_reset(amx::reset &&reset);
		handler_iterator register_handler(const std::function<void(task&)> &func);
		handler_iterator register_handler(std::function<void(task&)> &&func);
		void unregister_handler(const handler_iterator &it);
	};

	struct extra : amx::extra
	{
		cell result = 0;
		std::weak_ptr<task> awaited_task;
		std::weak_ptr<task> bound_task;

		extra(AMX *amx) : amx::extra(amx)
		{

		}
	};

	std::shared_ptr<task> add();
	std::shared_ptr<task> add_tick_task(cell ticks);
	std::shared_ptr<task> add_timer_task(cell interval);
	cell get_id(const task *ptr);
	bool get_by_id(cell id, task *&ptr);
	bool get_by_id(cell id, std::shared_ptr<task> &ptr);
	void register_tick(cell ticks, amx::reset &&reset);
	void register_timer(cell interval, amx::reset &&reset);
	bool contains(const task *ptr);
	bool remove(task *ptr);
	void clear();
	void run_pending();

	std::shared_ptr<task> find(task *ptr);

	void tick();
	size_t size();

	extra &get_extra(AMX *amx, amx::object &owner);
}

#endif
