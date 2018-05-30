#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "utils/optional.h"
#include "objects/reset.h"
#include "sdk/amx/amx.h"
#include <queue>
#include <memory>

typedef size_t task_id;

namespace tasks
{
	bool set_result(AMX *amx, cell result);
	cell get_result(AMX *amx);
}

class Task
{
	const task_id id;
	cell result = 0;
	bool completed = false;
	std::queue<amx::reset> waiting;
public:
	Task(task_id id) : id(id)
	{

	}
	task_id Id()
	{
		return id;
	}
	cell Result()
	{
		return result;
	}
	bool Completed()
	{
		return completed;
	}
	void SetCompleted(cell result);
	void register_callback(amx::reset &&reset);
};

namespace TaskPool
{
	std::shared_ptr<Task> CreateNew();
	std::shared_ptr<Task> CreateTickTask(cell ticks);
	std::shared_ptr<Task> CreateTimerTask(cell interval);
	std::shared_ptr<Task> Get(task_id id);
	void OnTick();
	size_t Size();

	void RegisterTicks(cell ticks, amx::reset &&reset);
	void RegisterTimer(cell interval, amx::reset &&reset);
};

#endif
