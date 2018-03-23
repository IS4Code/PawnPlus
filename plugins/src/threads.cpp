#include "threads.h"
#include "hooks.h"
#include "reset.h"
#include "tasks.h"
#include "utils/thread.h"
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <tuple>
#include <limits>

class thread_state;
extern std::unordered_multimap<AMX*, thread_state*> running_threads;
thread_local thread_state *my_instance;

class thread_state
{
	std::mutex mutex;
	AMX_RESET reset;
	aux::thread thread;
	AMX_CALLBACK orig_callback;
	AMX *amx;
	Threads::SyncFlags flags;
	std::tuple<int, cell, cell*, cell*> pending_callback;
	bool started = false;
	std::condition_variable resume_sync;
	std::condition_variable join_sync;
	volatile bool pending = false;
	volatile bool attach = false;
	volatile bool done = false;
	bool paused = false;

	static constexpr cell sync_index = std::numeric_limits<cell>::min();

	int auto_callback(AMX *amx, cell index, cell *result, cell *params)
	{
		std::unique_lock<std::mutex> lock(mutex);
		reset = AMX_RESET(amx);
		amx->callback = orig_callback;
		pending_callback = std::make_tuple(0, index, result, params);
		pending = true;
		join_sync.notify_all();
		resume_sync.wait(lock);
		reset.restore_no_context();
		if((flags & Threads::SyncFlags::SyncAuto) == Threads::SyncFlags::SyncAuto)
		{
			amx->callback = &new_callback;
		} else {
			amx->callback = &amx_Callback;
		}
		return std::get<0>(pending_callback);
	}

	static AMXAPI int new_callback(AMX *amx, cell index, cell *result, cell *params)
	{
		return my_instance->auto_callback(amx, index, result, params);
	}

	void run()
	{
		my_instance = this;
		{
			std::unique_lock<std::mutex> lock(mutex);
			reset.restore_no_context();
			orig_callback = amx->callback;
			if((flags & Threads::SyncFlags::SyncAuto) == Threads::SyncFlags::SyncAuto)
			{
				amx->callback = &new_callback;
			}else{
				amx->callback = &amx_Callback;
			}
			started = true;
			join_sync.notify_all();
		}

		cell retval;
		while(true)
		{
			int ret = amx_ExecOrig(amx, &retval, AMX_EXEC_CONT);
			if(ret == AMX_ERR_SLEEP)
			{
				switch(amx->pri & SleepReturnTypeMask)
				{
					case SleepReturnAttach:
						amx->callback = orig_callback;
						amx->pri = 0;
						amx->error = 0;
						reset = AMX_RESET(amx);
						attach = true;
						join_sync.notify_all();
						return;
					case SleepReturnSync:
						amx->pri = 0;
						amx->error = 0;
						if((flags & Threads::SyncFlags::SyncAuto) != Threads::SyncFlags::SyncAuto)
						{
							auto_callback(amx, sync_index, nullptr, nullptr);
						}
						break;
					case SleepReturnDetach:
						amx->pri = 0;
						set_flags(static_cast<Threads::SyncFlags>(SleepReturnValueMask & amx->pri));
						break;
					default:
						amx->callback = orig_callback;
						return;
				}
			}
		}
	}

public:
	thread_state(AMX *amx) : amx(amx), orig_callback(amx->callback), reset(amx),
		thread([=]() { run(); })
	{

	}

	thread_state(const thread_state&) = delete;
	thread_state &operator=(const thread_state&) = delete;

	void set_flags(Threads::SyncFlags flags)
	{
		this->flags = flags;
	}

	bool sync()
	{
		if(done) return false;

		if(attach)
		{
			attach = false;
			done = true;
			reset.restore_no_context();
			cell retval;
			amx_Exec(amx, &retval, AMX_EXEC_CONT);
			return true;
		}
		if(pending)
		{
			pending = false;
			reset.restore_no_context();
			amx->callback = orig_callback;
			cell &result = std::get<0>(pending_callback);
			cell index = std::get<1>(pending_callback);
			if(index != sync_index)
			{
				// In auto-sync mode, we don't want to patch the code
				cell sysreq_d = amx->sysreq_d;
				amx->sysreq_d = 0;
				result = amx->callback(amx, index, std::get<2>(pending_callback), std::get<3>(pending_callback));
				amx->sysreq_d = sysreq_d;
			}else{
				result = 1;
			}

			orig_callback = amx->callback;
			if((flags & Threads::SyncFlags::SyncAuto) == Threads::SyncFlags::SyncAuto)
			{
				amx->callback = &new_callback;
			}else{
				amx->callback = &amx_Callback;
			}
			reset = AMX_RESET(amx);

			std::lock_guard<std::mutex> lock(mutex);
			resume_sync.notify_one();
		}
		return false;
	}

	void start()
	{
		if(!started)
		{
			std::unique_lock<std::mutex> lock(mutex);
			thread.start();
			join_sync.wait(lock);
		}
	}

	bool join()
	{
		if(started && !paused && !done)
		{
			std::unique_lock<std::mutex> lock(mutex);
			if(pending || attach || done) return false;
			join_sync.wait(lock);
			if(!pending && !attach)
			{
				delete this;
				return true;
			}
		}
		return false;
	}

	void pause()
	{
		if(started && !pending && (flags & Threads::SyncFlags::SyncInterrupt) == Threads::SyncFlags::SyncInterrupt)
		{
			std::lock_guard<std::mutex> lock(mutex);
			if(pending) return;

			thread.pause();
			amx->callback = orig_callback;
			// might as well store paramcount
			reset = AMX_RESET(amx);
			amx->stk = amx->stp;
			amx->hea = amx->hlw;
			amx->cip = 0;
			amx->paramcount = 0;
			amx->error = 0;
			amx->frm = 0;
			paused = true;
		}
	}

	void resume()
	{
		if(paused)
		{
			reset.restore_no_context();
			orig_callback = amx->callback;
			if((flags & Threads::SyncFlags::SyncAuto) == Threads::SyncFlags::SyncAuto)
			{
				amx->callback = &new_callback;
			}else{
				amx->callback = &amx_Callback;
			}
			paused = false;
			thread.resume();
		}
	}
};

std::unordered_multimap<AMX*, thread_state*> running_threads;

namespace Threads
{
	void DetachThread(AMX *amx, SyncFlags flags)
	{
		auto &state = *running_threads.emplace(amx, new thread_state(amx))->second;
		state.set_flags(flags);
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

	void JoinThreads(AMX *amx)
	{
		auto bounds = running_threads.equal_range(amx);
		auto it = bounds.first;
		while(it != bounds.second)
		{
			if(it->second->join())
			{
				it = running_threads.erase(it);
			}else{
				it++;
			}
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
		auto it = running_threads.begin();
		while(it != running_threads.end())
		{
			if(it->second->sync())
			{
				it = running_threads.erase(it);
			}else{
				it++;
			}
		}
	}
}
