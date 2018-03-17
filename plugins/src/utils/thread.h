#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace aux
{
	class thread : public std::thread
	{
		union {
			char _0;
			std::mutex mutex;
		};
		union {
			char _1;
			std::condition_variable sync1;
		};
		volatile bool started;

		void global_init();
		void thread_init();
		void thread_exit();

		template<class Function, class... Args>
		std::function<typename std::result_of<Function(Args...)>::type(Args...)> wrap_func(Function&& f)
		{
			// must be initialized before the thread starts
			started = false;
			new (&mutex) std::mutex();
			new (&sync1) std::condition_variable();

			global_init();
			return [=](Args&&... args)
			{
				thread_init();
				f(std::forward<Args>(args)...);
				//invoke<Function, Args...>(f, std::forward<Args>(args)...);
				//std::invoke(f, std::forward<Args>(args)...);
				thread_exit();
			};
		}

	public:
		template<class Function, class... Args>
		explicit thread(Function&& f, Args&&... args)
			: std::thread(wrap_func<Function, Args...>(std::forward<Function>(f)), std::forward<Args>(args)...)
		{
			
		}

		void start();
		void pause();
		void resume();

		~thread()
		{
			mutex.~mutex();
			sync1.~condition_variable();
		}
	};
}

#endif
