#include "../natives.h"
#include "../tasks.h"
#include "../context.h"

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

namespace Natives
{
	// native task:wait_ticks(ticks);
	static cell AMX_NATIVE_CALL wait_ticks(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &ctx = Context::Get(amx);
		ctx.pause_reason = PauseReason::Await;
		if(params[1] < 0)
		{
			ctx.awaiting_task = TaskPool::CreateNew().Id();
		}else{
			ctx.awaiting_task = TaskPool::CreateTickTask(params[1]).Id();
		}
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 0;
	}

	// native task:wait_ms(interval);
	static cell AMX_NATIVE_CALL wait_ms(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &ctx = Context::Get(amx);
		ctx.pause_reason = PauseReason::Await;
		if(params[1] < 0)
		{
			ctx.awaiting_task = TaskPool::CreateNew().Id();
		}else{
			ctx.awaiting_task = TaskPool::CreateTimerTask(params[1]).Id();
		}
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 0;
	}

	// native task:task_new();
	static cell AMX_NATIVE_CALL task_new(AMX *amx, cell *params)
	{
		return TaskPool::CreateNew().Id();
	}

	// native task_set_result(task:task, {_,Float}:result);
	static cell AMX_NATIVE_CALL task_set_result(AMX *amx, cell *params)
	{
		auto task = TaskPool::Get(params[1]);
		if(task != nullptr)
		{
			task->SetCompleted(params[2]);
			return 1;
		}
		return 0;
	}

	// native task_get_result(task:task);
	static cell AMX_NATIVE_CALL task_get_result(AMX *amx, cell *params)
	{
		auto task = TaskPool::Get(params[1]);
		if(task != nullptr)
		{
			return task->Result();
		}
		return 0;
	}

	// native task:task_ticks(ticks);
	static cell AMX_NATIVE_CALL task_ticks(AMX *amx, cell *params)
	{
		if(params[1] <= 0)
		{
			auto &task = TaskPool::CreateNew();
			if(params[1] == 0)
			{
				task.SetCompleted(0);
			}
			return task.Id();
		}else{
			auto &task = TaskPool::CreateTickTask(params[1]);
			return task.Id();
		}
	}

	// native task:task_ms(interval);
	static cell AMX_NATIVE_CALL task_ms(AMX *amx, cell *params)
	{
		if(params[1] <= 0)
		{
			auto &task = TaskPool::CreateNew();
			if(params[1] == 0)
			{
				task.SetCompleted(0);
			}
			return task.Id();
		}else{
			auto &task = TaskPool::CreateTimerTask(params[1]);
			return task.Id();
		}
	}

	// native task_await(task:task);
	static cell AMX_NATIVE_CALL task_await(AMX *amx, cell *params)
	{
		task_id id = static_cast<task_id>(params[1]);
		auto task = TaskPool::Get(id);
		if(task != nullptr)
		{
			if(!task->Completed())
			{
				auto &ctx = Context::Get(amx);
				ctx.pause_reason = PauseReason::Await;
				ctx.awaiting_task = id;
				amx_RaiseError(amx, AMX_ERR_SLEEP);
				return 0;
			}else{
				return task->Result();
			}
		}
		return 0;
	}

	// native task_yield({_,Float}:value);
	static cell AMX_NATIVE_CALL task_yield(AMX *amx, cell *params)
	{
		auto &ctx = Context::Get(amx);
		ctx.result = params[1];
		if(ctx.task_object != -1)
		{
			TaskPool::Get(ctx.task_object)->SetCompleted(ctx.result);
			return 1;
		}
		return 0;
	}

	// native thread_detach(bool:natives_protect=true);
	static cell AMX_NATIVE_CALL thread_detach(AMX *amx, cell *params)
	{
		auto &ctx = Context::Get(amx);
		ctx.natives_protect = static_cast<bool>(params[1]);
		ctx.pause_reason = PauseReason::Detach;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 1;
	}

	// native thread_attach();
	static cell AMX_NATIVE_CALL thread_attach(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 1;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(wait_ticks),
	AMX_DECLARE_NATIVE(wait_ms),
	AMX_DECLARE_NATIVE(task_new),
	AMX_DECLARE_NATIVE(task_set_result),
	AMX_DECLARE_NATIVE(task_get_result),
	AMX_DECLARE_NATIVE(task_ticks),
	AMX_DECLARE_NATIVE(task_ms),
	AMX_DECLARE_NATIVE(task_await),
	AMX_DECLARE_NATIVE(task_yield),
	AMX_DECLARE_NATIVE(thread_detach),
	AMX_DECLARE_NATIVE(thread_attach),
};

int RegisterTasksNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
