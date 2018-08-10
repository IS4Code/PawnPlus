#ifndef AMX_HOOK_H_INCLUDED
#define AMX_HOOK_H_INCLUDED

#include "sdk/amx/amx.h"

namespace amxhook
{
	cell register_hook(AMX *amx, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs);
	bool remove_hook(AMX *amx, cell id);
}

#endif
