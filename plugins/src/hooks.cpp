#include "hooks.h"
#include "main.h"
#include "exec.h"
#include "amxinfo.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/events.h"
#include "modules/capi.h"
#include "modules/debug.h"
#include "modules/amxutils.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"
#include "subhook/subhook_private.h"

extern void *pAMXFunctions;

extern int ExecLevel;

const char *api_names[] = {
	"Align16",
	"Align32",
	"Align64",
	"Allot",
	"Callback",
	"Cleanup",
	"Clone",
	"Exec",
	"FindNative",
	"FindPublic",
	"FindPubVar",
	"FindTagId",
	"Flags",
	"GetAddr",
	"GetNative",
	"GetPublic",
	"GetPubVar",
	"GetString",
	"GetTag",
	"GetUserData",
	"Init",
	"InitJIT",
	"MemInfo",
	"NameLength",
	"NativeInfo",
	"NumNatives",
	"NumPublics",
	"NumPubVars",
	"NumTags",
	"Push",
	"PushArray",
	"PushString",
	"RaiseError",
	"Register",
	"Release",
	"SetCallback",
	"SetDebugHook",
	"SetString",
	"SetUserData",
	"StrLen",
	"UTF8Check",
	"UTF8Get",
	"UTF8Len",
	"UTF8Put"
};

template <int Index>
class amx_hook;

template <class FType>
class amx_hook_func;

template <class Ret, class... Args>
class amx_hook_func<Ret(*)(Args...)>
{
public:
	typedef Ret hook_ftype(Ret(*)(Args...), Args...);

	typedef Ret AMXAPI handler_ftype(Args...);

	template <int Index, hook_ftype *Handler>
	static Ret AMXAPI handler(Args... args)
	{
		//logdebug("[PawnPlus] [debug] amx_%s (%d)", api_names[Index], Index);
		auto trampoline = reinterpret_cast<handler_ftype*>(subhook_get_trampoline(amx_hook<Index>::hook()));
		if(!trampoline)
		{
			trampoline = fallback<Index>;
		}
		return Handler(trampoline, args...);
	}

	template <int Index>
	static Ret AMXAPI fallback(Args... args)
	{
		const auto &hook = amx_hook<Index>::hook();
		auto dst = subhook_read_dst(subhook_get_src(hook));
		auto olddst = subhook_get_dst(hook);
		auto func = reinterpret_cast<handler_ftype*>(subhook_get_src(hook));
		if(dst != olddst)
		{
			hook->dst = dst;
			subhook_remove(hook);
			auto ret = func(std::forward<Args>(args)...);
			subhook_install(hook);
			hook->dst = olddst;
			return ret;
		}else if(!dst)
		{
			return func(std::forward<Args>(args)...);
		}else{
			subhook_remove(hook);
			auto ret = func(std::forward<Args>(args)...);
			subhook_install(hook);
			return ret;
		}
	}
};

template <int Index>
class amx_hook
{
public:
	static subhook_t &hook()
	{
		static subhook_t val;
		return val;
	}

	template <class FType, typename amx_hook_func<FType>::hook_ftype *Func>
	struct ctl
	{
		static void load()
		{
			typename amx_hook_func<FType>::handler_ftype *hookfn = &amx_hook_func<FType>::template handler<Index, Func>;

			hook() = subhook_new(reinterpret_cast<void*>(((FType*)pAMXFunctions)[Index]), reinterpret_cast<void*>(hookfn), {});
			subhook_install(hook());
		}

		static void unload()
		{
			subhook_remove(hook());
			subhook_free(hook());
		}

		static void install()
		{
			subhook_install(hook());
		}

		static void uninstall()
		{
			subhook_remove(hook());
		}

		static FType orig()
		{
			if(subhook_is_installed(hook()))
			{
				auto trampoline = reinterpret_cast<FType>(subhook_get_trampoline(hook()));
				if(!trampoline)
				{
					return &amx_hook_func<FType>::template fallback<Index>;
				}
				return trampoline;
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
			return ret;
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
			decltype(strings::pool)::ref_container *str;
			if(strings::pool.get_by_addr(amx, amx_addr, str))
			{
				strings::pool.set_cache(*str);
				*phys_addr = &(**str)[0];
				return AMX_ERR_NONE;
			}
			decltype(variants::pool)::ref_container *var;
			if(variants::pool.get_by_addr(amx, amx_addr, var))
			{
				*phys_addr = &(**var)[0];
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
			decltype(strings::pool)::ref_container *str;
			if(strings::pool.get_by_addr(amx, amx_addr, str))
			{
				strings::pool.set_cache(*str);
				*phys_addr = &(**str)[0];
				return AMX_ERR_NONE;
			}
			decltype(variants::pool)::ref_container *var;
			if(variants::pool.get_by_addr(amx, amx_addr, var))
			{
				*phys_addr = &(**var)[0];
				return AMX_ERR_NONE;
			}
		}
		return ret;
	}

	int AMX_HOOK_FUNC(amx_StrLen, const cell *cstring, int *length)
	{
		const decltype(strings::pool)::ref_container *str;
		if(strings::pool.find_cache(cstring, str))
		{
			*length = (*str)->size();
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

	int AMX_HOOK_FUNC(amx_FindPublic, AMX *amx, const char *funcname, int *index)
	{
		if(index && funcname)
		{
			if(funcname[0] == 0x1B)
			{
				int num;
				if(amx_NumPublics(amx, &num) == AMX_ERR_NONE)
				{
					auto name = reinterpret_cast<const unsigned char *>(funcname + 1);
					cell value;
					if(amx_decode_value(name, reinterpret_cast<ucell&>(value)) && value < num && value >= 0)
					{
						*index = value;
						return AMX_ERR_NONE;
					}
				}
			}
			int custom_index = events::find_callback(amx, funcname);
			if(custom_index != -1)
			{
				*index = custom_index;
				return AMX_ERR_NONE;
			}
		}
		return base_func(amx, funcname, index);
	}

	int AMX_HOOK_FUNC(amx_NumPublics, AMX *amx, int *number)
	{
		int ret = base_func(amx, number);
		if(ret == AMX_ERR_NONE && number)
		{
			*number += events::num_callbacks(amx);
		}
		return ret;
	}

	int AMX_HOOK_FUNC(amx_GetPublic, AMX *amx, int index, char *funcname)
	{
		if(funcname)
		{
			auto name = events::callback_name(amx, index);
			if(name)
			{
				std::strcpy(funcname, name);
				return AMX_ERR_NONE;
			}
		}
		return base_func(amx, index, funcname);
	}

	int AMX_HOOK_FUNC(amx_NameLength, AMX *amx, int *length)
	{
		int ret = base_func(amx, length);
		if(ret == AMX_ERR_NONE && length)
		{
			events::name_length(amx, *length);
		}
		return ret;
	}

	int AMX_HOOK_FUNC(amx_GetPubVar, AMX *amx, int index, char *varname, cell *amx_addr)
	{
		int ret = base_func(amx, index, varname, amx_addr);
		if(ret == AMX_ERR_NONE)
		{
			last_pubvar_index = index;
		}
		return ret;
	}

	// Just "return 0;" in SA-MP, making it identical to other equivalent functions.
	int AMX_HOOK_FUNC(amx_Cleanup, AMX *amx)
	{
		if(amx && amx::valid(amx))
		{
			const auto &obj = amx::load_lock(amx);
			obj->run_finalizers();
		}
		return base_func(amx);
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

int AMXAPI amx_NumPublicsOrig(AMX *amx, int *number)
{
	return amx_Hook(NumPublics)::orig()(amx, number);
}

void Hooks::Register()
{
	amx_Hook(Init)::load();
	amx_Hook(Exec)::load();
	amx_Hook(GetAddr)::load();
	amx_Hook(StrLen)::load();
	amx_Hook(Register)::load();
	amx_Hook(Flags)::load();
	amx_Hook(FindPublic)::load();
	amx_Hook(GetPublic)::load();
	amx_Hook(NumPublics)::load();
	amx_Hook(NameLength)::load();
	amx_Hook(GetPubVar)::load();
	amx_Hook(Cleanup)::load();
}

void Hooks::Unregister()
{
	amx_Hook(Init)::unload();
	amx_Hook(Exec)::unload();
	amx_Hook(GetAddr)::unload();
	amx_Hook(StrLen)::unload();
	amx_Hook(Register)::unload();
	amx_Hook(Flags)::unload();
	amx_Hook(FindPublic)::unload();
	amx_Hook(GetPublic)::unload();
	amx_Hook(NumPublics)::unload();
	amx_Hook(NameLength)::unload();
	amx_Hook(GetPubVar)::unload();
	amx_Hook(Cleanup)::unload();
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
