#ifndef AMX_HOOK_H_INCLUDED
#define AMX_HOOK_H_INCLUDED

#include "sdk/amx/amx.h"

namespace amxhook
{
	int register_hook(AMX *amx, bool post, const char *native, const char *native_format, const char *handler, const char *format, const cell *params, int numargs);
	bool remove_hook(AMX *amx, int index);
}

#endif
