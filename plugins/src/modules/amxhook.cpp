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
#include <string>
#include <cstring>
#include <tuple>

class hook_handler
{
	size_t index;
	AMX *amx;
	std::string handler;
	std::string format;
	std::vector<stored_param> arg_values;

public:
	hook_handler() = default;
	hook_handler(size_t index, AMX *amx, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs);
	bool invoke(const class hooked_func &parent, AMX *amx, cell *params, cell &result) const;
	size_t get_index() { return index; }
};

class hooked_func
{
	std::string native;
	size_t index;
	subhook_t hook = nullptr;
	std::vector<hook_handler> pre_handlers;
	std::vector<hook_handler> post_handlers;

	void free_hook();
public:
	hooked_func() = default;
	hooked_func(AMX *amx, const std::string &native);
	hooked_func(hooked_func &&obj);
	hooked_func(const hooked_func &) = delete;
	~hooked_func();
	hooked_func &operator=(hooked_func &&obj);
	hooked_func &operator=(hooked_func &) = delete;

	size_t get_index() const { return index; }

	void add_handler(size_t index, bool post, AMX *amx, const char *func_format, const char *handler, const char *format, const cell *params, int numargs);
	bool remove_handler(size_t index);
	cell invoke(AMX *amx, cell *params) const;
	bool empty() const { return pre_handlers.empty() && post_handlers.empty(); }
	const std::string &name() const { return native; }
};

constexpr const size_t max_hooked_funcs = 256;
std::array<hooked_func, max_hooked_funcs> native_hooks;
std::unordered_map<std::string, size_t> hooks_map;
aux::linear_pool<size_t> hooks;

cell native_hook_handler(size_t index, AMX *amx, cell *params)
{
	auto &hook = native_hooks[index];
	return hook.invoke(amx, params);
}

using func_pool = aux::func<cell(AMX*, cell*)>::pool<max_hooked_funcs, native_hook_handler>;

int amxhook::register_hook(AMX *amx, bool post, const char *native, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	std::string str(native);

	auto it = hooks_map.find(str);
	if(it == hooks_map.end())
	{
		hooked_func hook(amx, str);
		size_t index = hook.get_index();
		if(index == -1)
		{
			logerror(amx, "[PP] Hook pool full. Adjust max_hooked_funcs (currently %d) and recompile.", max_hooked_funcs);
			return -1;
		}
		native_hooks[index] = std::move(hook);
		it = hooks_map.emplace(std::move(str), index).first;
	}

	hooked_func &hook = native_hooks[it->second];
	size_t index = hooks.add(hook.get_index());
	try{
		hook.add_handler(index, post, amx, func_format, handler, format, params, numargs);
	}catch(nullptr_t)
	{
		hooks.remove(index);
		if(hook.empty())
		{
			hooks_map.erase(it);
			native_hooks[hook.get_index()] = {};
		}
		return -1;
	}
	return index;
}

bool amxhook::remove_hook(AMX *amx, int index)
{
	size_t *ptr = hooks.get(index);
	if(ptr == nullptr) return false;
	auto &hook = native_hooks[*ptr];
	hooks.remove(index);
	if(!hook.remove_handler(index)) return false;
	if(hook.empty())
	{
		hooks_map.erase(hook.name());
		native_hooks[hook.get_index()] = {};
	}
	return true;
}

hooked_func::hooked_func(AMX *amx, const std::string &native) : native(native)
{
	auto p = func_pool::add();
	index = p.first;
	if(index == -1) return;
	auto f = amx::find_native(amx, native);
	hook = subhook_new(reinterpret_cast<void*>(f), reinterpret_cast<void*>(p.second), {});
	subhook_install(hook);
}

hooked_func::hooked_func(hooked_func &&obj) : pre_handlers(std::move(obj.pre_handlers)), post_handlers(std::move(obj.post_handlers)), native(std::move(obj.native)), index(obj.index), hook(obj.hook)
{
	obj.hook = nullptr;
}

hooked_func::~hooked_func()
{
	free_hook();
}

void hooked_func::free_hook()
{
	if(hook != nullptr)
	{
		subhook_remove(hook);
		subhook_free(hook);
		hook = nullptr;
		func_pool::remove(index);
	}
}

hooked_func &hooked_func::operator=(hooked_func &&obj)
{
	if(this == &obj) return *this;
	free_hook();
	pre_handlers = std::move(obj.pre_handlers);
	post_handlers = std::move(obj.post_handlers);
	native = std::move(obj.native);
	index = obj.index;
	hook = obj.hook;
	obj.hook = nullptr;
	return *this;
}

void hooked_func::add_handler(size_t index, bool post, AMX *amx, const char *func_format, const char *handler, const char *format, const cell *params, int numargs)
{
	auto &vect = post ? post_handlers : pre_handlers;
	vect.emplace_back(index, amx, func_format, handler, format, params, numargs);
}

bool hooked_func::remove_handler(size_t index)
{
	auto it = std::find_if(pre_handlers.begin(), pre_handlers.end(), [=](hook_handler &hndl) {return hndl.get_index() == index; });
	if(it != pre_handlers.end())
	{
		pre_handlers.erase(it);
		return true;
	}
	it = std::find_if(post_handlers.begin(), post_handlers.end(), [=](hook_handler &hndl) {return hndl.get_index() == index; });
	if(it != post_handlers.end())
	{
		post_handlers.erase(it);
		return true;
	}
	return false;
}

cell hooked_func::invoke(AMX *amx, cell *params) const
{
	cell result = 0;

	// we might get deleted at any time, be cautious
	auto func = reinterpret_cast<AMX_NATIVE>(subhook_get_src(hook));
	for(auto it = pre_handlers.rbegin(); it != pre_handlers.rend(); it++)
	{
		if(!it->invoke(*this, amx, params, result)) return result;
		if(pre_handlers.size() == 0) break;
	}
	if(hook == nullptr)
	{
		result = func(amx, params);
	}else{
		result = reinterpret_cast<AMX_NATIVE>(subhook_get_trampoline(hook))(amx, params);
	}
	for(auto it = post_handlers.begin(); it != post_handlers.end(); it++)
	{
		if(!it->invoke(*this, amx, params, result)) return result;
		if(post_handlers.size() == 0) break;
	}

	return result;
}

hook_handler::hook_handler(size_t index, AMX *amx, const char *func_format, const char *function, const char *format, const cell *args, size_t numargs) : index(index), amx(amx), handler(function)
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

bool hook_handler::invoke(const hooked_func &parent, AMX *amx, cell *params, cell &result) const
{
	auto my_amx = this->amx;

	int index;
	if(amx_FindPublic(my_amx, handler.c_str(), &index) != AMX_ERR_NONE)
	{
		logwarn(amx, "[PP] Hook handler %s was not found.", handler.c_str());
		return true;
	}

	cell numargs = params[0] / sizeof(cell);

	cell reset_hea, *tmp;
	amx_Allot(my_amx, 0, &reset_hea, &tmp);

	std::vector<std::tuple<cell*, cell*, size_t>> storage;

	for(int argi = format.size() - 1; argi >= 0; argi--)
	{
		char c = format[argi];
		if(argi >= numargs)
		{
			logwarn(amx, "[PP] Hook handler %s was not able to handle a call to %s, because the parameter #%d ('%c') was not passed to the native function.", handler.c_str(), parent.name().c_str(), argi, c);
			return true;
		}
		cell &param = params[1 + argi];
		switch(c)
		{
			case '_':
			{
				break;
			}
			case '*':
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
			case 's':
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
					storage.push_back(std::make_tuple(target_addr, src_addr, length));

					amx_Push(my_amx, amx_addr);
				}else{
					amx_Push(my_amx, param);
				}
				break;
			}
			default:
			{
				cell amx_addr, *phys_addr;
				amx_Allot(my_amx, 1, &amx_addr, &phys_addr);

				*phys_addr = param;
				storage.push_back(std::make_tuple(phys_addr, &param, 1));

				amx_Push(my_amx, amx_addr);
				break;
			}
		}
	}

	cell amx_addr, *phys_addr;
	amx_Allot(my_amx, 1, &amx_addr, &phys_addr);
	*phys_addr = result;
	amx_Push(my_amx, amx_addr);
	storage.push_back(std::make_tuple(phys_addr, &result, 1));

	for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
	{
		it->push(my_amx, this->index);
	}

	amx_Push(my_amx, reinterpret_cast<cell>(amx));
	
	cell retval;
	amx_Exec(my_amx, &retval, index);

	for(auto mem : storage)
	{
		std::memcpy(std::get<1>(mem), std::get<0>(mem), std::get<2>(mem) * sizeof(cell));
	}

	amx_Release(my_amx, reset_hea);

	return !!retval;
}
