#include "natives.h"
#include "context.h"
#include "modules/strings.h"
#include <thread>
#include <sstream>

namespace Natives
{
	// native thread_detach(sync_flags:flags);
	static cell AMX_NATIVE_CALL thread_detach(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnDetach | (SleepReturnValueMask & params[1]);
	}

	// native thread_attach();
	static cell AMX_NATIVE_CALL thread_attach(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnAttach;
	}

	// native thread_sync();
	static cell AMX_NATIVE_CALL thread_sync(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnSync;
	}

	// native thread_sleep(ms);
	static cell AMX_NATIVE_CALL thread_sleep(AMX *amx, cell *params)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(params[1]));
		return 0;
	}

	// native thread_id(id[], size=sizeof id);
	static cell AMX_NATIVE_CALL thread_id(AMX *amx, cell *params)
	{
		std::ostringstream buf;
		buf << std::this_thread::get_id();
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		std::string str = buf.str();
		amx_SetString(addr, str.c_str(), false, false, params[2]);
		return str.size();
	}

	// native String:thread_id_s();
	static cell AMX_NATIVE_CALL thread_id_s(AMX *amx, cell *params)
	{
		std::ostringstream buf;
		buf << std::this_thread::get_id();
		return reinterpret_cast<cell>(strings::create(buf.str(), true));
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(thread_detach),
	AMX_DECLARE_NATIVE(thread_attach),
	AMX_DECLARE_NATIVE(thread_sync),
	AMX_DECLARE_NATIVE(thread_sleep),
	AMX_DECLARE_NATIVE(thread_id),
	AMX_DECLARE_NATIVE(thread_id_s),
};

int RegisterThreadNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
