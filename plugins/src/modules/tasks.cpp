#include "tasks.h"
#include "hooks.h"

#include "fixes/linux.h"
#include "utils/shared_linear_pool.h"
#include "utils/set_pool.h"
#include "sdk/amx/amx.h"
#include <utility>
#include <chrono>
#include <vector>
#include <unordered_set>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

aux::shared_linear_pool<Task> pool;
aux::set_pool<std::pair<ucell, std::shared_ptr<Task>>> tickTasks;
aux::set_pool<std::pair<std::chrono::system_clock::time_point, std::shared_ptr<Task>>> timerTasks;
aux::set_pool<std::pair<ucell, AMX_RESET>> tickResets;
aux::set_pool<std::pair<std::chrono::system_clock::time_point, AMX_RESET>> timerResets;

std::shared_ptr<Task> TaskPool::CreateNew()
{
	size_t id = pool.size();
	pool.add(Task(id));
	return pool.at(id);
}

std::shared_ptr<Task> TaskPool::Get(task_id id)
{
	return pool.at(id);
}

void Task::SetCompleted(cell result)
{
	this->completed = true;
	this->result = result;
	while(waiting.size() > 0)
	{
		auto &reset = waiting.front();
		AMX *amx = reset.amx;
		if(!Context::IsValid(amx)) continue;

		cell retval;

		reset.pri = result;
		amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
		waiting.pop();
	}
}

void Task::register_callback(AMX_RESET &&reset)
{
	waiting.push(std::move(reset));
}

std::shared_ptr<Task> TaskPool::CreateTickTask(cell ticks)
{
	auto task = CreateNew();
	if(ticks == 0)
	{
		task->SetCompleted(task->Id());
	}else{
		tickTasks.add(std::make_pair(ticks, task));
	}
	return task;
}

std::shared_ptr<Task> TaskPool::CreateTimerTask(cell interval)
{
	auto task = CreateNew();
	if(interval <= 0)
	{
		task->SetCompleted(task->Id());
	}else{
		auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
		timerTasks.add(std::make_pair(time, task));
	}
	return task;
}

void TaskPool::RegisterTicks(cell ticks, AMX_RESET &&reset)
{
	tickResets.add(std::make_pair(ticks, std::move(reset)));
}

void TaskPool::RegisterTimer(cell interval, AMX_RESET &&reset)
{
	auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
	timerResets.add(std::make_pair(time, std::move(reset)));
}

size_t TaskPool::Size()
{
	return pool.size();
}

void TaskPool::OnTick()
{
	{
		auto it = tickTasks.begin();
		while(it != tickTasks.end())
		{
			auto obj = *it;

			obj->first--;
			if(obj->first == 0)
			{
				auto task = obj->second;
				tickTasks.erase(it);
				task->SetCompleted(task->Id());
				it = tickTasks.begin();
			}else{
				it++;
			}
		}
	}
	{
		auto it = tickResets.begin();
		while(it != tickResets.end())
		{
			auto obj = *it;

			obj->first--;
			if(obj->first == 0)
			{
				AMX_RESET reset = std::move(obj->second);
				tickResets.erase(it);
				AMX *amx = reset.amx;
				if(!Context::IsValid(amx)) continue;

				cell retval;
				amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);

				it = tickResets.begin();
			}else{
				it++;
			}
		}
	}

	auto now = std::chrono::system_clock::now();
	{
		auto it = timerTasks.begin();
		while(it != timerTasks.end())
		{
			auto obj = *it;

			if(now >= obj->first)
			{
				auto task = obj->second;
				timerTasks.erase(it);
				task->SetCompleted(task->Id());
				it = timerTasks.begin();
			}else{
				it++;
			}
		}
	}
	{
		auto it = timerResets.begin();
		while(it != timerResets.end())
		{
			auto obj = *it;

			if(now >= obj->first)
			{
				AMX_RESET reset = std::move(obj->second);
				timerResets.erase(it);
				AMX *amx = reset.amx;
				if(!Context::IsValid(amx)) continue;

				cell retval;
				amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);

				it = timerResets.begin();
			}else{
				it++;
			}
		}
	}
}
