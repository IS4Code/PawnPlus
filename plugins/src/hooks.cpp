#include "hooks.h"
#include "tasks.h"
#include "strings.h"
#include "context.h"
#include "events.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

#include <cstring>
#include <unordered_map>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

extern void *pAMXFunctions;

extern int ExecLevel;

subhook_t amx_Exec_h;
subhook_t amx_GetAddr_h;
subhook_t amx_StrLen_h;
subhook_t amx_FindPublic_h;

std::unordered_map<AMX*, AMX_CALLBACK> originalCallbacks;

int AMXAPI amx_CustomCallback(AMX *amx, cell index, cell *result, cell *params)
{
	auto it = originalCallbacks.find(amx);
	if(it != originalCallbacks.end())
	{
		subhook_install(amx_Exec_h);
		amx->callback = it->second;
		originalCallbacks.erase(it);
		return amx->callback(amx, index, result, params);
	}else{
		logprintf("[PP] callback hook cannot be restored");
		return amx_RaiseError(amx, AMX_ERR_CALLBACK);
	}
}

int AMXAPI amx_ExecOrig(AMX *amx, cell *retval, int index)
{
	if(subhook_is_installed(amx_Exec_h))
	{
		originalCallbacks[amx] = amx->callback;
		amx->callback = amx_CustomCallback;

		subhook_remove(amx_Exec_h);
		int ret = amx_Exec(amx, retval, index);
		subhook_install(amx_Exec_h);

		auto it = originalCallbacks.find(amx);
		if(it != originalCallbacks.end())
		{
			amx->callback = it->second;
			originalCallbacks.erase(it);
		}

		return ret;
	}else{
		return amx_Exec(amx, retval, index);
	}
}

int AMXAPI amx_FindPublicOrig(AMX *amx, const char *funcname, int *index)
{
	if(subhook_is_installed(amx_FindPublic_h))
	{
		subhook_remove(amx_FindPublic_h);
		int ret = ::amx_FindPublic(amx, funcname, index);
		subhook_install(amx_FindPublic_h);
		return ret;
	}else{
		return amx_FindPublic(amx, funcname, index);
	}
}

namespace Hooks
{
	int AMXAPI amx_Exec(AMX *amx, cell *retval, int index)
	{
		if(index <= -3)
		{
			auto name = Events::Invoke(-3 - index, amx, retval);
			if(name != nullptr)
			{
				int err = amx_FindPublicOrig(amx, name, &index);
				if(err) return err;
			}
		}

		Context::Push(amx);
		int ret = amx_ExecOrig(amx, retval, index);
		if(ret == AMX_ERR_SLEEP)
		{
			auto &ctx = Context::Get(amx);
			if(ctx.awaiting_task != -1)
			{
				TaskPool::Get(ctx.awaiting_task)->Register(AMX_RESET(amx));
				*retval = ctx.result;
				amx->error = ret = AMX_ERR_NONE;
			}
		}
		Context::Pop(amx);
		return ret;
	}

	int AMXAPI amx_GetAddr(AMX *amx, cell amx_addr, cell **phys_addr)
	{
		subhook_remove(amx_GetAddr_h);
		int ret = ::amx_GetAddr(amx, amx_addr, phys_addr);
		subhook_install(amx_GetAddr_h);

		if(ret == AMX_ERR_MEMACCESS)
		{
			if(Strings::IsNullAddress(amx, amx_addr))
			{
				Strings::NullValue1[0] = 0;
				*phys_addr = Strings::NullValue1;
				return AMX_ERR_NONE;
			}
			auto ptr = Strings::Get(amx, amx_addr);
			if(ptr != nullptr)
			{
				Strings::SetCache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}else if(ret == 0)
		{
			if(Strings::IsNullAddress(amx, **phys_addr))
			{
				Strings::NullValue2[0] = 1;
				Strings::NullValue2[1] = 0;
				*phys_addr = Strings::NullValue2;
				return AMX_ERR_NONE;
			}
			auto ptr = Strings::Get(amx, **phys_addr);
			if(ptr != nullptr)
			{
				Strings::SetCache(ptr);
				*phys_addr = &(*ptr)[0];
				return AMX_ERR_NONE;
			}
		}
		return ret;
	}

	int AMXAPI amx_StrLen(const cell *cstring, int *length)
	{
		auto str = Strings::FindCache(cstring);
		if(str != nullptr)
		{
			*length = str->size();
			return AMX_ERR_NONE;
		}

		subhook_remove(amx_StrLen_h);
		int ret = ::amx_StrLen(cstring, length);
		subhook_install(amx_StrLen_h);
		return ret;
	}

	int AMXAPI amx_FindPublic(AMX *amx, const char *funcname, int *index)
	{
		int id = Events::GetCallbackId(amx, funcname);
		if(id != -1)
		{
			*index = -3 - id;
			return AMX_ERR_NONE;
		}

		return amx_FindPublicOrig(amx, funcname, index);
	}
}

template <class Func>
void RegisterAmxHook(subhook_t &hook, int index, Func *fnptr)
{
	hook = subhook_new(((Func**)pAMXFunctions)[index], fnptr, {});
	subhook_install(hook);
}

void Hooks::Register()
{
	RegisterAmxHook(amx_Exec_h, PLUGIN_AMX_EXPORT_Exec, &Hooks::amx_Exec);
	RegisterAmxHook(amx_GetAddr_h, PLUGIN_AMX_EXPORT_GetAddr, &Hooks::amx_GetAddr);
	RegisterAmxHook(amx_StrLen_h, PLUGIN_AMX_EXPORT_StrLen, &Hooks::amx_StrLen);
	RegisterAmxHook(amx_FindPublic_h, PLUGIN_AMX_EXPORT_FindPublic, &Hooks::amx_FindPublic);
}

void UnregisterHook(subhook_t hook)
{
	subhook_remove(hook);
	subhook_free(hook);
}

void Hooks::Unregister()
{
	UnregisterHook(amx_Exec_h);
	UnregisterHook(amx_GetAddr_h);
	UnregisterHook(amx_StrLen_h);
	UnregisterHook(amx_FindPublic_h);
}

void Hooks::ToggleStrLen(bool toggle)
{
	if(toggle)
	{
		subhook_install(amx_StrLen_h);
	}else{
		subhook_remove(amx_StrLen_h);
	}
}
