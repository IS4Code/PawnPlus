#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "sdk/amx/amx.h"

namespace events
{
	int register_callback(const char *callback, cell flags, AMX *amx, const char *function, const char *format, const cell *params, int numargs);
	bool remove_callback(AMX *amx, cell id);
	bool invoke_callbacks(AMX *amx, int index, cell *retval);
}

#endif
