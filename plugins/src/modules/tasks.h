#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "utils/optional.h"
#include "objects/reset.h"
#include "sdk/amx/amx.h"
#include <queue>

typedef size_t task_id;

class Task
{
	const task_id id;
	cell result = 0;
	bool completed = false;
	std::queue<AMX_RESET> waiting;
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
	void register_callback(AMX_RESET &&reset);
};

namespace TaskPool
{
	Task &CreateNew();
	Task &CreateTickTask(cell ticks);
	Task &CreateTimerTask(cell interval);
	Task *Get(task_id id);
	void OnTick();
	size_t Size();

	void RegisterTicks(cell ticks, AMX_RESET &&reset);
	void RegisterTimer(cell interval, AMX_RESET &&reset);
};

#endif
