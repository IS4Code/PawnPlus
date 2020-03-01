#ifndef REF_LOCK_H_INCLUDED
#define REF_LOCK_H_INCLUDED

#include <atomic>

namespace aux
{
	template <class Obj, Obj &Val>
	class lock_guard
	{
		static std::atomic_flag &value() noexcept
		{
			static std::atomic_flag obj = ATOMIC_FLAG_INIT;
			return obj;
		}

	public:
		lock_guard() noexcept
		{
			while(value().test_and_set(std::memory_order_acquire)){}
		}

		~lock_guard() noexcept
		{
			value().clear(std::memory_order_release);
		}
	};

	template <class Obj, Obj &Val>
	class lock_object
	{
	public:
		template <class Func>
		auto operator=(Func func) noexcept(noexcept(func())) -> decltype(func())
		{
			lock_guard<Obj, Val> guard;
			return func();
		}
	};
}

#define locked(obj) ::aux::lock_object<decltype(obj), obj>{}=[&]()

#endif
