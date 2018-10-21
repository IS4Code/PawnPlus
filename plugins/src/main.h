#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "sdk/amx/amx.h"
#include <utility>

typedef void(*logprintf_t)(const char* format, ...);
extern logprintf_t logprintf;

template <class... Args>
int logerror(AMX *amx, int error, const char *format, Args&&... args)
{
	logprintf(format, std::forward<Args>(args)...);
	amx_RaiseError(amx, error);
	return error;
}

template <class... Args>
int logerror(AMX *amx, const char *format, Args&&... args)
{
	return logerror(amx, AMX_ERR_NATIVE, format, std::forward<Args>(args)...);
}

template <class... Args>
void logwarn(AMX *amx, const char *format, Args&&... args)
{
	logprintf(format, std::forward<Args>(args)...);
}

void gc_collect();

#endif
