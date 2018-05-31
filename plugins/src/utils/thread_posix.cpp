#ifndef _WIN32
#include "thread.h"
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

namespace aux
{
	void thread::global_init()
	{
		static bool init = false;
		if(!init)
		{
			sigset_t set;
			sigemptyset(&set);
			sigaddset(&set, SIGUSR1);
			sigaddset(&set, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &set, NULL);

			struct sigaction action;
			action.sa_handler = [](int signo)
			{
				if(signo == SIGUSR1)
				{
					sigset_t set;
					sigemptyset(&set);
					sigaddset(&set, SIGUSR2);
					int ret;
					sigwait(&set, &ret);
				}
			};
			sigemptyset(&action.sa_mask);
			action.sa_flags = SA_RESTART;
			sigaction(SIGUSR1, &action, nullptr);

			init = true;
		}
	}

	void thread::thread_init()
	{
		{
			std::unique_lock<std::mutex> lock(mutex);

			sigset_t set;
			sigemptyset(&set);
			sigaddset(&set, SIGUSR1);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);
			sigemptyset(&set);
			sigaddset(&set, SIGUSR2);
			pthread_sigmask(SIG_BLOCK, &set, NULL);

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
		std::lock_guard<std::mutex> lock(mutex);
		started = true;
		sync1.notify_one();
	}

	void thread::pause()
	{
		pthread_kill(native_handle(), SIGUSR1);
	}

	void thread::resume()
	{
		pthread_kill(native_handle(), SIGUSR2);
	}
}

#endif
