#include "threads.h"
#include "hooks.h"
#include "reset.h"
#include "tasks.h"
#include "utils/thread.h"
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <tuple>

class thread_state;
extern std::unordered_multimap<AMX*, thread_state*> running_threads;

class thread_state
{
	std::mutex mutex;
	AMX_RESET reset;
	aux::thread thread;
	AMX_CALLBACK orig_callback;
	AMX *amx;
	bool safe;
	std::tuple<int, cell, cell*, cell*> pending_callback;
	bool started = false;
	std::condition_variable resume_sync;
	volatile bool pending = false;
	volatile bool attach = false;

	static AMXAPI int new_callback(AMX *amx, cell index, cell *result, cell *params)
	{
		// how do I identify myself from within the AMX?
		// well I can copy code to produce instances of this function for each instance,
		// and since subhook already does pretty undefined behaviour, why not
		auto &self = *running_threads.find(amx)->second; //race condition
		{
			std::unique_lock<std::mutex> lock(self.mutex);
			self.reset = AMX_RESET(amx);
			amx->callback = self.orig_callback;
			self.pending_callback = std::make_tuple(0, index, result, params);
			self.pending = true;
			self.resume_sync.wait(lock);
			self.reset.restore_no_context();
			amx->callback = &new_callback;
			return std::get<0>(self.pending_callback);
		}
	}

	void run()
	{
		{
			std::unique_lock<std::mutex> lock(mutex);
			reset.restore_no_context();
			if(safe)
			{
				amx->callback = &new_callback;
			}
		}

		cell retval;
		int ret = amx_ExecOrig(amx, &retval, AMX_EXEC_CONT);
		amx->callback = orig_callback;
		if(ret == AMX_ERR_SLEEP)
		{
			amx->error = 0;
			reset = AMX_RESET(amx);
			attach = true;
		}
	}

public:
	thread_state(AMX *amx) : amx(amx), orig_callback(amx->callback), reset(amx),
		thread([=]() { run(); })
	{

	}

	thread_state(const thread_state&) = delete;
	thread_state &operator=(const thread_state&) = delete;

	void set_safe(bool safe)
	{
		this->safe = safe;
	}

	void sync()
	{
		if(pending)
		{
			pending = false;

			reset.restore_no_context();
			if(safe)
			{
				amx->callback = orig_callback;
			}
			std::get<0>(pending_callback) = amx->callback(amx, std::get<1>(pending_callback), std::get<2>(pending_callback), std::get<3>(pending_callback));
			if(safe)
			{
				orig_callback = amx->callback;
				amx->callback = &new_callback;
			}
			reset = AMX_RESET(amx);

			std::lock_guard<std::mutex> lock(mutex);
			resume_sync.notify_one();
		}
		if(attach)
		{
			attach = false;

			reset.restore_no_context();
			cell retval;
			amx_Exec(amx, &retval, AMX_EXEC_CONT);
		}
	}

	void start()
	{
		if(!started)
		{
			started = true;
			thread.start();
		}
	}

	void pause()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(!pending)
		{
			thread.pause();
			if(safe)
			{
				amx->callback = orig_callback;
			}
			// might as well store paramcount
			reset = AMX_RESET(amx);
			amx->stk = amx->stp;
			amx->hea = amx->hlw;
			amx->cip = 0;
			amx->paramcount = 0;
			amx->error = 0;
			amx->frm = 0;
		}
	}

	void resume()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(!pending)
		{
			reset.restore_no_context();
			if(safe)
			{
				orig_callback = amx->callback;
				amx->callback = &new_callback;
			}
			thread.resume();
		}
	}
};

std::unordered_multimap<AMX*, thread_state*> running_threads;

namespace Threads
{
	void DetachThread(AMX *amx, bool safe)
	{
		auto &state = *running_threads.emplace(amx, new thread_state(amx))->second;
		state.set_safe(safe);
	}

	void PauseThreads(AMX *amx)
	{
		auto bounds = running_threads.equal_range(amx);
		for(auto it = bounds.first; it != bounds.second; it++)
		{
			it->second->pause();
		}
	}

	void ResumeThreads(AMX *amx)
	{
		auto bounds = running_threads.equal_range(amx);
		for(auto it = bounds.first; it != bounds.second; it++)
		{
			it->second->resume();
		}
	}

	void StartThreads()
	{
		for(auto &thread : running_threads)
		{
			thread.second->start();
		}
	}

	void SyncThreads()
	{
		for(auto &thread : running_threads)
		{
			thread.second->sync();
		}
	}
}
