#include "natives.h"
#include "amxinfo.h"
#include "hooks.h"
#include "modules/events.h"
#include "modules/containers.h"
#include "modules/amxhook.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "objects/dyn_object.h"
#include <memory>
#include <cstring>
#include <unordered_map>

struct amx_stack
{
	cell reset_stk, reset_hea;
	AMX *amx;
	unsigned char *data;
	bool native;
	int count = 0;

	amx_stack(AMX *amx, bool native) : amx(amx), native(native)
	{
		data = amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat;
		if(native)
		{
			cell *tmp2;
			reset_stk = amx->stk;
			amx_Allot(amx, 0, &reset_hea, &tmp2);
		}
	}

	void push(cell value)
	{
		if(native)
		{
			amx->stk -= sizeof(cell);
			*reinterpret_cast<cell*>(data + amx->stk) = value;
			count++;
		}else{
			amx_Push(amx, value);
		}
	}

	void done()
	{
		if(native)
		{
			amx->stk -= sizeof(cell);
			*reinterpret_cast<cell*>(data + amx->stk) = count * sizeof(cell);
		}
	}

	void reset()
	{
		if(native)
		{
			amx->stk = reset_stk;
		}
		amx_Release(amx, reset_hea);
	}
};

template <bool native>
static cell AMX_NATIVE_CALL pawn_call(AMX *amx, cell *params)
{
	char *fname;
	amx_StrParam(amx, params[1], fname);

	char *format;
	amx_StrParam(amx, params[2], format);

	if(fname == nullptr) return -1;

	int numargs = format == nullptr ? 0 : std::strlen(format);
	if(numargs > 0 && format[numargs - 1] == '+')
	{
		numargs--;
	}

	if(params[0] < (2 + numargs) * static_cast<int>(sizeof(cell)))
	{
		if(native)
		{
			logerror(amx, "[PP] pawn_call_native: not enough arguments");
		}else{
			logerror(amx, "[PP] pawn_call_public: not enough arguments");
		}
		return 0;
	}

	int pubindex = 0;
	AMX_NATIVE func = nullptr;

	if(native ? (func = amx::find_native(amx, fname)) != nullptr : !amx_FindPublic(amx, fname, &pubindex))
	{
		amx_stack stack(amx, native);
		std::unordered_map<dyn_object*, cell> storage;

		if(format[numargs] == '+')
		{
			for(int i = (params[0] / sizeof(cell)) - 1; i >= numargs; i--)
			{
				stack.push(params[3 + i]);
			}
		}

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
					stack.push(param);
					break;
				}
				case 'S':
				{
					amx_GetAddr(amx, param, &addr);
					strings::cell_string *ptr;
					if(strings::pool.get_by_id(*addr, ptr))
					{
						size_t size = ptr->size();
						amx_Allot(amx, size + 1, &param, &addr);
						std::memcpy(addr, ptr->c_str(), size * sizeof(cell));
						addr[size] = 0;
					}else{
						amx_Allot(amx, 1, &param, &addr);
						addr[0] = 0;
					}
					stack.push(param);
					break;
				}
				case 'L':
				{
					amx_GetAddr(amx, param, &addr);
					list_t *ptr;
					if(list_pool.get_by_id(*addr, ptr))
					{
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							cell addr = it->store(amx);
							storage[&*it] = addr;
							stack.push(addr);
						}
						cell fmt_value;
						amx_Allot(amx, ptr->size() + 1, &fmt_value, &addr);
						for(auto it = ptr->begin(); it != ptr->end(); it++)
						{
							*(addr++) = it->get_specifier();
						}
						*addr = 0;
						stack.push(fmt_value);
					}else{
						cell fmt_value;
						amx_Allot(amx, 1, &fmt_value, &addr);
						addr[0] = 0;
						stack.push(fmt_value);
					}
					break;
				}
				case 'l':
				{
					amx_GetAddr(amx, param, &addr);
					list_t *ptr;
					if(list_pool.get_by_id(*addr, ptr))
					{
						for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
						{
							cell addr = it->store(amx);
							storage[&*it] = addr;
							stack.push(addr);
						}
					}
					break;
				}
				case 'v':
				{
					amx_GetAddr(amx, param, &addr);
					dyn_object *ptr;
					if(variants::pool.get_by_id(*addr, ptr))
					{
						param = ptr->store(amx);
						storage[ptr] = param;
					}else{
						param = 0;
					}
					stack.push(param);
					break;
				}
				case '+':
				{
					if(native)
					{
						logerror(amx, "[PP] pawn_call_native: + must be the last specifier");
					}else{
						logerror(amx, "[PP] pawn_call_public: + must be the last specifier");
					}
					break;
				}
				default:
				{
					amx_GetAddr(amx, param, &addr);
					param = *addr;
					stack.push(param);
					break;
				}
			}
		}

		stack.done();

		cell result;
		int ret;
		if(native)
		{
			amx->error = AMX_ERR_NONE;
			result = func(amx, reinterpret_cast<cell*>(stack.data + amx->stk));
			ret = amx->error;
		}else{
			ret = amx_Exec(amx, &result, pubindex);
		}
		if(ret == AMX_ERR_NONE)
		{
			for(auto &pair : storage)
			{
				pair.first->load(amx, pair.second);
			}
		} else {
			result = -2;
		}
		stack.reset();
		return result;
	}
	return -1;
}

namespace Natives
{
	// native pawn_call_native(const function[], const format[], AnyTag:...);
	static cell AMX_NATIVE_CALL pawn_call_native(AMX *amx, cell *params)
	{
		return pawn_call<true>(amx, params);
	}

	// native pawn_call_public(const function[], const format[], AnyTag:...);
	static cell AMX_NATIVE_CALL pawn_call_public(AMX *amx, cell *params)
	{
		return pawn_call<false>(amx, params);
	}

	// native CallbackHandler:pawn_register_callback(const callback[], const function[], const additional_format[], AnyTag:...);
	static cell AMX_NATIVE_CALL pawn_register_callback(AMX *amx, cell *params)
	{
		char *callback;
		amx_StrParam(amx, params[1], callback);

		char *fname;
		amx_StrParam(amx, params[2], fname);

		char *format;
		amx_StrParam(amx, params[3], format);

		if(callback == nullptr || fname == nullptr) return -1;

		int ret = events::register_callback(callback, amx, fname, format, params + 4, (params[0] / static_cast<int>(sizeof(cell))) - 3);
		if(ret == -1)
		{
			logerror(amx, "[PP] pawn_register_callback: not enough arguments");
			return 0;
		}
		return ret;
	}

	// native pawn_unregister_callback(CallbackHandler:id);
	static cell AMX_NATIVE_CALL pawn_unregister_callback(AMX *amx, cell *params)
	{
		return static_cast<cell>(events::remove_callback(amx, params[1]));
	}

	// native NativeHook:pawn_native_filter(const function[], const format[], bool:output=false, const handler[], const additional_format[], AnyTag:...);
	static cell AMX_NATIVE_CALL pawn_native_filter(AMX *amx, cell *params)
	{
		char *native;
		amx_StrParam(amx, params[1], native);

		char *native_format;
		amx_StrParam(amx, params[2], native_format);

		bool post = !!params[3];

		char *fname;
		amx_StrParam(amx, params[4], fname);

		char *format;
		amx_StrParam(amx, params[5], format);

		if(native == nullptr || fname == nullptr) return -1;

		int ret = amxhook::register_hook(amx, post, native, native_format, fname, format, params + 5, (params[0] / static_cast<int>(sizeof(cell))) - 4);
		if(ret == -1)
		{
			logerror(amx, "[PP] pawn_set_native_filter: not enough arguments");
			return 0;
		}
		return ret;
	}

	// native pawn_remove_hook(NativeHook:id);
	static cell AMX_NATIVE_CALL pawn_remove_hook(AMX *amx, cell *params)
	{
		return static_cast<cell>(amxhook::remove_hook(amx, params[1]));
	}

	// native Guard:pawn_guard(AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL pawn_guard(AMX *amx, cell *params)
	{
		auto obj = dyn_object(amx, params[1], params[2]);
		if(obj.get_tag()->inherits_from(tags::tag_cell))
		{
			return 0;
		}
		return guards::get_id(amx, guards::add(amx, std::move(obj)));
	}

	// native Guard:pawn_guard_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL pawn_guard_arr(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		auto obj = dyn_object(amx, addr, params[2], params[3]);
		if(obj.get_tag()->inherits_from(tags::tag_cell))
		{
			return 0;
		}
		return guards::get_id(amx, guards::add(amx, std::move(obj)));
	}

	// native bool:pawn_guard_valid(Guard:guard);
	static cell AMX_NATIVE_CALL pawn_guard_valid(AMX *amx, cell *params)
	{
		dyn_object *obj;
		return guards::get_by_id(amx, params[1], obj);
	}

	// native bool:pawn_guard_free(Guard:guard);
	static cell AMX_NATIVE_CALL pawn_guard_free(AMX *amx, cell *params)
	{
		dyn_object *obj;
		if(guards::get_by_id(amx, params[1], obj))
		{
			return guards::free(amx, obj);
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pawn_call_native),
	AMX_DECLARE_NATIVE(pawn_call_public),
	AMX_DECLARE_NATIVE(pawn_register_callback),
	AMX_DECLARE_NATIVE(pawn_unregister_callback),
	AMX_DECLARE_NATIVE(pawn_native_filter),
	AMX_DECLARE_NATIVE(pawn_remove_hook),
	AMX_DECLARE_NATIVE(pawn_guard),
	AMX_DECLARE_NATIVE(pawn_guard_arr),
	AMX_DECLARE_NATIVE(pawn_guard_valid),
	AMX_DECLARE_NATIVE(pawn_guard_free),
};

int RegisterPawnNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
