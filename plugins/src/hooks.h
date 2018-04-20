#ifndef HOOKS_H_INCLUDED
#define HOOKS_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"

int AMXAPI amx_ExecOrig(AMX *amx, cell *retval, int index);
int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, AMX_RESET *reset);
int AMXAPI amx_FindPublicOrig(AMX *amx, const char *funcname, int *index);

namespace Hooks
{
	void register_callback();
	void Unregister();
	void ToggleStrLen(bool toggle);
	void ToggleRefArgs(bool toggle);
}

#endif
