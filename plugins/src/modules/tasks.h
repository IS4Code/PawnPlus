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
		cell _result = 0;
		bool _completed = false;
		std::list<amx::reset> callbacks;
		std::list<std::function<void(task&)>> handlers;
	public:
		task()
		{

		}
		cell result()
		{
			return _result;
		}
		bool completed()
		{
			return _completed;
		}

		typedef std::list<amx::reset>::iterator reset_iterator;
		typedef std::list<std::function<void(task&)>>::iterator handler_iterator;

		void set_completed(cell result);
		reset_iterator register_reset(amx::reset &&reset);
		handler_iterator register_handler(const std::function<void(task&)> &func);
		handler_iterator register_handler(std::function<void(task&)> &&func);
		void unregister_reset(const reset_iterator &it);
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
