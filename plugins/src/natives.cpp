#include "natives.h"
#include "amxinfo.h"

tag_ptr native_return_tag;

cell impl::handle_error(AMX *amx, const cell *params, const char *native, const errors::native_error &error)
{
	int handler;
	if(amx_FindPublicSafe(amx, "pp_on_error", &handler) == AMX_ERR_NONE)
	{
		cell amx_addr, *addr;
		cell reset_hea, *ret_addr;
		amx_Allot(amx, 1, &reset_hea, &ret_addr);
		*ret_addr = 0;
		amx_Push(amx, reset_hea);
		amx_Push(amx, error.level);
		amx_PushString(amx, &amx_addr, &addr, error.message.c_str(), false, false);
		amx_PushString(amx, &amx_addr, &addr, native, false, false);
		cell ret = 0;
		amx_Exec(amx, &ret, handler);
		cell retval = *ret_addr;
		amx_Release(amx, reset_hea);

		if(ret || amx->error != AMX_ERR_NONE)
		{
			return retval;
		}
	}

	logprintf("[PawnPlus] %s: %s", native, error.message.c_str());
	if(error.level >= amx::load_lock(amx)->get_extra<native_error_level>().level)
	{
		amx_RaiseError(amx, AMX_ERR_NATIVE);
	}
	return 0;
}

std::unordered_map<AMX_NATIVE, impl::runtime_native_info> &impl::runtime_native_map()
{
	static std::unordered_map<AMX_NATIVE, impl::runtime_native_info> map;
	return map;
}
