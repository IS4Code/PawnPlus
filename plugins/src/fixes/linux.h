#ifndef LINUX_H_INCLUDED
#define LINUX_H_INCLUDED

#include <memory>
#include <type_traits>

#ifndef _WIN32
namespace std
{
	template <typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template <typename T, typename = typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0>::type>
	std::unique_ptr<T> make_unique(size_t size)
	{
		return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]);
	}
}
#endif

#endif
