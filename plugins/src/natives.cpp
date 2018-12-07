#include "natives.h"

cell impl::handle_error(AMX *amx, cell *params, const char *native, const errors::native_error &error)
{
	logprintf("[PP] %s: %s", native, error.message.c_str());
	if(error.code)
	{
		amx_RaiseError(amx, error.code);
	}
	return 0;
}
