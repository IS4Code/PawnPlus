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

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"

#include <list>

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
	iter_pool.clear();
	tasks::clear();
	strings::pool.clear();

	Hooks::Unregister();

	logprintf(" PawnPlus " PP_VERSION_STRING " unloaded");
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) noexcept
{
	amx::load(amx);
	RegisterNatives(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) noexcept
{
	amx::invalidate(amx);
	amx::unload(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() noexcept
{
	tasks::tick();
	Threads::SyncThreads();
}

std::list<void(*)()> gc_list;

void gc_collect()
{
	variants::pool.clear_tmp();
	handle_pool.clear_tmp();
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
