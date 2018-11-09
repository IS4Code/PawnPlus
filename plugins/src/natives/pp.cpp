#include "natives.h"
#include "main.h"
#include "hooks.h"
#include "exec.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "modules/containers.h"

namespace Natives
{
	// native pp_hook_strlen(bool:hook);
	AMX_DEFINE_NATIVE(pp_hook_strlen, 1)
	{
		Hooks::ToggleStrLen(static_cast<bool>(params[1]));
		return 1;
	}

	// native pp_hook_check_ref_args(bool:hook);
	AMX_DEFINE_NATIVE(pp_hook_check_ref_args, 1)
	{
		Hooks::ToggleRefArgs(static_cast<bool>(params[1]));
		return 1;
	}

	// native pp_num_tasks();
	AMX_DEFINE_NATIVE(pp_num_tasks, 0)
	{
		return tasks::size();
	}

	// native pp_num_local_strings();
	AMX_DEFINE_NATIVE(pp_num_local_strings, 0)
	{
		return strings::pool.local_size();
	}

	// native pp_num_global_strings();
	AMX_DEFINE_NATIVE(pp_num_global_strings, 0)
	{
		return strings::pool.global_size();
	}

	// native pp_num_local_variants();
	AMX_DEFINE_NATIVE(pp_num_local_variants, 0)
	{
		return variants::pool.local_size();
	}

	// native pp_num_global_variants();
	AMX_DEFINE_NATIVE(pp_num_global_variants, 0)
	{
		return variants::pool.global_size();
	}

	// native pp_num_lists();
	AMX_DEFINE_NATIVE(pp_num_lists, 0)
	{
		return list_pool.size();
	}

	// native pp_num_maps();
	AMX_DEFINE_NATIVE(pp_num_maps, 0)
	{
		return map_pool.size();
	}

	// native pp_num_guards();
	AMX_DEFINE_NATIVE(pp_num_guards, 0)
	{
		return guards::count(amx);
	}

	// native String:pp_entry_s();
	AMX_DEFINE_NATIVE(pp_entry_s, 0)
	{
		if(!amx::has_context(amx)) return 0;

		amx::object owner;
		auto &ctx = amx::get_context(amx, owner);
		int index = ctx.get_index();
		
		if(index == AMX_EXEC_CONT)
		{
			return strings::create("#cont");
		}else if(index == AMX_EXEC_MAIN)
		{
			return strings::create("#main");
		}else{
			int len;
			amx_NameLength(amx, &len);
			char *func_name = static_cast<char*>(alloca(len + 1));
			if(amx_GetPublic(amx, index, func_name) == AMX_ERR_NONE)
			{
				return strings::create(std::string(func_name));
			}else{
				return 0;
			}
		}
	}

	// native pp_num_local_iters();
	AMX_DEFINE_NATIVE(pp_num_local_iters, 0)
	{
		return iter_pool.local_size();
	}

	// native pp_num_global_iters();
	AMX_DEFINE_NATIVE(pp_num_global_iters, 0)
	{
		return iter_pool.global_size();
	}

	// native pp_collect();
	AMX_DEFINE_NATIVE(pp_collect, 0)
	{
		gc_collect();
		return 0;
	}

	// native pp_num_natives();
	AMX_DEFINE_NATIVE(pp_num_natives, 0)
	{
		return amx::num_natives(amx);
	}

	// native pp_max_recursion(level);
	AMX_DEFINE_NATIVE(pp_max_recursion, 1)
	{
		maxRecursionLevel = params[1];
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
	AMX_DECLARE_NATIVE(pp_num_natives),
	AMX_DECLARE_NATIVE(pp_max_recursion),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
