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
		std::mutex mutex;
		std::condition_variable sync1;
		volatile bool started;

		void global_init();
		void thread_init();
		void thread_exit();

		template<class Function, class... Args>
		std::function<typename std::result_of<Function(Args...)>::type(Args...)> wrap_func(Function&& f)
		{
			global_init();
			return [=](Args&&... args)
			{
				thread_init();
				std::invoke(f, std::forward<Args>(args)...);
				thread_exit();
			};
		}

	public:
		template<class Function, class... Args>
		explicit thread(Function&& f, Args&&... args) : mutex(), sync1(), started(false)
		{
			// 
			this->std::thread::thread(wrap_func<Function, Args...>(std::forward<Function>(f)), std::forward<Args>(args)...);
		}

		void start();
		void pause();
		void resume();
	};
}

#endif