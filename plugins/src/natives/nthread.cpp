#include "../natives.h"
#include "../context.h"

namespace Natives
{
	// native thread_detach(sync_flags:flags);
	static cell AMX_NATIVE_CALL thread_detach(AMX *amx, cell *params)
	{
		auto &ctx = Context::Get(amx);
		ctx.sync_flags = params[1];
		ctx.pause_reason = PauseReason::Detach;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 1;
	}

	// native thread_attach();
	static cell AMX_NATIVE_CALL thread_attach(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return -1;
	}

	// native thread_sync();
	static cell AMX_NATIVE_CALL thread_sync(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return -2;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(thread_detach),
	AMX_DECLARE_NATIVE(thread_attach),
	AMX_DECLARE_NATIVE(thread_sync),
};

int RegisterThreadNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
