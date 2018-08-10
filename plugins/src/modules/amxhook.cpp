#include "amxhook.h"
#include "amxinfo.h"
#include "main.h"
#include "utils/func_pool.h"
#include "objects/stored_param.h"
#include "utils/linear_pool.h"
#include "subhook/subhook.h"
#include <array>
#include <unordered_map>
#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <cstring>
#include <tuple>

class hook_handler
{
	amx::handle amx;
	std::string handler;
	std::string format;
	std::vector<stored_param> arg_values;

public:
	hook_handler() = default;
	hook_handler(AMX *amx, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs);
	bool invoke(const class hooked_func &parent, AMX *amx, cell *params, cell &result) const;
	bool valid() const;
};

class hooked_func
{
	std::string name;
	AMX_NATIVE native;
	size_t index;
	subhook_t hook = nullptr;
	std::vector<std::unique_ptr<hook_handler>> handlers;
	size_t handler_level = -1;
	std::vector<cell> removed_handlers;

public:
	hooked_func(std::string name, AMX_NATIVE native, AMX_NATIVE hook, size_t index);
	hooked_func(const hooked_func &) = delete;
	hooked_func &operator=(hooked_func &) = delete;
	~hooked_func();

	cell add_handler(AMX *amx, const char *func_format, const char *handler, const char *format, const cell *params, int numargs);
	bool remove_handler(cell id);
	cell invoke(AMX *amx, cell *params);
	bool empty() const { return handlers.empty(); }
	bool running() const { return handler_level != -1; }
	std::string get_name() const { return name; }
	AMX_NATIVE get_native() const { return native; }
	size_t get_index() const { return index; }
};

constexpr const size_t max_hooked_funcs = 256;
std::array<std::unique_ptr<hooked_func>, max_hooked_funcs> native_hooks;
std::unordered_map<AMX_NATIVE, size_t> hooks_map;
std::unordered_map<cell, size_t> hook_handlers;

cell native_hook_handler(size_t index, AMX *amx, cell *params)
{
	auto &hook = native_hooks[index];
	if(hook)
	{
		return hook->invoke(amx, params);
	}
	throw std::logic_error("[PP] Hook was not properly unregistered.");
}

using func_pool = aux::func<cell(AMX*, cell*)>::pool<max_hooked_funcs, native_hook_handler>;

cell amxhook::register_hook(AMX *amx, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	std::string name(native);
	AMX_NATIVE func = amx::find_native(amx, name);

	auto it = hooks_map.find(func);
	if(it == hooks_map.end())
	{
		auto p = func_pool::add();
		size_t index = p.first;
		if(index == -1)
		{
			logerror(amx, "[PP] Hook pool full. Adjust max_hooked_funcs (currently %d) and recompile.", max_hooked_funcs);
			return -1;
		}
		native_hooks[index] = std::make_unique<hooked_func>(name, func, p.second, index);
		it = hooks_map.emplace(func, index).first;
	}

	hooked_func &hook = *native_hooks[it->second];
	size_t index = hook.get_index();
	try{
		cell id = hook.add_handler(amx, func_format, handler, format, params, numargs);
		hook_handlers[id] = index;
		return id;
	}catch(nullptr_t)
	{
		if(hook.empty() && !hook.running())
		{
			hooks_map.erase(it);
			native_hooks[index] = nullptr;
			func_pool::remove(index);
		}
		return -1;
	}
}

bool amxhook::remove_hook(AMX *amx, cell id)
{
	auto it = hook_handlers.find(id);
	if(it == hook_handlers.end()) return false;

	size_t index = it->second;
	hook_handlers.erase(it);
	hooked_func &hook = *native_hooks[index];
	if(!hook.remove_handler(id)) return false;

	if(hook.empty() && !hook.running())
	{
		hooks_map.erase(hook.get_native());
		size_t index = hook.get_index();
		native_hooks[index] = nullptr;
		func_pool::remove(index);
	}
	return true;
}

hooked_func::hooked_func(std::string name, AMX_NATIVE native, AMX_NATIVE hook, size_t index) : name(name), native(native), index(index)
{
	this->hook = subhook_new(reinterpret_cast<void*>(native), reinterpret_cast<void*>(hook), {});
	subhook_install(this->hook);
}

hooked_func::~hooked_func()
{
	if(hook != nullptr)
	{
		subhook_remove(hook);
		subhook_free(hook);
		hook = nullptr;
	}
}

cell hooked_func::add_handler(AMX *amx, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	auto ptr = std::make_unique<hook_handler>(amx, func_format, handler, format, params, numargs);
	cell id = reinterpret_cast<cell>(ptr.get());
	handlers.push_back(std::move(ptr));
	return id;
}

bool hooked_func::remove_handler(cell id)
{
	auto test = reinterpret_cast<hook_handler*>(id);

	auto it = std::find_if(handlers.begin(), handlers.end(), [=](std::unique_ptr<hook_handler> &ptr) {return ptr.get() == test; });
	if(it != handlers.end())
	{
		if(handler_level != -1)
		{
			**it = {};
		}else{
			handlers.erase(it);
		}
		return true;
	}
	return false;
}

cell hooked_func::invoke(AMX *amx, cell *params)
{
	cell result;

	size_t old = handler_level;
	if(handler_level == -1)
	{
		handler_level = handlers.size();
	}

	bool ok = false;
	while(!ok)
	{
		if(handler_level == 0)
		{
			result = reinterpret_cast<AMX_NATIVE>(subhook_get_trampoline(hook))(amx, params);
			ok = true;
		}else{
			handler_level--;
			ok = handlers[handler_level]->invoke(*this, amx, params, result);
		}
	}

	handler_level = old;
	if(old == -1)
	{
		for(auto it = handlers.begin(); it != handlers.end(); ++it)
		{
			if(!(*it)->valid())
			{
				it = handlers.erase(it);
			}
		}
		if(empty())
		{
			hooks_map.erase(native);
			size_t index = this->index;
			native_hooks[index] = nullptr;
			func_pool::remove(index);
		}
	}
	return result;
}

hook_handler::hook_handler(AMX *amx, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs) : amx(amx::load(amx)), handler(function)
{
	if(func_format != nullptr)
	{
		this->format = func_format;
	}
	if(format != nullptr)
	{
		size_t argi = -1;
		size_t len = strlen(format);
		for(size_t i = 0; i < len; i++)
		{
			arg_values.push_back(stored_param::create(amx, format[i], args, argi, numargs));
		}
	}
}

bool hook_handler::valid() const
{
	if(auto amx_obj = this->amx.lock()) return amx_obj->valid();
	return false;
}

bool hook_handler::invoke(const hooked_func &parent, AMX *amx, cell *params, cell &result) const
{
	auto amx_obj = this->amx.lock();
	if(!amx_obj || !amx_obj->valid()) return false;
	auto &my_amx = *amx_obj;

	int index;
	if(amx_FindPublic(my_amx, handler.c_str(), &index) != AMX_ERR_NONE)
	{
		logwarn(amx, "[PP] Hook handler %s was not found.", handler.c_str());
		return false;
	}

	int numargs = format.size();
	if(format[numargs - 1] == '+')
	{
		numargs--;
	}

	if(params[0] < numargs * static_cast<int>(sizeof(cell)))
	{
		int argi = 1 + params[0] / sizeof(cell);
		logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, format[argi]);
		return false;
	}

	cell reset_hea, *tmp;
	amx_Allot(my_amx, 0, &reset_hea, &tmp);

	std::vector<std::tuple<cell*, cell*, size_t>> storage;

	if(format[numargs] == '+')
	{
		for(int argi = (params[0] / sizeof(cell)) - 1; argi >= numargs; argi--)
		{
			cell &param = params[1 + argi];
			if(amx != my_amx)
			{
				// assume a string (can copy too much memory, but not less than required)
				cell amx_addr, *src_addr, *target_addr;
				amx_GetAddr(amx, param, &src_addr);
				int length;
				amx_StrLen(src_addr, &length);
				if(src_addr[0] & 0xFF000000)
				{
					length = 1 + ((length - 1) / sizeof(cell));
				}

				amx_Allot(my_amx, length + 1, &amx_addr, &target_addr);
				std::memcpy(target_addr, src_addr, length * sizeof(cell));
				target_addr[length] = 0;

				amx_Push(my_amx, amx_addr);
			}else{
				amx_Push(my_amx, param);
			}
		}
	}

	for(int argi = numargs - 1; argi >= 0; argi--)
	{
		char c = format[argi];
		cell &param = params[1 + argi];
		switch(c)
		{
			case '+':
			{
				logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') must be at the end of the format string.", handler.c_str(), parent.get_name().c_str(), argi, c);
				return false;
			}
			case '_': //ignore
			{
				break;
			}
			case '*': //by-ref cell
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);

					amx_Allot(my_amx, 1, &amx_addr, &target_addr);
					*target_addr = *src_addr;
					storage.push_back(std::make_tuple(target_addr, src_addr, 1));

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'a': //in array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);

					if(argi + 1 >= numargs)
					{
						logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
						return false;
					}
					int length = params[2 + argi];
					amx_Allot(my_amx, length, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));

					amx_Push(my_amx, amx_addr);
				} else {
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'A': //in-out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);

					if(argi + 1 >= numargs)
					{
						logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
						return false;
					}
					int length = params[2 + argi];
					amx_Allot(my_amx, length, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));
					storage.push_back(std::make_tuple(target_addr, src_addr, length));

					amx_Push(my_amx, amx_addr);
				} else {
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'o': //out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);

					if(argi + 1 >= numargs)
					{
						logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
						return false;
					}
					int length = params[2 + argi];
					amx_Allot(my_amx, length, &amx_addr, &target_addr);
					storage.push_back(std::make_tuple(target_addr, src_addr, length));

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 's': //in string
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);
					int length;
					amx_StrLen(src_addr, &length);
					if(src_addr[0] & 0xFF000000)
					{
						length = 1 + ((length - 1) / sizeof(cell));
					}

					amx_Allot(my_amx, length + 1, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));
					target_addr[length] = 0;

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'S': //in-out string
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					amx_GetAddr(amx, param, &src_addr);
					int length;
					amx_StrLen(src_addr, &length);
					if(src_addr[0] & 0xFF000000)
					{
						length = 1 + ((length - 1) / sizeof(cell));
					}

					amx_Allot(my_amx, length + 1, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));
					target_addr[length] = 0;
					storage.push_back(std::make_tuple(target_addr, src_addr, length + 1));

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			default: //cell
			{
				amx_Push(my_amx, param);
				break;
			}
		}
	}

	for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
	{
		it->push(my_amx, reinterpret_cast<cell>(this));
	}

	//amx_Push(my_amx, reinterpret_cast<cell>(amx));
	
	amx_Exec(my_amx, &result, index);

	for(auto mem : storage)
	{
		std::memcpy(std::get<1>(mem), std::get<0>(mem), std::get<2>(mem) * sizeof(cell));
	}

	amx_Release(my_amx, reset_hea);

	return true;
}
