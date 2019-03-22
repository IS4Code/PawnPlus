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
#include "modules/amxhook.h"

#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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

	template <class Func>
	static cell pp_entry_string(AMX *amx, cell *params, Func f)
	{
		if(!amx::has_context(amx)) return 0;

		amx::object owner;
		auto &ctx = amx::get_context(amx, owner);
		int index = ctx.get_index();
		
		if(index == AMX_EXEC_CONT)
		{
			return f("#cont");
		}else if(index == AMX_EXEC_MAIN)
		{
			return f("#main");
		}else{
			int len;
			amx_NameLength(amx, &len);
			char *func_name = static_cast<char*>(alloca(len + 1));
			if(amx_GetPublic(amx, index, func_name) == AMX_ERR_NONE)
			{
				return f(func_name);
			}else{
				return f(nullptr);
			}
		}
	}

	// native pp_entry(name[], size=sizeof(name));
	AMX_DEFINE_NATIVE(pp_entry, 2)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		return pp_entry_string(amx, params, [&](const char *str) -> cell
		{
			if(str)
			{
				amx_SetString(addr, str, false, false, params[2]);
				return std::strlen(str);
			}
			return 0;
		});
	}

	// native String:pp_entry_s();
	AMX_DEFINE_NATIVE(pp_entry_s, 0)
	{
		return pp_entry_string(amx, params, [&](const char *str)
		{
			return strings::create(str);
		});
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

	// native pp_num_local_handles();
	AMX_DEFINE_NATIVE(pp_num_local_handles, 0)
	{
		return handle_pool.local_size();
	}

	// native pp_num_global_handles();
	AMX_DEFINE_NATIVE(pp_num_global_handles, 0)
	{
		return handle_pool.global_size();
	}

	// native pp_max_hooked_natives();
	AMX_DEFINE_NATIVE(pp_max_hooked_natives, 0)
	{
		return amxhook::hook_pool_size();
	}

	// native pp_num_hooked_natives();
	AMX_DEFINE_NATIVE(pp_num_hooked_natives, 0)
	{
		return amxhook::hook_count();
	}

	// native pp_collect();
	AMX_DEFINE_NATIVE(pp_collect, 0)
	{
		gc_collect();
		return 1;
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
		return 1;
	}

	// native pp_error_level(error_level:level);
	AMX_DEFINE_NATIVE(pp_error_level, 1)
	{
		auto &extra = amx::load_lock(amx)->get_extra<native_error_level>();
		auto old = extra.level;
		extra.level = params[1];
		return old;
	}

	// native pp_raise_error(const message[], error_level:level=error_logic);
	AMX_DEFINE_NATIVE(pp_raise_error, 1)
	{
		const char *message;
		amx_StrParam(amx, params[1], message);
		throw errors::native_error(std::string(message), optparam(2, 2));
	}

	template <class Func>
	static cell pp_module_name_string(AMX *amx, cell *params, Func f)
	{
		const char *function;
		amx_StrParam(amx, params[1], function);
		auto func = amx::find_native(amx, function);
		if(!func)
		{
			amx_FormalError(errors::func_not_found, "native", function);
			return 0;
		}

		const char* name;
		size_t len;
#ifdef _WIN32
		HMODULE hmod;
		if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(func), &hmod))
		{
			return 0;
		}
		char filename[MAX_PATH];
		len = GetModuleFileNameA(hmod, filename, sizeof(filename));
		name = filename;
#else
		Dl_info info;
		if(dladdr(reinterpret_cast<void*>(func), &info) != 0)
		{
			return 0;
		}
		name = info.dli_fname;
		len = std::strlen(name);
#endif
		if(len > 0)
		{
			const char *last = name + len;
			const char *dot = nullptr;
			while(last > name)
			{
				last -= 1;
				if(*last == '.' && !dot)
				{
					dot = last;
				}else if(*last == '/' || *last == '\\')
				{
					name = last + 1;
					break;
				}
			}
			if(dot)
			{
				return f(std::string(name, dot));
			}
		}
		return f(name);
	}

	// native pp_module_name(const function[], name[], size=sizeof(name));
	AMX_DEFINE_NATIVE(pp_module_name, 3)
	{
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		return pp_module_name_string(amx, params, [&](const std::string &str)
		{
			amx_SetString(addr, str.c_str(), false, false, params[3]);
			return str.size();
		});
	}

	// native String:pp_module_name_s(const function[]);
	AMX_DEFINE_NATIVE(pp_module_name_s, 1)
	{
		return pp_module_name_string(amx, params, [&](std::string &&str)
		{
			return strings::create(std::move(str));
		});
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
	AMX_DECLARE_NATIVE(pp_num_local_handles),
	AMX_DECLARE_NATIVE(pp_num_global_handles),
	AMX_DECLARE_NATIVE(pp_max_hooked_natives),
	AMX_DECLARE_NATIVE(pp_num_hooked_natives),
	AMX_DECLARE_NATIVE(pp_entry),
	AMX_DECLARE_NATIVE(pp_entry_s),
	AMX_DECLARE_NATIVE(pp_collect),
	AMX_DECLARE_NATIVE(pp_num_natives),
	AMX_DECLARE_NATIVE(pp_max_recursion),
	AMX_DECLARE_NATIVE(pp_error_level),
	AMX_DECLARE_NATIVE(pp_raise_error),
	AMX_DECLARE_NATIVE(pp_module_name),
	AMX_DECLARE_NATIVE(pp_module_name_s),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
