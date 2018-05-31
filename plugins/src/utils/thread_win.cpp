#ifdef _WIN32
#include "thread.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace aux
{
	void thread::global_init()
	{

	}

	void thread::thread_init()
	{
		{
			std::unique_lock<std::mutex> lock(mutex);
			if(!started) sync1.wait(lock);
		}
		mutex.~mutex();
	}

	void thread::thread_exit()
	{
		detach();
	}

	void thread::start()
	{
		if(!started)
		{
			std::lock_guard<std::mutex> lock(mutex);
			started = true;
			sync1.notify_one();
		}
	}

	void thread::pause()
	{
		HANDLE hThread = static_cast<HANDLE>(native_handle());
		SuspendThread(hThread);
		CONTEXT ctx;
		ctx.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, &ctx);
	}

	void thread::resume()
	{
		HANDLE hThread = static_cast<HANDLE>(native_handle());
		ResumeThread(hThread);
	}
}

#endif
