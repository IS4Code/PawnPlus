#include "main.h"
#include "amxinfo.h"
#include "hooks.h"
#include "natives.h"
#include "context.h"
#include "modules/tasks.h"
#include "modules/events.h"
#include "modules/threads.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/containers.h"
#include "modules/tags.h"
#include "modules/debug.h"
#include "modules/expressions.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"

#include <list>
#include <limits>

logprintf_t logprintf;
extern void *pAMXFunctions;

thread_local bool is_main_thread = false;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() noexcept
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData) noexcept
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];
	is_main_thread = true;

	strings::set_locale(std::locale::classic(), -1);
	Hooks::Register();
	debug::init();

	amx::on_bottom(
		[](AMX *amx){
			tasks::run_pending();
			gc_collect();
			Threads::StartThreads();
		}
	);

	logprintf(" PawnPlus " PP_VERSION_STRING " loaded");
	logprintf(" Created by IllidanS4");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() noexcept
{
	variants::pool.clear();
	handle_pool.clear();
	list_pool.clear();
	map_pool.clear();
	linked_list_pool.clear();
	pool_pool.clear();
	expression_pool.clear();
	iter_pool.clear();
	tasks::clear();
	strings::pool.clear();

	Hooks::Unregister();

	logprintf(" PawnPlus " PP_VERSION_STRING " unloaded");
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) noexcept
{
	auto &obj = amx::load_lock(amx);
	RegisterNatives(amx);

	obj->loaded = true;
	obj->run_initializers();

	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) noexcept
{
	amx::invalidate(amx);
	amx::unload(amx);
	return AMX_ERR_NONE;
}

void pp_tick()
{
	tasks::tick();
	Threads::SyncThreads();
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() noexcept
{
	debug::clear_file();
	pp_tick();
}

std::list<void(*)()> gc_list;

void gc_collect()
{
	variants::pool.clear_tmp();
	handle_pool.clear_tmp();
	expression_pool.clear_tmp();
	iter_pool.clear_tmp();
	strings::pool.clear_tmp();
	for(const auto &it : gc_list)
	{
		it();
	}
}

void *gc_register(void(*func)())
{
	gc_list.push_back(func);
	auto it = gc_list.end();
	--it;
	return new decltype(it)(it);
}

void gc_unregister(void *id)
{
	auto it = static_cast<decltype(gc_list.end())*>(id);
	gc_list.erase(*it);
	delete it;
}

unsigned char *amx_GetData(AMX *amx)
{
	cell *addr;
	if(amx->hea > 0)
	{
		amx_GetAddr(amx, 0, &addr);
	}else{
		cell oldhea = amx->hea;
		amx->hea = sizeof(cell);
		amx_GetAddr(amx, 0, &addr);
		amx->hea = oldhea;
	}
	return reinterpret_cast<unsigned char*>(addr);
}

int public_min_index = -1;
bool use_funcidx = false;
bool disable_public_warning = false;

int amx_FindPublicSafe(AMX *amx, const char *funcname, int *index)
{
	if(!funcname)
	{
		funcname = "";
	}
	if(use_funcidx && index)
	{
		auto native = amx::find_native(amx, "funcidx");
		if(!native)
		{
			return AMX_ERR_NOTFOUND;
		}
		*index = amx::call_native(amx, native, funcname);
		if(*index == -1)
		{
			*index = std::numeric_limits<int>::max();
			return AMX_ERR_NOTFOUND;
		}
		return AMX_ERR_NONE;
	}else{
		int ret = amx_FindPublic(amx, funcname, index);
		if(ret == AMX_ERR_NONE && index)
		{
			if(public_min_index != -1 && *index < public_min_index)
			{
				*index = std::numeric_limits<int>::max();
				return AMX_ERR_NOTFOUND;
			}
			if(*index < 0 && !disable_public_warning)
			{
				logprintf("[PawnPlus] Warning: amx_FindPublic returned negative index %d for function '%s'. Possible collision with SAMPGDK detected. Use pp_public_min_index or pp_use_funcidx to remove this warning.", *index, funcname);
			}
		}
		return ret;
	}
}

int last_pubvar_index;

int amx_NameLengthSafe(AMX *amx)
{
	int len;
	if(amx_NameLength(amx, &len) != AMX_ERR_NONE)
	{
		return sNAMEMAX;
	}
	return len;
}
