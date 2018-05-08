#include "natives.h"
#include "hooks.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "pools.h"

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

	// native pp_num_tasks();
	static cell AMX_NATIVE_CALL pp_num_tasks(AMX *amx, cell *params)
	{
		return TaskPool::Size();
	}

	// native pp_num_local_strings();
	static cell AMX_NATIVE_CALL pp_num_local_strings(AMX *amx, cell *params)
	{
		return strings::pool.local_size();
	}

	// native pp_num_global_strings();
	static cell AMX_NATIVE_CALL pp_num_global_strings(AMX *amx, cell *params)
	{
		return strings::pool.global_size();
	}

	// native pp_num_local_variants();
	static cell AMX_NATIVE_CALL pp_num_local_variants(AMX *amx, cell *params)
	{
		return variants::pool.local_size();
	}

	// native pp_num_global_variants();
	static cell AMX_NATIVE_CALL pp_num_global_variants(AMX *amx, cell *params)
	{
		return variants::pool.global_size();
	}

	// native pp_num_lists();
	static cell AMX_NATIVE_CALL pp_num_lists(AMX *amx, cell *params)
	{
		return list_pool.size();
	}

	// native pp_num_maps();
	static cell AMX_NATIVE_CALL pp_num_maps(AMX *amx, cell *params)
	{
		return map_pool.size();
	}

	// native pp_num_guards();
	static cell AMX_NATIVE_CALL pp_num_guards(AMX *amx, cell *params)
	{
		auto &ctx = Context::Get(amx);
		return ctx.guards.size();
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pp_hook_strlen),
	AMX_DECLARE_NATIVE(pp_hook_check_ref_args),
	AMX_DECLARE_NATIVE(pp_num_tasks),
	AMX_DECLARE_NATIVE(pp_num_local_strings),
	AMX_DECLARE_NATIVE(pp_num_global_strings),
	AMX_DECLARE_NATIVE(pp_num_local_variants),
	AMX_DECLARE_NATIVE(pp_num_global_variants),
	AMX_DECLARE_NATIVE(pp_num_lists),
	AMX_DECLARE_NATIVE(pp_num_maps),
	AMX_DECLARE_NATIVE(pp_num_guards),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
