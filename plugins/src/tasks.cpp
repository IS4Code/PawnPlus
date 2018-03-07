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
	Task &task = pool.emplace_back(id);
	return task;
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

		AMX_RESET *old = nullptr;
		if(Context::IsPresent(amx))
		{
			old = new AMX_RESET(amx);
		}

		Context::Push(amx);
		reset.restore();
		amx->pri = result;
		int ret = amx_ExecOrig(amx, &retval, AMX_EXEC_CONT);
		if(ret == AMX_ERR_SLEEP)
		{
			auto &ctx = Context::Get(amx);
			if(ctx.awaiting_task != -1)
			{
				Task *task = TaskPool::Get(ctx.awaiting_task);
				if(task != nullptr)
				{
					task->Register(AMX_RESET(amx));
					amx->error = ret = AMX_ERR_NONE;
				}
			}
		}
		Context::Pop(amx);

		if(old != nullptr)
		{
			old->restore();
			delete old;
		}
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
