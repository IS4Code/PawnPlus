#include "systools.h"
#include <stdint.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

size_t stackspace()
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(static_cast<LPCVOID>(&mbi), &mbi, sizeof(mbi));
	return reinterpret_cast<uintptr_t>(&mbi) - reinterpret_cast<uintptr_t>(mbi.AllocationBase);
}

#else

#include <pthread.h>

size_t stackspace()
{
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	void *addr;
	size_t size;
	pthread_attr_getstack(&attr, &addr, &size);
	return reinterpret_cast<uintptr_t>(&attr) - reinterpret_cast<uintptr_t>(addr);
}

#endif
