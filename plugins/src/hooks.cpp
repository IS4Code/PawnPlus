#include "hooks.h"
#include "main.h"
#include "exec.h"
#include "amxinfo.h"
#include "modules/strings.h"
#include "modules/events.h"
#include "modules/capi.h"
#include "modules/debug.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

extern void *pAMXFunctions;

extern int ExecLevel;

template <class FType>
class amx_hook_func;

template <class Ret, class... Args>
class amx_hook_func<Ret(*)(Args...)>
{
public:
	typedef Ret hook_ftype(Ret(*)(Args...), Args...);

	typedef Ret AMXAPI handler_ftype(Args...);

	template <subhook_t &Hook, hook_ftype *Handler>
	static Ret AMXAPI handler(Args... args)
	{
		return Handler(reinterpret_cast<Ret(*)(Args...)>(subhook_get_trampoline(Hook)), args...);
	}
};

template <int Index>
class amx_hook;

template <int Index>
class amx_hook_var
{
	friend class amx_hook<Index>;

	//GCC causes issues when this member is inside amx_hook
	static subhook_t hook;
};

template <int Index>
subhook_t amx_hook_var<Index>::hook;

template <int Index>
class amx_hook
{
	typedef amx_hook_var<Index> var;

public:
	template <class FType, typename amx_hook_func<FType>::hook_ftype *Func>
	struct ctl
	{
		static void load()
		{
			typename amx_hook_func<FType>::handler_ftype *hookfn = &amx_hook_func<FType>::template handler<var::hook, Func>;

			var::hook = subhook_new(reinterpret_cast<void*>(((FType*)pAMXFunctions)[Index]), reinterpret_cast<void*>(hookfn), {});
			subhook_install(var::hook);
		}

		static void unload()
		{
			subhook_remove(var::hook);
			subhook_free(var::hook);
		}

		static void install()
		{
			subhook_install(var::hook);
		}

		static void uninstall()
		{
			subhook_remove(var::hook);
		}

		static FType orig()
		{
			if(subhook_is_installed(var::hook))
			{
				return reinterpret_cast<FType>(subhook_get_trampoline(var::hook));
			}else{
				return ((FType*)pAMXFunctions)[Index];
			}
		}
	};
};

#define AMX_HOOK_FUNC(Func, ...) Func(decltype(&::Func) _base_func, __VA_ARGS__) noexcept
#define base_func _base_func
#define amx_Hook(Func) amx_hook<PLUGIN_AMX_EXPORT_##Func>::ctl<decltype(&::amx_##Func), &Hooks::amx_##Func>

bool hook_ref_args = false;

namespace Hooks
{
	int AMX_HOOK_FUNC(amx_Init, AMX *amx, void *program)
	{
		amx->base = (unsigned char*)program;
		amx::load_lock(amx)->get_extra<amx_code_info>();
		amx->base = nullptr;
		int ret = base_func(amx, program);
		if(ret != AMX_ERR_NONE)
		{
			amx::unload(amx);
		}

		std::unique_ptr<char[]> name;
		auto dbg = debug::create_last(name);
		if(dbg)
		{
			if(((AMX_HEADER*)amx->base)->flags & AMX_FLAG_DEBUG)
			{
				amx::load_lock(amx)->dbg = dbg;
			}
		}
		if(name)
		{
			amx::load_lock(amx)->name = name.get();
		}
		return ret;
	}

	int AMX_HOOK_FUNC(amx_Exec, AMX *amx, cell *retval, int index)
	{
		if(amx && (amx->flags & AMX_FLAG_BROWSE) == 0)
		{
			return amx_ExecContext(amx, retval, index, false, nullptr);
		}
		return base_func(amx, retval, index);
	}

	int AMX_HOOK_FUNC(amx_GetAddr, AMX *amx, cell amx_addr, cell **phys_addr)
	{
		int ret = base_func(amx, amx_addr, phys_addr);

		if(ret == AMX_ERR_MEMACCESS)
		{
			if(strings::pool.is_null_address(amx, amx_addr))
			{
				strings::null_value1[0] = 0;
				*phys_addr = strings::null_value1;
				return AMX_ERR_NONE;
			}
			auto &ptr = strings::pool.get(amx, amx_addr);
			if(ptr != nullptr)
			{
				strings::pool.set_cache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}else if(ret == 0 && hook_ref_args)
		{
			// Variadic functions pass all arguments by ref
			// so checking the actual cell value is necessary,
			// but there is a chance that it will interpret
			// a number as a string. Better have it disabled by default.
			if(strings::pool.is_null_address(amx, **phys_addr))
			{
				strings::null_value2[0] = 1;
				strings::null_value2[1] = 0;
				*phys_addr = strings::null_value2;
				return AMX_ERR_NONE;
			}
			auto &ptr = strings::pool.get(amx, **phys_addr);
			if(ptr != nullptr)
			{
				strings::pool.set_cache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}
		return ret;
	}

	int AMX_HOOK_FUNC(amx_StrLen, const cell *cstring, int *length)
	{
		auto &str = strings::pool.find_cache(cstring);
		if(str != nullptr)
		{
			*length = str->size();
			return AMX_ERR_NONE;
		}

		return base_func(cstring, length);
	}

	int AMX_HOOK_FUNC(amx_Register, AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
	{
		int ret = base_func(amx, nativelist, number);
		amx::register_natives(amx, nativelist, number);
		return ret;
	}

	int AMX_HOOK_FUNC(amx_Flags, AMX *amx, uint16_t *flags)
	{
		if(amx && flags && *flags == pp::hook_flags_initial)
		{
			auto hdr = (AMX_HEADER*)amx->base;
			if(hdr && hdr->magic == pp::hook_magic)
			{
				*flags = pp::hook_flags_final;
				return reinterpret_cast<int>(get_api_table());
			}
		}
		return base_func(amx, flags);
	}
}

int AMXAPI amx_InitOrig(AMX *amx, void *program)
{
	return amx_Hook(Init)::orig()(amx, program);
}

int AMXAPI amx_ExecOrig(AMX *amx, cell *retval, int index)
{
	return amx_Hook(Exec)::orig()(amx, retval, index);
}

void Hooks::Register()
{
	amx_Hook(Init)::load();
	amx_Hook(Exec)::load();
	amx_Hook(GetAddr)::load();
	amx_Hook(StrLen)::load();
	amx_Hook(Register)::load();
	amx_Hook(Flags)::load();
}

void Hooks::Unregister()
{
	amx_Hook(Init)::unload();
	amx_Hook(Exec)::unload();
	amx_Hook(GetAddr)::unload();
	amx_Hook(StrLen)::unload();
	amx_Hook(Register)::unload();
	amx_Hook(Flags)::unload();
}

void Hooks::ToggleStrLen(bool toggle)
{
	if(toggle)
	{
		amx_Hook(StrLen)::install();
	}else{
		amx_Hook(StrLen)::uninstall();
	}
}

void Hooks::ToggleRefArgs(bool toggle)
{
	hook_ref_args = toggle;
}
