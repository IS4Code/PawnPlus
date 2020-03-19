#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "objects/reset.h"
#include "objects/dyn_object.h"
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

		virtual cell set_completed(class task &t) = 0;
		virtual cell set_faulted(class task &t) = 0;

	public:
		virtual ~handler() = default;
	};

	template <class Func>
	class func_handler : public handler
	{
		Func _func;

	public:
		func_handler(Func func) : _func(std::move(func))
		{

		}

		virtual cell set_completed(task &t) override
		{
			return _func(t);
		}

		virtual cell set_faulted(task &t) override
		{
			return _func(t);
		}
	};

	class task
	{
		union{
			cell _error = 0;
			dyn_object _value;
		};
		unsigned char _state = 0;
		bool _keep = false;
		std::list<std::unique_ptr<handler>> handlers;

	public:
		task() noexcept
		{

		}
		task(dyn_object &&result) noexcept : _state(1), _value(std::move(result))
		{

		}
		task(task &&obj) noexcept : _state(obj._state), _keep(obj._keep), handlers(std::move(obj.handlers))
		{
			if(_state == 1)
			{
				_value = std::move(obj._value);
			}else if(_state == 2)
			{
				_error = obj._error;
			}
		}
		void keep(bool keep)
		{
			_keep = keep;
		}
		bool is_keep() const
		{
			return _keep;
		}
		bool faulted() const
		{
			return _state == 2;
		}
		bool completed() const
		{
			return _state == 1;
		}
		dyn_object result() const
		{
			if(completed()) return _value;
			return {};
		}
		cell error() const
		{
			if(faulted()) return _error;
			return 0;
		}
		cell state() const
		{
			return _state;
		}
		bool check_error(AMX *amx) const
		{
			if(completed())
			{
				return true;
			}else if(faulted())
			{
				amx_RaiseError(amx, _error);
				return false;
			}else{
				return false;
			}
		}
		task clone()
		{
			task clone;
			if(completed())
			{
				new (&clone._value) dyn_object();
				clone._value = _value;
			}else{
				clone._error = _error;
			}
			clone._state = _state;
			clone._keep = _keep;
			return clone;
		}
		void reset()
		{
			if(completed())
			{
				_value.~dyn_object();
			}
			_error = 0;
			_state = 0;
			handlers.clear();
		}

		task &operator=(task &&obj) noexcept
		{
			if(this != &obj)
			{
				if(obj.completed())
				{
					if(_state != 1)
					{
						new (&_value) dyn_object();
					}
					_value = std::move(obj._value);
				}else{
					_error = obj._error;
				}
				_state = obj._state;
				_keep = obj._keep;
				handlers = std::move(obj.handlers);
			}
			return *this;
		}

		typedef std::list<std::unique_ptr<handler>>::iterator handler_iterator;

		cell set_completed(dyn_object &&result);
		cell set_faulted(cell error);
		handler_iterator register_reset(amx::reset &&reset);

		template <class Func>
		handler_iterator register_handler(Func func)
		{
			handlers.push_back(std::unique_ptr<func_handler<Func>>(new func_handler<Func>(std::move(func))));
			auto it = handlers.end();
			return --it;
		}

		void unregister_handler(const handler_iterator &it);

		~task()
		{
			if(completed())
			{
				_value.~dyn_object();
				_state = 0;
			}
		}
	};

	struct extra : amx::extra
	{
		cell result = 0;
		std::weak_ptr<task> awaited_task;
		std::weak_ptr<task> bound_task;
		amx::restore_range restore_heap = amx::restore_range::full;
		amx::restore_range restore_stack = amx::restore_range::full;

		extra(AMX *amx) : amx::extra(amx)
		{

		}

		~extra();
	};

	std::shared_ptr<task> add();
	std::shared_ptr<task> add_tick_task(cell ticks);
	std::shared_ptr<task> add_timer_task(cell interval);
	void add_tick_task(const std::shared_ptr<task> &task, cell ticks);
	void add_timer_task(const std::shared_ptr<task> &task, cell interval);
	void add_tick_task_result(const std::shared_ptr<task> &task, cell ticks, dyn_object &&result);
	void add_timer_task_result(const std::shared_ptr<task> &task, cell interval, dyn_object &&result);
	void add_tick_task_error(const std::shared_ptr<task> &task, cell ticks, cell error);
	void add_timer_task_error(const std::shared_ptr<task> &task, cell interval, cell error);
	std::shared_ptr<task> get(task *ptr);
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
