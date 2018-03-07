#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include <vector>
#include <unordered_set>

#include "sdk/amx/amx.h"

#include "utils/optional.h"
#include "reset.h"

typedef size_t task_id;

class Task
{
	const task_id id;
	cell result;
	bool completed;
	std::vector<AMX_RESET> waiting;
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
	void Register(AMX_RESET &&reset);
};

namespace TaskPool
{
	Task &CreateNew();
	Task &CreateTickTask(cell ticks);
	Task &CreateTimerTask(cell interval);
	Task *Get(task_id id);
	void OnTick();
};

#endif
