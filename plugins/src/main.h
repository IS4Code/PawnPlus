#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "sdk/amx/amx.h"
#include <utility>

#define PP_VERSION_STRING "v1.0.1"
#define PP_VERSION_NUMBER 0x101

typedef void(*logprintf_t)(const char* format, ...);
extern logprintf_t logprintf;

extern thread_local bool is_main_thread;

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
void *gc_register(void(*func)());
void gc_unregister(void *id);

unsigned char *amx_GetData(AMX *amx);

#endif
