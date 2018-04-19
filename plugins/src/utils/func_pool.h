#ifndef FUNC_POOL_H_INCLUDED
#define FUNC_POOL_H_INCLUDED

#include <utility>
#include <array>

namespace aux
{
	namespace impl
	{
		template <size_t Size, size_t Index, template <size_t> class Prototype>
		struct func_pool_allocator
		{
			static std::pair<size_t, decltype(&Prototype<Index>::func)> alloc(std::array<bool, Size> &arr)
			{
				if(std::get<Index>(arr))
				{
					return func_pool_allocator<Size, Index + 1, Prototype>::alloc(arr);
				}else{
					std::get<Index>(arr) = true;
					return std::make_pair(Index, &Prototype<Index>::func);
				}
			}
		};

		template <size_t Size, template <size_t> class Prototype>
		struct func_pool_allocator<Size, Size, Prototype>
		{
			static std::pair<size_t, decltype(&Prototype<-1>::func)> alloc(std::array<bool, Size> &arr)
			{
				return std::make_pair(-1, &Prototype<-1>::func);
			}
		};
	}

	template <class Func>
	struct func;

	template <class Ret, class... Args>
	struct func<Ret(Args...)>
	{
		template <size_t Size, Ret (&Handler)(size_t, Args...)>
		class pool
		{
			static std::array<bool, Size> used;

			template <size_t Id>
			struct prototype
			{
				static Ret func(Args... args)
				{
					return Handler(Id, args...);
				}
			};

		public:
			pool() = delete;

			static std::pair<size_t, Ret(*)(Args...)> add()
			{
				return impl::func_pool_allocator<Size, 0, prototype>::alloc(used);
			}

			static bool remove(size_t index)
			{
				if(index < 0 || index >= Size) return false;
				bool val = used[index];
				used[index] = false;
				return val;
			}
		};
	};

	template <class Ret, class... Args>
	template <size_t Size, Ret(&Handler)(size_t, Args...)>
	std::array<bool, Size> func<Ret(Args...)>::pool<Size, Handler>::used;
}

#endif
