#include "natives.h"
#include "amxinfo.h"

tag_ptr native_return_tag;

cell impl::handle_error(AMX *amx, const cell *params, const char *native, size_t native_size, const errors::native_error &error)
{
	int handler;
	if(amx_FindPublicSafe(amx, "pp_on_error", &handler) == AMX_ERR_NONE)
	{
		try{
			amx::guard guard(amx);
			cell amx_addr, *addr;
			cell ret, *ret_addr;
			amx_AllotSafe(amx, 1, &ret, &ret_addr);
			*ret_addr = 0;
			amx_Push(amx, ret);
			amx_Push(amx, error.level);
			amx_AllotSafe(amx, error.message.size() + 1, &amx_addr, &addr);
			amx_SetString(addr, error.message.c_str(), false, false, UNLIMITED);
			amx_Push(amx, amx_addr);
			amx_AllotSafe(amx, native_size + 1, &amx_addr, &addr);
			amx_SetString(addr, native, false, false, UNLIMITED);
			amx_Push(amx, amx_addr);
			ret = 0;
			amx_Exec(amx, &ret, handler);
			cell retval = *ret_addr;

			if(ret || amx->error != AMX_ERR_NONE)
			{
				return retval;
			}
		}catch(const errors::amx_error &err)
		{
			logprintf("[PawnPlus] Error handler could not be invoked due to an AMX error %d: %s", err.code, amx::StrError(err.code));
			logprintf("[PawnPlus] %s: %s", native, error.message.c_str());
			amx_RaiseError(amx, err.code);
			return 0;
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
