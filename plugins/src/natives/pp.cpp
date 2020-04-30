#include "natives.h"
#include "main.h"
#include "hooks.h"
#include "exec.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/strings.h"
#include "modules/format.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "modules/containers.h"
#include "modules/amxhook.h"
#include "modules/expressions.h"
#include "utils/systools.h"

#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Natives
{
	// native pp_version();
	AMX_DEFINE_NATIVE_TAG(pp_version, 0, cell)
	{
		return PP_VERSION_NUMBER;
	}

	// native pp_version_string(version[], size=sizeof(version));
	AMX_DEFINE_NATIVE_TAG(pp_version_string, 2, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		amx_SetString(addr, PP_VERSION_STRING, false, false, params[2]);
		return sizeof(PP_VERSION_STRING) - 1;
	}

	// native String:pp_version_string_s();
	AMX_DEFINE_NATIVE_TAG(pp_version_string_s, 0, string)
	{
		return strings::create(PP_VERSION_STRING);
	}

	// native pp_hook_strlen(bool:hook);
	AMX_DEFINE_NATIVE_TAG(pp_hook_strlen, 1, cell)
	{
		Hooks::ToggleStrLen(static_cast<bool>(params[1]));
		return 1;
	}

	// native pp_hook_check_ref_args(bool:hook);
	AMX_DEFINE_NATIVE_TAG(pp_hook_check_ref_args, 1, cell)
	{
		Hooks::ToggleRefArgs(static_cast<bool>(params[1]));
		return 1;
	}

	// native pp_public_min_index(index);
	AMX_DEFINE_NATIVE_TAG(pp_public_min_index, 1, cell)
	{
		cell index = params[1];
		if(index > 0)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		disable_public_warning = true;
		int orig = public_min_index;
		public_min_index = index;
		return orig;
	}

	// native bool:pp_use_funcidx(bool:use);
	AMX_DEFINE_NATIVE_TAG(pp_use_funcidx, 1, bool)
	{
		bool orig = use_funcidx;
		disable_public_warning = true;
		use_funcidx = params[1];
		return orig;
	}

	// native pp_tick();
	AMX_DEFINE_NATIVE_TAG(pp_tick, 0, cell)
	{
		::pp_tick();
		return 1;
	}

	// native pp_num_tasks();
	AMX_DEFINE_NATIVE_TAG(pp_num_tasks, 0, cell)
	{
		return tasks::size();
	}

	// native pp_num_local_strings();
	AMX_DEFINE_NATIVE_TAG(pp_num_local_strings, 0, cell)
	{
		return strings::pool.local_size();
	}

	// native pp_num_global_strings();
	AMX_DEFINE_NATIVE_TAG(pp_num_global_strings, 0, cell)
	{
		return strings::pool.global_size();
	}

	// native pp_num_local_variants();
	AMX_DEFINE_NATIVE_TAG(pp_num_local_variants, 0, cell)
	{
		return variants::pool.local_size();
	}

	// native pp_num_global_variants();
	AMX_DEFINE_NATIVE_TAG(pp_num_global_variants, 0, cell)
	{
		return variants::pool.global_size();
	}

	// native pp_num_lists();
	AMX_DEFINE_NATIVE_TAG(pp_num_lists, 0, cell)
	{
		return list_pool.size();
	}

	// native pp_num_linked_lists();
	AMX_DEFINE_NATIVE_TAG(pp_num_linked_lists, 0, cell)
	{
		return linked_list_pool.size();
	}

	// native pp_num_maps();
	AMX_DEFINE_NATIVE_TAG(pp_num_maps, 0, cell)
	{
		return map_pool.size();
	}

	// native pp_num_pools();
	AMX_DEFINE_NATIVE_TAG(pp_num_pools, 0, cell)
	{
		return pool_pool.size();
	}

	// native pp_num_guards();
	AMX_DEFINE_NATIVE_TAG(pp_num_guards, 0, cell)
	{
		return guards::count(amx);
	}

	// native pp_num_amx_guards();
	AMX_DEFINE_NATIVE_TAG(pp_num_amx_guards, 0, cell)
	{
		return amx_guards::count(amx);
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
			char *func_name = amx_NameBuffer(amx);
			if(amx_GetPublic(amx, index, func_name) == AMX_ERR_NONE)
			{
				return f(func_name);
			}else{
				return f(nullptr);
			}
		}
	}

	// native pp_entry(name[], size=sizeof(name));
	AMX_DEFINE_NATIVE_TAG(pp_entry, 2, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
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
	AMX_DEFINE_NATIVE_TAG(pp_entry_s, 0, string)
	{
		return pp_entry_string(amx, params, [&](const char *str)
		{
			return strings::create(str);
		});
	}
	
	// native pp_num_local_iters();
	AMX_DEFINE_NATIVE_TAG(pp_num_local_iters, 0, cell)
	{
		return iter_pool.local_size();
	}

	// native pp_num_global_iters();
	AMX_DEFINE_NATIVE_TAG(pp_num_global_iters, 0, cell)
	{
		return iter_pool.global_size();
	}

	// native pp_num_local_handles();
	AMX_DEFINE_NATIVE_TAG(pp_num_local_handles, 0, cell)
	{
		return handle_pool.local_size();
	}

	// native pp_num_global_handles();
	AMX_DEFINE_NATIVE_TAG(pp_num_global_handles, 0, cell)
	{
		return handle_pool.global_size();
	}

	// native pp_num_local_expressions();
	AMX_DEFINE_NATIVE_TAG(pp_num_local_expressions, 0, cell)
	{
		return expression_pool.local_size();
	}

	// native pp_num_global_expressions();
	AMX_DEFINE_NATIVE_TAG(pp_num_global_expressions, 0, cell)
	{
		return expression_pool.global_size();
	}

	// native pp_max_hooked_natives();
	AMX_DEFINE_NATIVE_TAG(pp_max_hooked_natives, 0, cell)
	{
		return amxhook::hook_pool_size();
	}

	// native pp_num_hooked_natives();
	AMX_DEFINE_NATIVE_TAG(pp_num_hooked_natives, 0, cell)
	{
		return amxhook::hook_count();
	}

	// native pp_collect();
	AMX_DEFINE_NATIVE_TAG(pp_collect, 0, cell)
	{
		gc_collect();
		return 1;
	}

	// native pp_num_natives();
	AMX_DEFINE_NATIVE_TAG(pp_num_natives, 0, cell)
	{
		return amx::num_natives(amx);
	}

	// native pp_max_recursion(level);
	AMX_DEFINE_NATIVE_TAG(pp_max_recursion, 1, cell)
	{
		maxRecursionLevel = params[1];
		return 1;
	}

	// native pp_error_level(error_level:level);
	AMX_DEFINE_NATIVE_TAG(pp_error_level, 1, cell)
	{
		auto &extra = amx::load_lock(amx)->get_extra<native_error_level>();
		auto old = extra.level;
		extra.level = params[1];
		return old;
	}

	// native pp_raise_error(const message[], error_level:level=error_logic);
	AMX_DEFINE_NATIVE_TAG(pp_raise_error, 1, cell)
	{
		const char *message;
		amx_StrParam(amx, params[1], message);
		if(message != nullptr)
		{
			throw errors::native_error(std::string(message), optparam(2, 2));
		}else{
			throw errors::native_error(std::string(), optparam(2, 2));
		}
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
		if(dladdr(reinterpret_cast<void*>(func), &info) == 0)
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
	AMX_DEFINE_NATIVE_TAG(pp_module_name, 3, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		return pp_module_name_string(amx, params, [&](const std::string &str)
		{
			amx_SetString(addr, str.c_str(), false, false, params[3]);
			return str.size();
		});
	}

	// native String:pp_module_name_s(const function[]);
	AMX_DEFINE_NATIVE_TAG(pp_module_name_s, 1, string)
	{
		return pp_module_name_string(amx, params, [&](std::string &&str)
		{
			return strings::create(std::move(str));
		});
	}

	// native pp_locale(const locale[], locale_category:category = locale_all);
	AMX_DEFINE_NATIVE_TAG(pp_locale, 1, cell)
	{
		char *locale;
		amx_StrParam(amx, params[1], locale);
		try{
			if(locale)
			{
				strings::set_locale(std::locale(locale), optparam(2, -1));
			}else{
				strings::set_locale(std::locale(""), optparam(2, -1));
			}
			return 1;
		}catch(const std::runtime_error &err)
		{
			amx_LogicError("%s", err.what());
			return 0;
		}
	}

	// native pp_locale_name(locale[], size=sizeof(locale));
	AMX_DEFINE_NATIVE_TAG(pp_locale_name, 2, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		auto name = strings::locale_name();
		amx_SetString(addr, name.c_str(), false, false, params[2]);
		return name.size();
	}

	// native String:pp_locale_name_s();
	AMX_DEFINE_NATIVE_TAG(pp_locale_name_s, 0, string)
	{
		return strings::create(strings::locale_name());
	}

	// native pp_format_env_push(Map:env);
	AMX_DEFINE_NATIVE_TAG(pp_format_env_push, 1, cell)
	{
		std::shared_ptr<map_t> ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		strings::format_env.push(std::move(ptr));
		return 1;
	}

	// native pp_format_env_pop();
	AMX_DEFINE_NATIVE_TAG(pp_format_env_pop, 0, cell)
	{
		if(strings::format_env.size() == 0)
		{
			amx_LogicError(errors::element_not_present);
		}
		strings::format_env.pop();
		return 1;
	}

	// native pp_stackspace();
	AMX_DEFINE_NATIVE_TAG(pp_stackspace, 0, cell)
	{
		return stackspace();
	}

	// native pp__reserved1();
	AMX_DEFINE_NATIVE(pp__reserved1, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved2();
	AMX_DEFINE_NATIVE(pp__reserved2, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved3();
	AMX_DEFINE_NATIVE(pp__reserved3, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved4();
	AMX_DEFINE_NATIVE(pp__reserved4, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved5();
	AMX_DEFINE_NATIVE(pp__reserved5, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved6();
	AMX_DEFINE_NATIVE(pp__reserved6, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved7();
	AMX_DEFINE_NATIVE(pp__reserved7, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved8();
	AMX_DEFINE_NATIVE(pp__reserved8, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved9();
	AMX_DEFINE_NATIVE(pp__reserved9, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved10();
	AMX_DEFINE_NATIVE(pp__reserved10, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved11();
	AMX_DEFINE_NATIVE(pp__reserved11, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved12();
	AMX_DEFINE_NATIVE(pp__reserved12, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved13();
	AMX_DEFINE_NATIVE(pp__reserved13, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved14();
	AMX_DEFINE_NATIVE(pp__reserved14, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved15();
	AMX_DEFINE_NATIVE(pp__reserved15, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}

	// native pp__reserved16();
	AMX_DEFINE_NATIVE(pp__reserved16, 0)
	{
		throw errors::amx_error(AMX_ERR_NOTFOUND);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pp_version),
	AMX_DECLARE_NATIVE(pp_version_string),
	AMX_DECLARE_NATIVE(pp_version_string_s),
	AMX_DECLARE_NATIVE(pp_hook_strlen),
	AMX_DECLARE_NATIVE(pp_hook_check_ref_args),
	AMX_DECLARE_NATIVE(pp_public_min_index),
	AMX_DECLARE_NATIVE(pp_use_funcidx),
	AMX_DECLARE_NATIVE(pp_tick),
	AMX_DECLARE_NATIVE(pp_num_tasks),
	AMX_DECLARE_NATIVE(pp_num_local_strings),
	AMX_DECLARE_NATIVE(pp_num_global_strings),
	AMX_DECLARE_NATIVE(pp_num_local_variants),
	AMX_DECLARE_NATIVE(pp_num_global_variants),
	AMX_DECLARE_NATIVE(pp_num_lists),
	AMX_DECLARE_NATIVE(pp_num_linked_lists),
	AMX_DECLARE_NATIVE(pp_num_maps),
	AMX_DECLARE_NATIVE(pp_num_pools),
	AMX_DECLARE_NATIVE(pp_num_guards),
	AMX_DECLARE_NATIVE(pp_num_amx_guards),
	AMX_DECLARE_NATIVE(pp_num_local_iters),
	AMX_DECLARE_NATIVE(pp_num_global_iters),
	AMX_DECLARE_NATIVE(pp_num_local_handles),
	AMX_DECLARE_NATIVE(pp_num_global_handles),
	AMX_DECLARE_NATIVE(pp_num_local_expressions),
	AMX_DECLARE_NATIVE(pp_num_global_expressions),
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
	AMX_DECLARE_NATIVE(pp_locale),
	AMX_DECLARE_NATIVE(pp_locale_name),
	AMX_DECLARE_NATIVE(pp_locale_name_s),
	AMX_DECLARE_NATIVE(pp_format_env_push),
	AMX_DECLARE_NATIVE(pp_format_env_pop),
	AMX_DECLARE_NATIVE(pp_stackspace),

	//Reserved for private use
	AMX_DECLARE_NATIVE(pp__reserved1),
	AMX_DECLARE_NATIVE(pp__reserved2),
	AMX_DECLARE_NATIVE(pp__reserved3),
	AMX_DECLARE_NATIVE(pp__reserved4),
	AMX_DECLARE_NATIVE(pp__reserved5),
	AMX_DECLARE_NATIVE(pp__reserved6),
	AMX_DECLARE_NATIVE(pp__reserved7),
	AMX_DECLARE_NATIVE(pp__reserved8),
	AMX_DECLARE_NATIVE(pp__reserved9),
	AMX_DECLARE_NATIVE(pp__reserved10),
	AMX_DECLARE_NATIVE(pp__reserved11),
	AMX_DECLARE_NATIVE(pp__reserved12),
	AMX_DECLARE_NATIVE(pp__reserved13),
	AMX_DECLARE_NATIVE(pp__reserved14),
	AMX_DECLARE_NATIVE(pp__reserved15),
	AMX_DECLARE_NATIVE(pp__reserved16),
};

int RegisterConfigNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
