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
		data = amx_GetData(amx);
		if(native)
		{
			cell *tmp2;
			reset_stk = amx->stk;
			amx_AllotSafe(amx, 0, &reset_hea, &tmp2);
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

cell pawn_call(AMX *amx, cell paramsize, cell *params, bool native, bool try_, std::string *msg, AMX *target_amx)
{
	size_t args_start = try_ ? 3 : 2;

	char *fname;
	amx_StrParam(amx, params[0], fname);

	char *format;
	amx_StrParam(amx, params[args_start - 1], format);

	if(fname == nullptr)
	{
		amx_FormalError(errors::arg_empty, "function");
	}
	if(format == nullptr) format = "";
	int numargs = std::strlen(format);
	if(numargs > 0 && format[numargs - 1] == '+')
	{
		numargs--;
	}

	if(paramsize < (2 + numargs) * static_cast<int>(sizeof(cell)))
	{
		throw errors::end_of_arguments_error(params, 2 + numargs);
	}

	int pubindex = 0;
	AMX_NATIVE func = nullptr;

	if(native ? (func = amx::find_native(target_amx, fname)) == nullptr : amx_FindPublicSafe(target_amx, fname, &pubindex) != AMX_ERR_NONE)
	{
		if(try_)
		{
			return AMX_ERR_NOTFOUND;
		}else{
			amx_FormalError(errors::func_not_found, native ? "native" : "public", fname);
		}
	}

	amx_stack stack(target_amx, native);
	std::unordered_map<dyn_object*, cell> storage;

	if(format[numargs] == '+')
	{
		for(int i = (paramsize / sizeof(cell)) - 1; i >= numargs; i--)
		{
			cell param = params[args_start + i];
			if(amx != target_amx)
			{
				cell *addr = amx_GetAddrSafe(amx, param);
				int length;
				amx_StrLen(addr, &length);
				if(addr[0] & 0xFF000000)
				{
					length = 1 + ((length - 1) / sizeof(cell));
				}
				length += 1;
				cell *newaddr;
				amx_AllotSafe(target_amx, length, &param, &newaddr);
				std::memcpy(newaddr, addr, length * sizeof(cell));
			}
			stack.push(param);
		}
	}

	for(int i = numargs - 1; i >= 0; i--)
	{
		cell param = params[args_start + i];
		cell *addr;

		switch(format[i])
		{
			case 'a':
			case 's':
			case '*':
			{
				if(amx != target_amx)
				{
					addr = amx_GetAddrSafe(amx, param);
					int length;
					if(format[i] == '*')
					{
						length = 1;
					}else{
						amx_StrLen(addr, &length);
						if(addr[0] & 0xFF000000)
						{
							length = 1 + ((length - 1) / sizeof(cell));
						}
						length += 1;
					}
					cell *newaddr;
					amx_AllotSafe(target_amx, length, &param, &newaddr);
					std::memcpy(newaddr, addr, length * sizeof(cell));
				}
				stack.push(param);
				break;
			}
			case 'S':
			{
				addr = amx_GetAddrSafe(amx, param);
				strings::cell_string *ptr;
				if(strings::pool.get_by_id(*addr, ptr))
				{
					size_t size = ptr->size();
					amx_AllotSafe(target_amx, size + 1, &param, &addr);
					std::memcpy(addr, ptr->c_str(), size * sizeof(cell));
					addr[size] = 0;
				}else if(ptr == nullptr)
				{
					amx_AllotSafe(target_amx, 1, &param, &addr);
					addr[0] = 0;
				}else{
					amx_LogicError(errors::pointer_invalid, "string", *addr);
				}
				stack.push(param);
				break;
			}
			case 'L':
			{
				addr = amx_GetAddrSafe(amx, param);
				list_t *ptr;
				if(list_pool.get_by_id(*addr, ptr))
				{
					for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
					{
						cell addr = it->store(target_amx);
						storage[&*it] = addr;
						stack.push(addr);
					}
					cell fmt_value;
					amx_AllotSafe(target_amx, ptr->size() + 1, &fmt_value, &addr);
					for(auto it = ptr->begin(); it != ptr->end(); it++)
					{
						*(addr++) = it->get_specifier();
					}
					*addr = 0;
					stack.push(fmt_value);
				}else{
					amx_LogicError(errors::pointer_invalid, "list", *addr);
				}
				break;
			}
			case 'l':
			{
				addr = amx_GetAddrSafe(amx, param);
				list_t *ptr;
				if(list_pool.get_by_id(*addr, ptr))
				{
					for(auto it = ptr->rbegin(); it != ptr->rend(); it++)
					{
						cell addr = it->store(target_amx);
						storage[&*it] = addr;
						stack.push(addr);
					}
				}else{
					amx_LogicError(errors::pointer_invalid, "list", *addr);
				}
				break;
			}
			case 'v':
			{
				addr = amx_GetAddrSafe(amx, param);
				dyn_object *ptr;
				if(variants::pool.get_by_id(*addr, ptr))
				{
					param = ptr->store(target_amx);
					storage[ptr] = param;
				}else if(ptr == nullptr)
				{
					param = 0;
				}else{
					amx_LogicError(errors::pointer_invalid, "variant", *addr);
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
				addr = amx_GetAddrSafe(amx, param);
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
		resultptr = amx_GetAddrSafe(amx, params[1]);
	}else{
		resultptr = &result;
	}

	int ret;
	if(native)
	{
		try{
			tag_ptr out_tag;
			*resultptr = amx::dynamic_call(target_amx, func, reinterpret_cast<cell*>(stack.data + target_amx->stk), out_tag);
			native_return_tag = out_tag;
			ret = AMX_ERR_NONE;
		}catch(errors::native_error &err)
		{
			if(try_)
			{
				if(msg)
				{
					*msg = std::move(err.message);
				}
				ret = AMX_ERR_NATIVE;
			}else{
				for(auto &pair : storage)
				{
					pair.first->load(target_amx, pair.second);
				}
				stack.reset();
				amx_LogicError(errors::inner_error_msg, "native", fname, err.message.c_str());
			}
		}catch(errors::amx_error &err)
		{
			ret = err.code;
		}
	}else{
		target_amx->error = AMX_ERR_NONE;
		ret = amx_Exec(target_amx, resultptr, pubindex);
		target_amx->error = AMX_ERR_NONE;
	}
	for(auto &pair : storage)
	{
		pair.first->load(target_amx, pair.second);
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
	// native bool:pawn_native_exists(const function[]);
	AMX_DEFINE_NATIVE_TAG(pawn_native_exists, 1, bool)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}

		return name && amx::find_native(amx, name);
	}

	// native bool:pawn_native_imported(const function[]);
	AMX_DEFINE_NATIVE_TAG(pawn_native_imported, 1, bool)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}

		int index;
		return name && amx_FindNative(amx, name, &index) == AMX_ERR_NONE;
	}

	// native bool:pawn_public_exists(const function[]);
	AMX_DEFINE_NATIVE_TAG(pawn_public_exists, 1, bool)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}

		int index;
		return name && amx_FindPublicSafe(amx, name, &index) == AMX_ERR_NONE;
	}

	// native pawn_call_native(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(pawn_call_native, 2)
	{
		return pawn_call(amx, params[0], params + 1, true, false, nullptr, amx);
	}

	// native pawn_call_public(const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_call_public, 2, cell)
	{
		return pawn_call(amx, params[0], params + 1, false, false, nullptr, amx);
	}

	// native amx_err:pawn_try_call_native(const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_try_call_native, 3, cell)
	{
		return pawn_call(amx, params[0], params + 1, true, true, nullptr, amx);
	}

	// native amx_err:pawn_try_call_native_msg(const function[], &result, msg[], msg_size=sizeof(msg), const format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_try_call_native_msg, 4, cell)
	{
		std::string msg;
		cell *msg_addr = amx_GetAddrSafe(amx, params[3]);
		cell result = pawn_call(amx, params[0], params + 1, true, true, &msg, amx);
		if(msg.size() == 0)
		{
			amx_SetString(msg_addr, amx::StrError(result), false, false, params[4]);
		}else{
			amx_SetString(msg_addr, msg.c_str(), false, false, params[4]);
		}
		return result;
	}

	// native amx_err:pawn_try_call_native_msg_s(const function[], &result, &StringTag:msg, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_try_call_native_msg_s, 3, cell)
	{
		std::string msg;
		cell *msg_addr = amx_GetAddrSafe(amx, params[3]);
		cell result = pawn_call(amx, params[0], params + 1, true, true, &msg, amx);
		if(msg.size() == 0)
		{
			*msg_addr = strings::create(amx::StrError(result));
		}else{
			*msg_addr = strings::create(msg);
		}
		return result;
	}

	// native amx_err:pawn_try_call_public(const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_try_call_public, 3, cell)
	{
		return pawn_call(amx, params[0], params + 1, false, true, nullptr, amx);
	}

	// native pawn_create_callback(const callback[], Expression:action);
	AMX_DEFINE_NATIVE_TAG(pawn_create_callback, 2, cell)
	{
		expression_ptr expr;
		if(params[2])
		{
			if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		}
		char *callback;
		amx_StrParam(amx, params[1], callback);
		if(callback == nullptr)
		{
			amx_FormalError(errors::arg_empty, "callback");
		}
		return events::new_callback(callback, amx, std::move(expr));
	}

	// native CallbackHandler:pawn_register_callback(const callback[], const function[], handler_flags:flags=handler_default, const additional_format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_register_callback, 2, callback_handler)
	{
		char *callback;
		amx_StrParam(amx, params[1], callback);

		char *fname;
		amx_StrParam(amx, params[2], fname);

		cell flags = optparam(3, 0);

		const char *format;
		amx_OptStrParam(amx, 4, format, "");

		if(callback == nullptr)
		{
			amx_FormalError(errors::arg_empty, "callback");
		}
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}

		cell ret = events::register_callback(callback, flags, amx, fname, format, params + 5, (params[0] / static_cast<int>(sizeof(cell))) - 4);
		return ret;
	}

	// native pawn_unregister_callback(CallbackHandler:id);
	AMX_DEFINE_NATIVE_TAG(pawn_unregister_callback, 1, cell)
	{
		return static_cast<cell>(events::remove_callback(amx, params[1]));
	}

	// native NativeHook:pawn_add_hook(const function[], const format[], const handler[], const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_add_hook, 3, native_hook)
	{
		char *native;
		amx_StrParam(amx, params[1], native);

		char *native_format;
		amx_StrParam(amx, params[2], native_format);

		char *fname;
		amx_StrParam(amx, params[3], fname);

		const char *format;
		amx_OptStrParam(amx, 4, format, "");

		if(native == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "handler");
		}

		cell ret = amxhook::register_hook(amx, native, native_format, fname, format, params + 5, (params[0] / static_cast<int>(sizeof(cell))) - 4);
		return ret;
	}

	// native NativeHook:pawn_add_filter(const function[], const format[], const handler[], filter_type:type=filter_in, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(pawn_add_filter, 3, native_hook)
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

		if(native == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "handler");
		}

		cell ret = amxhook::register_filter(amx, output, native, native_format, fname, format, params + 6, (params[0] / static_cast<int>(sizeof(cell))) - 5);
		return ret;
	}

	// native pawn_remove_hook(NativeHook:id);
	AMX_DEFINE_NATIVE_TAG(pawn_remove_hook, 1, cell)
	{
		return static_cast<cell>(amxhook::remove_hook(params[1]));
	}

	// native Guard:pawn_guard(AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pawn_guard, 2, guard)
	{
		auto obj = dyn_object(amx, params[1], params[2]);
		return guards::get_id(amx, guards::add(amx, std::move(obj)));
	}

	// native Guard:pawn_guard_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pawn_guard_arr, 3, guard)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		auto obj = dyn_object(amx, addr, params[2], params[3]);
		return guards::get_id(amx, guards::add(amx, std::move(obj)));
	}

	// native bool:pawn_guard_valid(Guard:guard);
	AMX_DEFINE_NATIVE_TAG(pawn_guard_valid, 1, bool)
	{
		handle_t *obj;
		return guards::get_by_id(amx, params[1], obj);
	}

	// native pawn_guard_free(Guard:guard);
	AMX_DEFINE_NATIVE_TAG(pawn_guard_free, 1, cell)
	{
		handle_t *obj;
		if(!guards::get_by_id(amx, params[1], obj)) amx_LogicError(errors::pointer_invalid, "guard", params[1]);
		
		return guards::free(amx, obj);
	}
	
	// native List:pawn_get_args(const format[], bool:byref=false, level=0);
	AMX_DEFINE_NATIVE_TAG(pawn_get_args, 1, list)
	{
		char *format;
		amx_StrParam(amx, params[1], format);
		size_t numargs = format == nullptr ? 0 : std::strlen(format);
		if(numargs > 0 && format[numargs - 1] == '+')
		{
			numargs--;
		}

		bool byref = optparam(2, 0);

		auto data = amx_GetData(amx);

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

	// native ArgTag:[2]pawn_arg_pack(AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pawn_arg_pack, 2, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		addr[0] = params[1];
		addr[1] = params[2];
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pawn_native_exists),
	AMX_DECLARE_NATIVE(pawn_native_imported),
	AMX_DECLARE_NATIVE(pawn_public_exists),
	AMX_DECLARE_NATIVE(pawn_call_native),
	AMX_DECLARE_NATIVE(pawn_call_public),
	AMX_DECLARE_NATIVE(pawn_try_call_native),
	AMX_DECLARE_NATIVE(pawn_try_call_native_msg),
	AMX_DECLARE_NATIVE(pawn_try_call_native_msg_s),
	AMX_DECLARE_NATIVE(pawn_try_call_public),
	AMX_DECLARE_NATIVE(pawn_create_callback),
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
	AMX_DECLARE_NATIVE(pawn_arg_pack),
};

int RegisterPawnNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
