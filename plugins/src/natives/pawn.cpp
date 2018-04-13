#include "../natives.h"
#include "../hooks.h"
#include "../events.h"
#include "../utils/dyn_object.h"
#include "../strings.h"
#include <memory>
#include <cstring>
#include <unordered_map>

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

		int numargs = format == nullptr ? 0 : std::strlen(format);

		if(params[0] < (2 + numargs) * static_cast<int>(sizeof(cell)))
		{
			logprintf("[PP] pawn_call_native: not enough arguments");
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}

		int index;
		if(amx_FindNative(amx, fname, &index) == AMX_ERR_NONE)
		{
			cell reset_stk = amx->stk;
			unsigned char *data = amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat;

			cell reset_hea, *tmp2;
			amx_Allot(amx, 0, &reset_hea, &tmp2);

			cell count = 0;

			std::unordered_map<dyn_object*, cell> storage;

			for(int i = numargs - 1; i >= 0; i--)
			{
				cell param = params[3 + i];
				cell *addr;

				switch(format[i])
				{
					case 'a':
					case 's':
					case '*':
						break;
					case 'S':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<strings::cell_string*>(*addr);
						size_t size = ptr->size();
						amx_Allot(amx, size + 1, &param, &addr);
						std::memcpy(addr, ptr->c_str(), size);
						addr[size] = 0;
						break;
					}
					case 'L':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<std::vector<dyn_object>*>(*addr);
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							amx->stk -= sizeof(cell);
							cell addr = it->store(amx);
							storage[&*it] = addr;
							*reinterpret_cast<cell*>(data + amx->stk) = addr;
							count++;
						}
						cell fmt_value;
						amx_Allot(amx, ptr->size() + 1, &fmt_value, &addr);
						for(auto it = ptr->begin(); it != ptr->end(); it++)
						{
							*(addr++) = it->get_specifier();
						}
						*addr = 0;
						amx->stk -= sizeof(cell);
						*reinterpret_cast<cell*>(data + amx->stk) = fmt_value;
						count++;
						continue;
					}
					case 'l':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<std::vector<dyn_object>*>(*addr);
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							amx->stk -= sizeof(cell);
							cell addr = it->store(amx);
							storage[&*it] = addr;
							*reinterpret_cast<cell*>(data + amx->stk) = addr;
							count++;
						}
						continue;
					}
					case 'v':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<dyn_object*>(*addr);
						param = ptr->store(amx);
						storage[ptr] = param;
						break;
					}
					default:
					{
						amx_GetAddr(amx, param, &addr);
						param = *addr;
						break;
					}
				}
				amx->stk -= sizeof(cell);
				*reinterpret_cast<cell*>(data + amx->stk) = param;
				count++;
			}

			amx->stk -= sizeof(cell);
			*reinterpret_cast<cell*>(data + amx->stk) = count * sizeof(cell);

			cell result;
			int ret = amx->callback(amx, index, &result, reinterpret_cast<cell*>(data + amx->stk));
			if(ret == AMX_ERR_NONE)
			{
				for(auto &pair : storage)
				{
					pair.first->load(amx, pair.second);
				}
			}else{
				result = -2;
			}
			amx->stk = reset_stk;
			amx_Release(amx, reset_hea);
			return result;
		}
		return -1;
	}

	// native pawn_call_public(const function[], const format[], {_,AmxString,Float}:...);
	static cell AMX_NATIVE_CALL pawn_call_public(AMX *amx, cell *params)
	{
		char *fname;
		amx_StrParam(amx, params[1], fname);

		char *format;
		amx_StrParam(amx, params[2], format);

		if(fname == nullptr) return -1;

		int numargs = format == nullptr ? 0 : std::strlen(format);

		if(params[0] < (2 + numargs) * static_cast<int>(sizeof(cell)))
		{
			logprintf("[PP] pawn_call_public: not enough arguments");
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}

		int index;
		if(amx_FindPublic(amx, fname, &index) == AMX_ERR_NONE)
		{
			cell reset_hea, *tmp2;
			amx_Allot(amx, 0, &reset_hea, &tmp2);

			std::unordered_map<dyn_object*, cell> storage;

			for(int i = numargs - 1; i >= 0; i--)
			{
				cell param = params[3 + i];
				cell *addr;

				switch(format[i])
				{
					case 'a':
					case 's':
					case '*':
					{
						amx_Push(amx, param);
						break;
					}
					case 'S':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<strings::cell_string*>(*addr);
						size_t size = ptr->size();
						amx_Allot(amx, size + 1, &param, &addr);
						std::memcpy(addr, ptr->c_str(), size);
						addr[size] = 0;
						amx_Push(amx, param);
						break;
					}
					case 'L':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<std::vector<dyn_object>*>(*addr);
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							cell addr = it->store(amx);
							storage[&*it] = addr;
							amx_Push(amx, addr);
						}
						cell fmt_value;
						amx_Allot(amx, ptr->size() + 1, &fmt_value, &addr);
						for(auto it = ptr->begin(); it != ptr->end(); it++)
						{
							*(addr++) = it->get_specifier();
						}
						*addr = 0;
						amx_Push(amx, fmt_value);
						break;
					}
					case 'l':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<std::vector<dyn_object>*>(*addr);
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							cell addr = it->store(amx);
							storage[&*it] = addr;
							amx_Push(amx, addr);
						}
						break;
					}
					case 'v':
					{
						amx_GetAddr(amx, param, &addr);
						auto ptr = reinterpret_cast<dyn_object*>(*addr);
						cell addr = ptr->store(amx);
						storage[ptr] = addr;
						amx_Push(amx, addr);
						break;
					}
					default:
					{
						amx_GetAddr(amx, param, &addr);
						param = *addr;
						amx_Push(amx, param);
						break;
					}
				}
			}

			cell result;
			int ret = amx_Exec(amx, &result, index);
			if(ret == AMX_ERR_NONE)
			{
				for(auto &pair : storage)
				{
					pair.first->load(amx, pair.second);
				}
			}else{
				result = -2;
			}
			amx_Release(amx, reset_hea);
			return result;
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
	AMX_DECLARE_NATIVE(pawn_call_public),
	AMX_DECLARE_NATIVE(pawn_register_callback),
	AMX_DECLARE_NATIVE(pawn_unregister_callback),
};

int RegisterPawnNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
