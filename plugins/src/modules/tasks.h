#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"
#include <list>
#include <memory>
#include <functional>

namespace tasks
{
	class task
	{
	public:
		class handler
		{
			friend class task;

			virtual void invoke(task &t) = 0;

		public:
			virtual ~handler() = default;
		};

	private:
		cell _result = 0;
		bool _completed = false;
		bool _keep = false;
		std::list<std::unique_ptr<handler>> handlers;

	public:
		task()
		{

		}
		void keep()
		{
			_keep = true;
		}
		cell result()
		{
			return _result;
		}
		bool completed()
		{
			return _completed;
		}

		typedef std::list<std::unique_ptr<handler>>::iterator handler_iterator;

		void set_completed(cell result);
		handler_iterator register_reset(amx::reset &&reset);
		handler_iterator register_handler(const std::function<void(task&)> &func);
		handler_iterator register_handler(std::function<void(task&)> &&func);
		void unregister_handler(const handler_iterator &it);

	private:
		struct reset_handler : public handler
		{
			amx::reset _reset;

			reset_handler(amx::reset &&reset) : _reset(std::move(reset))
			{

			}

			virtual void invoke(task &t) override;
		};

		struct func_handler : public handler
		{
			std::function<void(task&)> _func;

			func_handler(const std::function<void(task&)> &func) : _func(func)
			{

			}

			func_handler(std::function<void(task&)> &&func) : _func(std::move(func))
			{

			}

			virtual void invoke(task &t) override;
		};
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

	bool contains(const task *ptr);
	bool remove(task *ptr);

	std::shared_ptr<task> find(task *ptr);

	void tick();
	size_t size();

	void register_tick(cell ticks, amx::reset &&reset);
	void register_timer(cell interval, amx::reset &&reset);

	extra &get_extra(AMX *amx, amx::object &owner);
}

#endif
