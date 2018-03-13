#include "../natives.h"
#include "../hooks.h"

namespace Natives
{
	// native pp_hook_strlen(bool:hook);
	static cell AMX_NATIVE_CALL pp_hook_strlen(AMX *amx, cell *params)
	{
		Hooks::ToggleStrLen(static_cast<bool>(params[1]));
		return 1;
	}

	// native pp_hook_check_ref_args(bool:hook);
	static cell AMX_NATIVE_CALL pp_hook_check_ref_args(AMX *amx, cell *params)
	{
		Hooks::ToggleRefArgs(static_cast<bool>(params[1]));
		return 1;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pp_hook_strlen),
	AMX_DECLARE_NATIVE(pp_hook_check_ref_args),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
