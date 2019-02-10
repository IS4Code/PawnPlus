#include "natives.h"
#include "amxinfo.h"
#include "hooks.h"
#include "errors.h"
#include "modules/events.h"
#include "modules/containers.h"
#include "modules/amxhook.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/guards.h"
#include "modules/debug.h"
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

template <bool native, bool try_>
static cell AMX_NATIVE_CALL pawn_call(AMX *amx, cell *params)
{
	constexpr size_t args_start = try_ ? 3 : 2;

	char *fname;
	amx_StrParam(amx, params[1], fname);

	char *format;
	amx_StrParam(amx, params[args_start], format);

	if(fname == nullptr)
	{
		amx_FormalError(errors::arg_empty);
	}
	if(format == nullptr) format = "";
	int numargs = std::strlen(format);
	if(numargs > 0 && format[numargs - 1] == '+')
	{
		numargs--;
	}

	if(params[0] < (2 + numargs) * static_cast<int>(sizeof(cell)))
	{
		amx_FormalError(errors::not_enough_args, 2 + numargs, params[0] / static_cast<cell>(sizeof(cell)));
	}

	int pubindex = 0;
	AMX_NATIVE func = nullptr;

	if(native ? (func = amx::find_native(amx, fname)) == nullptr : amx_FindPublic(amx, fname, &pubindex) != AMX_ERR_NONE)
	{
		if(try_)
		{
			return AMX_ERR_NOTFOUND;
		}else{
			amx_FormalError(errors::func_not_found, native ? "native" : "public", fname);
		}
	}

	amx_stack stack(amx, native);
	std::unordered_map<dyn_object*, cell> storage;

	if(format[numargs] == '+')
	{
		for(int i = (params[0] / sizeof(cell)) - 1; i >= numargs; i--)
		{
			stack.push(params[args_start + 1 + i]);
		}
	}

	for(int i = numargs - 1; i >= 0; i--)
	{
		cell param = params[args_start + 1 + i];
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
				amx_FormalError("+ must be the last specifier");
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

	cell result = 0, *resultptr;
	if(try_)
	{
		amx_GetAddr(amx, params[2], &resultptr);
	}else{
		resultptr = &result;
	}

	int ret;
	amx->error = AMX_ERR_NONE;
	if(native)
	{
		*resultptr = func(amx, reinterpret_cast<cell*>(stack.data + amx->stk));
		ret = amx->error;
	}else{
		ret = amx_Exec(amx, resultptr, pubindex);
	}
	amx->error = AMX_ERR_NONE;
	for(auto &pair : storage)
	{
		pair.first->load(amx, pair.second);
	}
	stack.reset();
	if(try_)
	{
		return ret;
	}else{
		if(ret != AMX_ERR_NONE)
		{
			amx_LogicError(errors::inner_error, native ? "native" : "public", fname, ret, amx::StrError(ret));
		}
		return result;
	}
}

namespace Natives
{
	// native pawn_call_native(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_call_native, 2)
	{
		return pawn_call<true, false>(amx, params);
	}

	// native pawn_call_public(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_call_public, 2)
	{
		return pawn_call<false, false>(amx, params);
	}

	// native amx_err:pawn_try_call_native(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_try_call_native, 3)
	{
		return pawn_call<true, true>(amx, params);
	}

	// native amx_err:pawn_try_call_public(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_try_call_public, 3)
	{
		return pawn_call<false, true>(amx, params);
	}

	// native CallbackHandler:pawn_register_callback(const callback[], const function[], handler_flags:flags=handler_default, const additional_format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_register_callback, 2)
	{
		char *callback;
		amx_StrParam(amx, params[1], callback);

		char *fname;
		amx_StrParam(amx, params[2], fname);

		cell flags = optparam(3, 0);

		const char *format;
		amx_OptStrParam(amx, 4, format, "");

		if(callback == nullptr || fname == nullptr)
		{
			amx_FormalError(errors::arg_empty);
		}

		cell ret = events::register_callback(callback, flags, amx, fname, format, params + 5, (params[0] / static_cast<int>(sizeof(cell))) - 4);
		return ret;
	}

	// native pawn_unregister_callback(CallbackHandler:id);
	AMX_DEFINE_NATIVE(pawn_unregister_callback, 1)
	{
		return static_cast<cell>(events::remove_callback(amx, params[1]));
	}

	// native NativeHook:pawn_add_hook(const function[], const format[], const handler[], const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_add_hook, 3)
	{
		char *native;
		amx_StrParam(amx, params[1], native);

		char *native_format;
		amx_StrParam(amx, params[2], native_format);

		char *fname;
		amx_StrParam(amx, params[3], fname);

		const char *format;
		amx_OptStrParam(amx, 4, format, "");

		if(native == nullptr || fname == nullptr)
		{
			amx_FormalError(errors::arg_empty);
		}

		cell ret = amxhook::register_hook(amx, native, native_format, fname, format, params + 5, (params[0] / static_cast<int>(sizeof(cell))) - 4);
		return ret;
	}

	// native NativeHook:pawn_add_filter(const function[], const format[], const handler[], filter_type:type=filter_in, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_add_filter, 3)
	{
		char *native;
		amx_StrParam(amx, params[1], native);

		char *native_format;
		amx_StrParam(amx, params[2], native_format);

		char *fname;
		amx_StrParam(amx, params[3], fname);

		bool output = !!optparam(4, 0);

		char *format;
		amx_OptStrParam(amx, 5, format, "");

		if(native == nullptr || fname == nullptr)
		{
			amx_FormalError(errors::arg_empty);
		}

		cell ret = amxhook::register_filter(amx, output, native, native_format, fname, format, params + 6, (params[0] / static_cast<int>(sizeof(cell))) - 5);
		return ret;
	}

	// native pawn_remove_hook(NativeHook:id);
	AMX_DEFINE_NATIVE(pawn_remove_hook, 1)
	{
		return static_cast<cell>(amxhook::remove_hook(params[1]));
	}

	// native Guard:pawn_guard(AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pawn_guard, 2)
	{
		auto obj = dyn_object(amx, params[1], params[2]);
		if(obj.get_tag()->inherits_from(tags::tag_cell))
		{
			return 0;
		}
		return guards::get_id(amx, guards::add(amx, std::move(obj)));
	}

	// native Guard:pawn_guard_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pawn_guard_arr, 3)
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
	AMX_DEFINE_NATIVE(pawn_guard_valid, 1)
	{
		handle_t *obj;
		return guards::get_by_id(amx, params[1], obj);
	}

	// native bool:pawn_guard_free(Guard:guard);
	AMX_DEFINE_NATIVE(pawn_guard_free, 1)
	{
		handle_t *obj;
		if(!guards::get_by_id(amx, params[1], obj)) amx_LogicError(errors::pointer_invalid, "guard", params[1]);
		
		return guards::free(amx, obj);
	}
	
	// native List:pawn_get_args(const format[], bool:byref=false, level=0);
	AMX_DEFINE_NATIVE(pawn_get_args, 1)
	{
		char *format;
		amx_StrParam(amx, params[1], format);
		size_t numargs = format == nullptr ? 0 : std::strlen(format);
		if(numargs > 0 && format[numargs - 1] == '+')
		{
			numargs--;
		}

		bool byref = optparam(2, 0);

		auto hdr = (AMX_HEADER *)amx->base;
		auto data = amx->data ? amx->data : amx->base + (int)hdr->dat;

		cell level = optparam(3, 0);
		cell frm = amx->frm;
		for(cell i = 0; i < level; i++)
		{
			frm = *reinterpret_cast<cell*>(data + frm);
		}
		auto args = reinterpret_cast<cell*>(data + frm) + 2;

		size_t nfuncargs = static_cast<size_t>(args[0] / sizeof(cell));
		if(nfuncargs < numargs)
		{
			amx_FormalError(errors::not_enough_args, numargs, nfuncargs);
		}

		auto ptr = list_pool.add();

		if(format != nullptr) for(size_t i = 0; i <= numargs; i++)
		{
			tag_ptr tag = tags::find_tag(tags::tag_cell);

			if(i == numargs)
			{
				if(format[i] == '+')
				{
					cell *addr;
					for(size_t j = i; j < nfuncargs; j++)
					{
						if(amx_GetAddr(amx, args[1 + j], &addr) == AMX_ERR_NONE)
						{
							ptr->push_back(dyn_object(*addr, tag));
						}else{
							ptr->push_back(dyn_object());
						}
					}
				}
				break;
			}

			cell param = args[1 + i];
			cell *addr;

			switch(format[i])
			{
				case '*':
				{
					if(amx_GetAddr(amx, param, &addr) == AMX_ERR_NONE)
					{
						ptr->push_back(dyn_object(*addr, tag));
					}else{
						ptr->push_back(dyn_object());
					}
					continue;
				}
				case 'a':
				{
					if(++i < nfuncargs && amx_GetAddr(amx, param, &addr) == AMX_ERR_NONE)
					{
						cell len = args[1 + i], *lenaddr;
						if(!byref || amx_GetAddr(amx, len, &lenaddr) == AMX_ERR_NONE)
						{
							if(byref)
							{
								len = *lenaddr;
							}
							ptr->push_back(dyn_object(addr, len, tag));
							continue;
						}
					}
					ptr->push_back(dyn_object());
					continue;
				}
				case 's':
				{
					if(amx_GetAddr(amx, param, &addr) == AMX_ERR_NONE)
					{
						ptr->push_back(dyn_object(addr));
					}else{
						ptr->push_back(dyn_object());
					}
					continue;
				}
				case '+':
				{
					list_pool.remove(ptr.get());
					amx_FormalError("+ must be the last specifier");
					break;
				}
				case 'b':
				{
					tag = tags::find_tag(tags::tag_bool);
					break;
				}
				case 'f':
				{
					tag = tags::find_tag(tags::tag_float);
					break;
				}
				case 'S':
				{
					tag = tags::find_tag(tags::tag_string);
					break;
				}
			}
			if(byref)
			{
				if(amx_GetAddr(amx, param, &addr) == AMX_ERR_NONE)
				{
					param = *addr;
				}else{
					ptr->push_back(dyn_object());
					continue;
				}
			}
			ptr->push_back(dyn_object(param, tag));
		}

		return list_pool.get_id(ptr);
	}

	// native pawn_get_line();
	AMX_DEFINE_NATIVE(pawn_get_line, 0)
	{
		auto obj = amx::load_lock(amx);
		if(obj->has_extra<debug::info>())
		{
			auto dbg = obj->get_extra<debug::info>().dbg;
			long line;
			if(dbg_LookupLine(dbg, amx->cip, &line) == AMX_ERR_NONE)
			{
				return line;
			}
		}
		return -1;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pawn_call_native),
	AMX_DECLARE_NATIVE(pawn_call_public),
	AMX_DECLARE_NATIVE(pawn_try_call_native),
	AMX_DECLARE_NATIVE(pawn_try_call_public),
	AMX_DECLARE_NATIVE(pawn_register_callback),
	AMX_DECLARE_NATIVE(pawn_unregister_callback),
	AMX_DECLARE_NATIVE(pawn_add_hook),
	AMX_DECLARE_NATIVE(pawn_add_filter),
	AMX_DECLARE_NATIVE(pawn_remove_hook),
	AMX_DECLARE_NATIVE(pawn_guard),
	AMX_DECLARE_NATIVE(pawn_guard_arr),
	AMX_DECLARE_NATIVE(pawn_guard_valid),
	AMX_DECLARE_NATIVE(pawn_guard_free),
	AMX_DECLARE_NATIVE(pawn_get_args),
	AMX_DECLARE_NATIVE(pawn_get_line),
};

int RegisterPawnNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
