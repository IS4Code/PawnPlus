#include "natives.h"

namespace Natives
{
	// native AMX:amx_this();
	static cell AMX_NATIVE_CALL amx_this(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(amx);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(amx_this),
};

int RegisterAmxNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
