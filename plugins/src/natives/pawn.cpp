#include "../natives.h"
#include "../hooks.h"
#include "../events.h"
#include <memory>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

namespace Natives
{
	// native pawn_call_native(const function[], const format[], {_,AmxString,Float}:...);
	static cell AMX_NATIVE_CALL pawn_call_native(AMX *amx, cell *params)
	{
		char *fname;
		amx_StrParam(amx, params[1], fname);

		char *format;
		amx_StrParam(amx, params[2], format);

		if(fname == nullptr) return -1;

		int numargs = format == nullptr ? 0 : strlen(format);

		if(params[0] < (2 + numargs) * static_cast<int>(sizeof(cell)))
		{
			logprintf("[PP] pawn_call_native: not enough arguments");
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}

		int index;
		if(amx_FindNative(amx, fname, &index) == AMX_ERR_NONE)
		{
			params[2] = numargs * PAWN_CELL_SIZE;
			for(int i = 0; i < numargs; i++)
			{
				switch(format[i])
				{
					case 'a':
					case 's':
					case '*':
						break;
					default:
						cell *addr;
						amx_GetAddr(amx, params[3 + i], &addr);
						params[3 + i] = *addr;
						break;
				}
			}
			cell result;
			if(amx->callback(amx, index, &result, params + 2) == AMX_ERR_NONE)
			{
				return result;
			}
			return -2;
		}
		return -1;
	}

	// native callback:pawn_register_callback(const callback[], const function[], const additional_format[], {_,Float}:...);
	static cell AMX_NATIVE_CALL pawn_register_callback(AMX *amx, cell *params)
	{
		char *callback;
		amx_StrParam(amx, params[1], callback);

		char *fname;
		amx_StrParam(amx, params[2], fname);

		char *format;
		amx_StrParam(amx, params[3], format);

		if(callback == nullptr || fname == nullptr) return -1;

		int ret = Events::Register(callback, amx, fname, format, params + 4, (params[0] / static_cast<int>(sizeof(cell))) - 3);
		if(ret == -1)
		{
			logprintf("[PP] pawn_register_callback: not enough arguments");
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		return ret;
	}

	// pawn_unregister_callback(callback:id);
	static cell AMX_NATIVE_CALL pawn_unregister_callback(AMX *amx, cell *params)
	{
		return static_cast<cell>(Events::Remove(amx, params[1]));
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pawn_call_native),
	AMX_DECLARE_NATIVE(pawn_register_callback),
	AMX_DECLARE_NATIVE(pawn_unregister_callback),
};

int RegisterPawnNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
