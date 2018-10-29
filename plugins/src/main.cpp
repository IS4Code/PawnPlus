#include "main.h"
#include "amxinfo.h"
#include "hooks.h"
#include "natives.h"
#include "context.h"
#include "pluginapi.h"
#include "capi.h"
#include "modules/tasks.h"
#include "modules/events.h"
#include "modules/threads.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/containers.h"
#include "modules/tags.h"

#include "sdk/amx/amx.h"
#include "sdk/plugincommon.h"

logprintf_t logprintf;
extern void *pAMXFunctions;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() 
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppData[PLUGIN_DATA_LOGPRINTF];

	InitPluginApi(ppData);
	RegisterPlugin("com.is4.pawnplus.api1", &capi::info);

	Hooks::Register();

	amx::on_bottom(
		[](AMX *amx){
			tasks::run_pending();
			gc_collect();
			Threads::StartThreads();
		}
	);

	logprintf(" PawnPlus v0.10.1 loaded");
	logprintf(" Created by IllidanS4");
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	variants::pool.clear();
	list_pool.clear();
	map_pool.clear();
	linked_list_pool.clear();
	iter_pool.clear();
	tasks::clear();
	strings::pool.clear();

	Hooks::Unregister();

	logprintf(" PawnPlus v0.10.1 unloaded");
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx) 
{
	amx::load(amx);
	RegisterNatives(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx) 
{
	amx::invalidate(amx);
	amx::unload(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	tasks::tick();
	Threads::SyncThreads();
}

void gc_collect()
{
	variants::pool.clear_tmp();
	iter_pool.clear_tmp();
	strings::pool.clear_tmp();
}
