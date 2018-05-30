#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"
#include <queue>
#include <memory>

namespace tasks
{
	class task
	{
		cell _result = 0;
		bool _completed = false;
		std::queue<amx::reset> waiting;
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
		void set_completed(cell result);
		void register_callback(amx::reset &&reset);
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
