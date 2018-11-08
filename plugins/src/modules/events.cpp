#include "events.h"
#include "amxinfo.h"
#include "hooks.h"
#include "main.h"
#include "objects/stored_param.h"
#include "utils/optional.h"

#include <cstring>
#include <memory>
#include <vector>
#include <unordered_map>

class event_info
{
	cell flags;
	std::vector<stored_param> arg_values;
	std::string handler;
	aux::optional<int> index;
	bool handler_index(AMX *amx, int &index);

public:
	event_info(cell flags, AMX *amx, const char *function, const char *format, const cell *args, size_t numargs);
	bool invoke(AMX *amx, cell *retval, cell id);
};


class amx_info : public amx::extra
{
public:
	std::vector<std::vector<std::unique_ptr<event_info>>> callback_handlers;
	std::unordered_map<cell, size_t> handler_ids;

	amx_info(AMX *amx) : amx::extra(amx)
	{

	}
};

amx_info &get_info(AMX *amx, amx::object &obj)
{
	obj = amx::load_lock(amx);
	return obj->get_extra<amx_info>();
}

namespace events
{
	int register_callback(const char *callback, cell flags, AMX *amx, const char *function, const char *format, const cell *params, int numargs)
	{
		try{
			amx::object obj;
			int index;
			if(amx_FindPublic(amx, callback, &index) != AMX_ERR_NONE || index < 0)
			{
				return -2;
			}
			auto &info = get_info(amx, obj);
			auto &callback_handlers = info.callback_handlers;
			if((size_t)index >= callback_handlers.size())
			{
				callback_handlers.resize(index + 1);
			}
			auto &list = callback_handlers[index];
			auto ptr = std::unique_ptr<event_info>(new event_info(flags, amx, function, format, params, numargs));
			cell id = reinterpret_cast<cell>(ptr.get());
			list.push_back(std::move(ptr));
			info.handler_ids[id] = index;
			return id;
		}catch(std::nullptr_t)
		{
			return -1;
		}
	}

	bool remove_callback(AMX *amx, cell id)
	{
		amx::object obj;
		auto &info = get_info(amx, obj);
		auto &handler_ids = info.handler_ids;
		auto ptr = reinterpret_cast<event_info*>(id);
		auto hit = handler_ids.find(id);
		if(hit != handler_ids.end())
		{
			auto &list = info.callback_handlers[hit->second];
			for(auto it = list.begin(); it != list.end(); it++)
			{
				if(it->get() == ptr)
				{
					list.erase(it);
					handler_ids.erase(hit);
					return true;
				}
			}
		}
		return false;
	}

	bool invoke_callbacks(AMX *amx, int index, cell *retval)
	{
		amx::object obj;
		auto &callback_handlers = get_info(amx, obj).callback_handlers;
		if(index < 0 || (size_t)index >= callback_handlers.size()) return false;
		for(auto &handler : callback_handlers[index])
		{
			if(handler)
			{
				if(handler->invoke(amx, retval, reinterpret_cast<cell>(handler.get())))
				{
					return true;
				}
			}
		}

		return false;
	}
}

event_info::event_info(cell flags, AMX *amx, const char *function, const char *format, const cell *args, size_t numargs)
	: flags(flags), handler(function)
{
	if(format != nullptr)
	{
		size_t argi = -1;
		size_t len = std::strlen(format);
		for(size_t i = 0; i < len; i++)
		{
			arg_values.push_back(stored_param::create(amx, format[i], args, argi, numargs));
		}
	}
}

bool event_info::handler_index(AMX *amx, int &index)
{
	if(this->index.has_value())
	{
		index = this->index.value();

		int len;
		amx_NameLength(amx, &len);
		char *funcname = static_cast<char*>(alloca(len + 1));

		if(amx_GetPublic(amx, index, funcname) == AMX_ERR_NONE && !std::strcmp(handler.c_str(), funcname))
		{
			return true;
		}else if(amx_FindPublic(amx, handler.c_str(), &index) == AMX_ERR_NONE)
		{
			this->index = index;
			return true;
		}
	}else if(amx_FindPublic(amx, handler.c_str(), &index) == AMX_ERR_NONE)
	{
		this->index = index;
		return true;
	}
	logwarn(amx, "[PP] Callback handler %s was not found.", handler.c_str());
	return false;
}

bool event_info::invoke(AMX *amx, cell *retval, cell id)
{
	int index;
	if(!handler_index(amx, index)) return false;

	int params = amx->paramcount;
	cell stk = amx->stk;

	cell flags = this->flags;

	std::vector<cell> oldargs;
	if(!(flags & 2))
	{
		cell *stk = reinterpret_cast<cell*>((amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + amx->stk);
		oldargs = {stk, stk + amx->paramcount};
	}

	cell heap, *heap_addr;
	amx_Allot(amx, 0, &heap, &heap_addr);

	cell *retarg = nullptr;
	if(flags & 1)
	{
		cell retaddr;
		amx_Allot(amx, 1, &retaddr, &retarg);
		amx_Push(amx, retaddr);
		*retarg = *retval;
	}

	for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
	{
		it->push(amx, id);
	}

	cell handled = 0;
	int err = amx_Exec(amx, &handled, index);

	amx_Release(amx, heap);

	if(flags & 2)
	{
		amx->paramcount = params;
		amx->stk = stk;
	}else for(auto it = oldargs.rbegin(); it != oldargs.rend(); it++)
	{
		amx_Push(amx, *it);
	}

	if(err) amx_RaiseError(amx, err);

	if(handled)
	{
		if(retarg)
		{
			*retval = *retarg;
		}else{
			*retval = handled;
		}
		return true;
	}
	return false;
}
