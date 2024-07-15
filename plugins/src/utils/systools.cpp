#include "systools.h"
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::size_t aux::stackspace()
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(static_cast<LPCVOID>(&mbi), &mbi, sizeof(mbi));
	return reinterpret_cast<uintptr_t>(&mbi) - reinterpret_cast<uintptr_t>(mbi.AllocationBase);
}

#else

#include <pthread.h>

std::size_t aux::stackspace()
{
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	void *addr;
	size_t size;
	pthread_attr_getstack(&attr, &addr, &size);
	return reinterpret_cast<uintptr_t>(&attr) - reinterpret_cast<uintptr_t>(addr);
}

#endif

std::tm aux::gmtime(const std::time_t &time)
{
	std::tm tm;
#ifdef _WIN32
	gmtime_s(&tm, &time);
#else
	gmtime_r(&time, &tm);
#endif
	return tm;
}

std::tm aux::localtime(const std::time_t &time)
{
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif
	return tm;
}
