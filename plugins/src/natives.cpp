#include "natives.h"

cell impl::handle_error(AMX *amx, cell *params, const char *native, const errors::native_error &error)
{
	int handler;
	if(amx_FindPublic(amx, "pp_on_error", &handler) == AMX_ERR_NONE)
	{
		cell amx_addr, *addr;
		cell reset_hea, *ret_addr;
		amx_Allot(amx, 1, &reset_hea, &ret_addr);
		*ret_addr = 0;
		amx_Push(amx, error.code);
		amx_Push(amx, reset_hea);
		amx_PushString(amx, &amx_addr, &addr, error.message.c_str(), false, false);
		amx_PushString(amx, &amx_addr, &addr, native, false, false);
		cell ret;
		amx_Exec(amx, &ret, handler);
		cell retval = *ret_addr;
		amx_Release(amx, reset_hea);

		if(ret)
		{
			return retval;
		}
	}

	logprintf("[PP] %s: %s", native, error.message.c_str());
	if(error.code)
	{
		amx_RaiseError(amx, error.code);
	}
	return 0;
}
