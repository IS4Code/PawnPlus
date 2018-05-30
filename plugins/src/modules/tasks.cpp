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

aux::shared_linear_pool<Task> pool;
aux::set_pool<std::pair<ucell, std::shared_ptr<Task>>> tickTasks;
aux::set_pool<std::pair<std::chrono::system_clock::time_point, std::shared_ptr<Task>>> timerTasks;
aux::set_pool<std::pair<ucell, amx::reset>> tickResets;
aux::set_pool<std::pair<std::chrono::system_clock::time_point, amx::reset>> timerResets;

struct task_extra : amx::extra
{
	size_t task_object = -1;
	cell result = 0;

	task_extra(AMX *amx) : amx::extra(amx)
	{

	}
};

bool tasks::set_result(AMX *amx, cell result)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &extra = ctx.get_extra<task_extra>();
	extra.result = result;
	if(extra.task_object != -1)
	{
		TaskPool::Get(extra.task_object)->SetCompleted(result);
		return true;
	}
	return false;
}

cell tasks::get_result(AMX *amx)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	return ctx.get_extra<task_extra>().result;
}

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
		auto obj = reset.amx.lock();
		if(obj)
		{
			AMX *amx = obj->get();

			cell retval;
			reset.pri = result;
			amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
		}
		waiting.pop();
	}
}

void Task::register_callback(amx::reset &&reset)
{
	waiting.push(std::move(reset));
}

std::shared_ptr<Task> TaskPool::CreateTickTask(cell ticks)
{
	auto task = CreateNew();
	if(ticks <= 0)
	{
		if(ticks == 0)
		{
			task->SetCompleted(1);
		}
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
		if(interval == 0)
		{
			task->SetCompleted(1);
		}
	}else{
		auto time = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(interval));
		timerTasks.add(std::make_pair(time, task));
	}
	return task;
}

void TaskPool::RegisterTicks(cell ticks, amx::reset &&reset)
{
	tickResets.add(std::make_pair(ticks, std::move(reset)));
}

void TaskPool::RegisterTimer(cell interval, amx::reset &&reset)
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
			auto pair = *it;

			pair->first--;
			if(pair->first == 0)
			{
				auto task = pair->second;
				it = tickTasks.erase(it);
				task->SetCompleted(1);
			}else{
				it++;
			}
		}
	}
	{
		auto it = tickResets.begin();
		while(it != tickResets.end())
		{
			auto pair = *it;

			pair->first--;
			if(pair->first == 0)
			{
				amx::reset reset = std::move(pair->second);
				it = tickResets.erase(it);

				auto obj = reset.amx.lock();
				if(obj)
				{
					AMX *amx = obj->get();

					cell retval;
					amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
				}
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
			auto pair = *it;

			if(now >= pair->first)
			{
				auto task = pair->second;
				it = timerTasks.erase(it);
				task->SetCompleted(1);
			}else{
				it++;
			}
		}
	}
	{
		auto it = timerResets.begin();
		while(it != timerResets.end())
		{
			auto pair = *it;

			if(now >= pair->first)
			{
				amx::reset reset = std::move(pair->second);
				it = timerResets.erase(it);

				auto obj = reset.amx.lock();
				if(obj)
				{
					AMX *amx = obj->get();

					cell retval;
					amx_ExecContext(amx, &retval, AMX_EXEC_CONT, true, &reset);
				}
			}else{
				it++;
			}
		}
	}
}
