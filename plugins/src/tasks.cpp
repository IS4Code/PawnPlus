#include "tasks.h"
#include "hooks.h"

#include "sdk/amx/amx.h"
#include <utility>
#include <chrono>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

std::vector<Task> pool;
std::vector<std::pair<ucell, task_id>> tickTasks;
std::vector<std::pair<std::chrono::system_clock::time_point, task_id>> timerTasks;

Task &TaskPool::CreateNew()
{
	size_t id = pool.size();
	pool.emplace_back(id);
	return pool[id];
}

Task *TaskPool::Get(task_id id)
{
	if(0 <= id && id < pool.size())
	{
		return &pool[id];
	}else{
		return nullptr;
	}
}

void Task::SetCompleted(cell result)
{
	this->completed = true;
	this->result = result;
	for (auto &reset : waiting)
	{
		AMX *amx = reset.amx;
		if(!Context::IsValid(amx)) continue;

		cell retval;

		reset.pri = result;
		amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
	}
	waiting.clear();
}

void Task::Register(AMX_RESET &&reset)
{
	waiting.push_back(std::move(reset));
}

Task &TaskPool::CreateTickTask(cell ticks)
{
	Task &task = CreateNew();
	if (ticks == 0)
	{
		task.SetCompleted(task.Id());
	} else {
		tickTasks.push_back(std::make_pair(ticks, task.Id()));
	}
	return task;
}

Task &TaskPool::CreateTimerTask(cell interval)
{
	Task &task = CreateNew();
	if (interval <= 0)
	{
		task.SetCompleted(task.Id());
	} else {
		auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
		timerTasks.push_back(std::make_pair(time, task.Id()));
	}
	return task;
}

void TaskPool::OnTick()
{
	{
		auto it = tickTasks.begin();
		while (it != tickTasks.end())
		{
			it->first--;
			if (it->first == 0)
			{
				auto id = it->second;
				tickTasks.erase(it);
				Get(id)->SetCompleted(id);
				it = tickTasks.begin();
			} else {
				it++;
			}
		}
	}

	{
		auto now = std::chrono::system_clock::now();
		auto it = timerTasks.begin();
		while (it != timerTasks.end())
		{
			if (now >= it->first)
			{
				auto id = it->second;
				timerTasks.erase(it);
				Get(id)->SetCompleted(id);
				it = timerTasks.begin();
			} else {
				it++;
			}
		}
	}
}
