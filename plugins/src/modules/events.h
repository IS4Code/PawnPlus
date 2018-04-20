#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "sdk/amx/amx.h"

namespace events
{
	void load(AMX *amx);
	void unload(AMX *amx);

	int register_callback(const char *callback, AMX *amx, const char *function, const char *format, const cell *params, int numargs);
	bool remove_callback(AMX *amx, int index);
	int get_callback_id(AMX *amx, const char *callback);
	const char *invoke_callback(int id, AMX *amx, cell *retval);
}

#endif
