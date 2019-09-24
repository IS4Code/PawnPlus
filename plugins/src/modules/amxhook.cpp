#include "amxhook.h"
#include "amxinfo.h"
#include "main.h"
#include "utils/func_pool.h"
#include "objects/stored_param.h"
#include "utils/linear_pool.h"
#include "subhook/subhook.h"
#include "utils/optional.h"
#include "errors.h"

#include <array>
#include <unordered_map>
#include <algorithm>
#include <exception>
#include <memory>
#include <string>
#include <cstring>
#include <tuple>
#include <limits>

class hook_handler
{
private:
	aux::optional<int> index;

protected:
	amx::handle amx;
	std::string handler;
	std::string format;
	std::vector<stored_param> arg_values;

	bool handler_index(AMX *&amx, int &index);

public:
	hook_handler() = default;
	hook_handler(AMX *amx, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs);
	virtual bool invoke(class hooked_func &parent, AMX *amx, cell *params, cell &result);
	bool valid() const;
};

class filter_handler : public hook_handler
{
	bool output;

public:
	filter_handler(AMX *amx, bool output, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs);
	virtual bool invoke(class hooked_func &parent, AMX *amx, cell *params, cell &result) override;
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

	cell add_handler(std::unique_ptr<hook_handler> &&handler);
	bool remove_handler(cell id);
	cell invoke(AMX *amx, cell *params);
	bool empty() const { return handlers.empty(); }
	bool running() const { return handler_level != -1; }
	std::string get_name() const { return name; }
	AMX_NATIVE get_native() const { return native; }
	size_t get_index() const { return index; }
};

constexpr const size_t max_hooked_natives = 1024;
std::array<std::unique_ptr<hooked_func>, max_hooked_natives> native_hooks;
std::unordered_map<AMX_NATIVE, size_t> hooks_map;
std::unordered_map<cell, size_t> hook_handlers;

cell native_hook_handler(size_t index, AMX *amx, cell *params)
{
	auto &hook = native_hooks[index];
	if(hook)
	{
		return hook->invoke(amx, params);
	}
	throw std::logic_error("[PawnPlus] Hook was not properly unregistered.");
}

using func_pool = aux::func<cell(AMX*, cell*)>::pool<max_hooked_natives, native_hook_handler>;

size_t amxhook::hook_pool_size()
{
	return max_hooked_natives;
}

size_t amxhook::hook_count()
{
	size_t count = 0;
	for(size_t i = 0; i < max_hooked_natives; i++)
	{
		if(native_hooks[i])
		{
			count++;
		}
	}
	return count;
}

cell register_handler(AMX *amx, const char *native, std::unique_ptr<hook_handler> &&handler)
{
	std::string name(native);
	AMX_NATIVE func = amx::find_native(amx, name);

	if(!func) amx_FormalError(errors::func_not_found, "native", name.c_str());

	auto it = hooks_map.find(func);
	if(it == hooks_map.end())
	{
		auto p = func_pool::add();
		size_t index = p.first;
		if(index == -1)
		{
			amx_LogicError("Hook pool full. Adjust max_hooked_natives (currently %d) and recompile.", max_hooked_natives);
		}
		native_hooks[index] = std::make_unique<hooked_func>(name, func, p.second, index);
		it = hooks_map.emplace(func, index).first;
	}

	hooked_func &hook = *native_hooks[it->second];
	size_t index = hook.get_index();
	try{
		cell id = hook.add_handler(std::move(handler));
		hook_handlers[id] = index;
		return id;
	}catch(...)
	{
		if(hook.empty() && !hook.running())
		{
			hooks_map.erase(it);
			native_hooks[index] = nullptr;
			func_pool::remove(index);
		}
		throw;
	}
}

cell amxhook::register_hook(AMX *amx, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	return register_handler(amx, native, std::make_unique<hook_handler>(amx, func_format, handler, format, params, numargs));
}

cell amxhook::register_filter(AMX *amx, bool output, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	return register_handler(amx, native, std::make_unique<filter_handler>(amx, output, func_format, handler, format, params, numargs));
}

bool amxhook::remove_hook(cell id)
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

cell hooked_func::add_handler(std::unique_ptr<hook_handler> &&handler)
{
	cell id = reinterpret_cast<cell>(handler.get());
	handlers.push_back(std::move(handler));
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
	try{
		cell result = std::numeric_limits<cell>::min();

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
			auto it = handlers.begin();
			while(it != handlers.end())
			{
				if(!(*it)->valid())
				{
					it = handlers.erase(it);
				}else{
					++it;
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
	}catch(const errors::amx_error &err)
	{
		amx_RaiseError(amx, err.code);
		return 0;
	}
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

filter_handler::filter_handler(AMX *amx, bool output, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs) : hook_handler(amx, func_format, function, format, args, numargs), output(output)
{

}

bool hook_handler::valid() const
{
	if(auto amx_obj = this->amx.lock()) return amx_obj->valid();
	return false;
}

bool hook_handler::handler_index(AMX *&amx, int &index)
{
	auto amx_obj = this->amx.lock();
	if(!amx_obj || !amx_obj->valid()) return false;
	amx = *amx_obj;

	if(this->index.has_value())
	{
		index = this->index.value();

		char *funcname = amx_NameBuffer(amx);
		
		if(amx_GetPublic(amx, index, funcname) == AMX_ERR_NONE && !std::strcmp(handler.c_str(), funcname))
		{
			return true;
		}else if(amx_FindPublicSafe(amx, handler.c_str(), &index) == AMX_ERR_NONE)
		{
			this->index = index;
			return true;
		}
	}else if(amx_FindPublicSafe(amx, handler.c_str(), &index) == AMX_ERR_NONE)
	{
		this->index = index;
		return true;
	}
	logwarn(amx, "[PawnPlus] Hook handler %s was not found.", handler.c_str());
	return false;
}

extern AMX *source_amx;

bool hook_handler::invoke(hooked_func &parent, AMX *amx, cell *params, cell &result)
{
	AMX *my_amx;
	int index;
	if(!handler_index(my_amx, index)) return false;

	int numargs = format.size();
	if(format[numargs - 1] == '+' || format[numargs - 1] == '-')
	{
		numargs--;
	}

	if(params[0] < numargs * static_cast<int>(sizeof(cell)))
	{
		int argi = 1 + params[0] / sizeof(cell);
		logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, format[argi]);
		return false;
	}

	amx::guard guard(my_amx);

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
				if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
				{
					int length;
					amx_StrLen(src_addr, &length);
					if(src_addr[0] & 0xFF000000)
					{
						length = 1 + ((length - 1) / sizeof(cell));
					}

					amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));
					target_addr[length] = 0;
					storage.push_back(std::make_tuple(target_addr, src_addr, length + 1));

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
			}else{
				amx_Push(my_amx, param);
			}
		}
	}else if(format[numargs] == '-')
	{
		for(int argi = (params[0] / sizeof(cell)) - 1; argi >= numargs; argi--)
		{
			cell &param = params[1 + argi];
			amx_Push(my_amx, param);
		}
	}

	for(int argi = numargs - 1; argi >= 0; argi--)
	{
		char c = format[argi];
		cell &param = params[1 + argi];
		switch(c)
		{
			case '+':
			case '-':
			{
				logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') must be at the end of the format string.", handler.c_str(), parent.get_name().c_str(), argi, c);
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						amx_AllotSafe(my_amx, 1, &amx_addr, &target_addr);
						*target_addr = *src_addr;
						storage.push_back(std::make_tuple(target_addr, src_addr, 1));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'A': //in-out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						storage.push_back(std::make_tuple(target_addr, src_addr, length));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'o': //out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						storage.push_back(std::make_tuple(target_addr, src_addr, length));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						int length;
						amx_StrLen(src_addr, &length);
						if(src_addr[0] & 0xFF000000)
						{
							length = 1 + ((length - 1) / sizeof(cell));
						}

						amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						target_addr[length] = 0;

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						int length;
						amx_StrLen(src_addr, &length);
						if(src_addr[0] & 0xFF000000)
						{
							length = 1 + ((length - 1) / sizeof(cell));
						}

						amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						target_addr[length] = 0;
						storage.push_back(std::make_tuple(target_addr, src_addr, length + 1));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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

	auto old_source_amx = source_amx;
	source_amx = amx;
	amx_Exec(my_amx, &result, index);
	source_amx = old_source_amx;

	for(auto mem : storage)
	{
		std::memcpy(std::get<1>(mem), std::get<0>(mem), std::get<2>(mem) * sizeof(cell));
	}

	return true;
}

bool filter_handler::invoke(hooked_func &parent, AMX *amx, cell *params, cell &result)
{
	AMX *my_amx;
	int index;
	if(!handler_index(my_amx, index)) return false;

	int numargs = format.size();
	if(format[numargs - 1] == '+' || format[numargs - 1] == '-')
	{
		numargs--;
	}

	if(params[0] < numargs * static_cast<int>(sizeof(cell)))
	{
		int argi = 1 + params[0] / sizeof(cell);
		logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, format[argi]);
		return false;
	}

	if(output)
	{
		result = parent.invoke(amx, params);
	}

	amx::guard guard(my_amx);

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
				if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
				{
					int length;
					amx_StrLen(src_addr, &length);
					if(src_addr[0] & 0xFF000000)
					{
						length = 1 + ((length - 1) / sizeof(cell));
					}

					amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
					std::memcpy(target_addr, src_addr, length * sizeof(cell));
					target_addr[length] = 0;

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
			}else{
				amx_Push(my_amx, param);
			}
		}
	}else if(format[numargs] == '-')
	{
		for(int argi = (params[0] / sizeof(cell)) - 1; argi >= numargs; argi--)
		{
			cell &param = params[1 + argi];
			cell amx_addr, *phys_addr;
			amx_AllotSafe(my_amx, 1, &amx_addr, &phys_addr);

			*phys_addr = param;
			storage.push_back(std::make_tuple(phys_addr, &param, 1));

			amx_Push(my_amx, amx_addr);
		}
	}

	for(int argi = numargs - 1; argi >= 0; argi--)
	{
		char c = format[argi];
		cell &param = params[1 + argi];
		switch(c)
		{
			case '+':
			case '-':
			{
				logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') must be at the end of the format string.", handler.c_str(), parent.get_name().c_str(), argi, c);
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						amx_AllotSafe(my_amx, 1, &amx_addr, &target_addr);
						*target_addr = *src_addr;
						storage.push_back(std::make_tuple(target_addr, src_addr, 1));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'A': //in-out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						storage.push_back(std::make_tuple(target_addr, src_addr, length));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			case 'o': //out array
			{
				if(amx != my_amx)
				{
					cell amx_addr, *src_addr, *target_addr;
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						if(argi + 1 >= numargs)
						{
							logwarn(amx, "[PawnPlus] Hook handler %s was not able to handle a call to %s, because the length of array #%d was not passed to the native function.", handler.c_str(), parent.get_name().c_str(), argi, c);
							return false;
						}
						int length = params[2 + argi];
						amx_AllotSafe(my_amx, length, &amx_addr, &target_addr);
						storage.push_back(std::make_tuple(target_addr, src_addr, length));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						int length;
						amx_StrLen(src_addr, &length);
						if(src_addr[0] & 0xFF000000)
						{
							length = 1 + ((length - 1) / sizeof(cell));
						}

						amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						target_addr[length] = 0;

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
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
					if(amx_GetAddr(amx, param, &src_addr) == AMX_ERR_NONE)
					{
						int length;
						amx_StrLen(src_addr, &length);
						if(src_addr[0] & 0xFF000000)
						{
							length = 1 + ((length - 1) / sizeof(cell));
						}

						amx_AllotSafe(my_amx, length + 1, &amx_addr, &target_addr);
						std::memcpy(target_addr, src_addr, length * sizeof(cell));
						target_addr[length] = 0;
						storage.push_back(std::make_tuple(target_addr, src_addr, length + 1));

						amx_Push(my_amx, amx_addr);
					}else{
						amx_Push(my_amx, param);
					}
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			default: //cell
			{
				if(amx == my_amx)
				{
					auto data = amx_GetData(amx);
					if(reinterpret_cast<unsigned char*>(&param) >= data && reinterpret_cast<unsigned char*>(&param) < data + amx->stp)
					{
						amx_Push(my_amx, reinterpret_cast<unsigned char*>(&param) - data);
						break;
					}
				}
				cell amx_addr, *phys_addr;
				amx_AllotSafe(my_amx, 1, &amx_addr, &phys_addr);

				*phys_addr = param;
				storage.push_back(std::make_tuple(phys_addr, &param, 1));

				amx_Push(my_amx, amx_addr);
				break;
			}
		}
	}

	cell amx_addr, *phys_addr;
	amx_AllotSafe(my_amx, 1, &amx_addr, &phys_addr);
	*phys_addr = result;
	amx_Push(my_amx, amx_addr);
	storage.push_back(std::make_tuple(phys_addr, &result, 1));

	for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
	{
		it->push(my_amx, reinterpret_cast<cell>(this));
	}

	cell retval;
	auto old_source_amx = source_amx;
	source_amx = amx;
	amx_Exec(my_amx, &retval, index);
	source_amx = old_source_amx;

	for(auto mem : storage)
	{
		std::memcpy(std::get<1>(mem), std::get<0>(mem), std::get<2>(mem) * sizeof(cell));
	}

	if(output)
	{
		return true;
	}else{
		return !!retval;
	}
}
