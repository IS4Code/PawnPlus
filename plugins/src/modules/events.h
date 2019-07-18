#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "expressions.h"
#include "sdk/amx/amx.h"

namespace events
{
	int register_callback(const char *callback, cell flags, AMX *amx, const char *function, const char *format, const cell *params, int numargs);
	bool remove_callback(AMX *amx, cell id);
	bool invoke_callbacks(AMX *amx, int index, cell *retval);

	int new_callback(const char *callback, AMX *amx, expression_ptr &&default_action);
	int find_callback(AMX *amx, const char *callback);
	const char *callback_name(AMX *amx, int index);
	int num_callbacks(AMX *amx);
	void name_length(AMX *amx, int &length);
}

#endif
