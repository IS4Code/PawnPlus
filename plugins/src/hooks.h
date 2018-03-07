#ifndef HOOKS_H_INCLUDED
#define HOOKS_H_INCLUDED

#include "sdk/amx/amx.h"
int AMXAPI amx_ExecOrig(AMX *amx, cell *retval, int index);
int AMXAPI amx_FindPublicOrig(AMX *amx, const char *funcname, int *index);

namespace Hooks
{
	void Register();
	void Unregister();
	void ToggleStrLen(bool toggle);
}

#endif
