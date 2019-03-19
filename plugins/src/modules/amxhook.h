#ifndef AMX_HOOK_H_INCLUDED
#define AMX_HOOK_H_INCLUDED

#include "sdk/amx/amx.h"

namespace amxhook
{
	cell register_hook(AMX *amx, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs);
	cell register_filter(AMX *amx, bool output, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs);
	bool remove_hook(cell id);
	size_t hook_pool_size();
	size_t hook_count();
}

#endif
