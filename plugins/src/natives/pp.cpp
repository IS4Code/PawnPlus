#include "natives.h"
#include "main.h"
#include "hooks.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "modules/containers.h"

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
		return tasks::size();
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
		return guards::count(amx);
	}

	// native String:pp_entry_s();
	static cell AMX_NATIVE_CALL pp_entry_s(AMX *amx, cell *params)
	{
		if(!amx::has_context(amx)) return 0;

		amx::object owner;
		auto &ctx = amx::get_context(amx, owner);
		int index = ctx.get_index();
		
		if(index == AMX_EXEC_CONT)
		{
			return strings::pool.get_id(strings::create("#cont", true));
		}else if(index == AMX_EXEC_MAIN)
		{
			return strings::pool.get_id(strings::create("#main", true));
		}else{
			int len;
			amx_NameLength(amx, &len);
			char *func_name = static_cast<char*>(alloca(len + 1));
			if(amx_GetPublic(amx, index, func_name) == AMX_ERR_NONE)
			{
				return strings::pool.get_id(strings::create(std::string(func_name), true));
			}else{
				return 0;
			}
		}
	}

	// native pp_num_local_iters();
	static cell AMX_NATIVE_CALL pp_num_local_iters(AMX *amx, cell *params)
	{
		return iter_pool.local_size();
	}

	// native pp_num_global_iters();
	static cell AMX_NATIVE_CALL pp_num_global_iters(AMX *amx, cell *params)
	{
		return iter_pool.global_size();
	}

	// native pp_collect();
	static cell AMX_NATIVE_CALL pp_collect(AMX *amx, cell *params)
	{
		gc_collect();
		return 0;
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
	AMX_DECLARE_NATIVE(pp_num_local_iters),
	AMX_DECLARE_NATIVE(pp_num_global_iters),
	AMX_DECLARE_NATIVE(pp_entry_s),
	AMX_DECLARE_NATIVE(pp_collect),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
