#include "events.h"
#include "amxinfo.h"
#include "hooks.h"
#include "objects/stored_param.h"

#include <unordered_map>
#include <cstring>
#include <memory>
#include <vector>

class event_info
{
	bool active;
	std::vector<stored_param> arg_values;
	std::string handler;
public:
	event_info(AMX *amx, const char *function, const char *format, const cell *args, size_t numargs);
	bool invoke(AMX *amx, cell *retval, int id) const;
	void deactivate();
	bool is_active() const;
};

class amx_info : public amx::extra
{
public:
	std::unordered_multimap<std::string, event_info> registered_events;
	std::vector<std::string> callback_ids;

	amx_info(AMX *amx) : amx::extra(amx)
	{

	}
};

amx_info &get_info(AMX *amx, amx::object &obj)
{
	obj = amx::load_lock(amx);
	return obj->get_extra<amx_info>();
}

struct not_enough_parameters {};
struct pool_empty {};

namespace events
{
	int register_callback(const char *callback, AMX *amx, const char *function, const char *format, const cell *params, int numargs)
	{
		try{
			amx::object obj;
			auto &registered_events = get_info(amx, obj).registered_events;
			auto it = registered_events.insert(std::make_pair(std::string(callback), event_info(amx, function, format, params, numargs)));
			return std::distance(registered_events.begin(), it);
		}catch(std::nullptr_t)
		{
			return -1;
		}
	}

	bool remove_callback(AMX *amx, int index)
	{
		amx::object obj;
		auto &registered_events = get_info(amx, obj).registered_events;
		auto it = registered_events.begin();
		std::advance(it, index);
		bool active = it->second.is_active();
		it->second.deactivate();
		return active;
	}

	int get_callback_id(AMX *amx, const char *callback)
	{
		amx::object obj;
		auto &info = get_info(amx, obj);
		auto &registered_events = info.registered_events;
		auto &callback_ids = info.callback_ids;

		std::string name(callback);
		if(registered_events.find(name) != registered_events.end())
		{
			int id = callback_ids.size();
			callback_ids.emplace_back(std::move(name));
			return id;
		}
		return -1;
	}

	const char *invoke_callback(int id, AMX *amx, cell *retval)
	{
		amx::object obj;
		auto &info = get_info(amx, obj);
		auto &registered_events = info.registered_events;
		auto &callback_ids = info.callback_ids;
		if(id < 0 || static_cast<size_t>(id) >= callback_ids.size()) return nullptr;

		auto range = registered_events.equal_range(callback_ids[id]);
		bool ok = false;
		while(range.first != range.second)
		{
			auto &ev = range.first->second;
			std::ptrdiff_t eid = std::distance(registered_events.begin(), range.first);
			if(ev.invoke(amx, retval, eid)) ok = true;
			range.first++;
		}

		return ok ? callback_ids[id].c_str() : nullptr;
	}

	bool get_name(int id, AMX *amx, char *funcname)
	{
		amx::object obj;
		auto &info = get_info(amx, obj);
		auto &registered_events = info.registered_events;
		auto &callback_ids = info.callback_ids;
		if(id < 0 || static_cast<size_t>(id) >= callback_ids.size()) return false;
		std::strcpy(funcname, callback_ids[id].c_str());
		return true;
	}
}

event_info::event_info(AMX *amx, const char *function, const char *format, const cell *args, size_t numargs)
	: handler(function)
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

void event_info::deactivate()
{
	active = false;
	arg_values.clear();
	handler.clear();
}

bool event_info::is_active() const
{
	return active;
}

bool event_info::invoke(AMX *amx, cell *retval, int id) const
{
	if(!active) return false;
	int index;
	if(amx_FindPublicOrig(amx, handler.c_str(), &index)) return false;

	cell *stk = reinterpret_cast<cell*>((amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + amx->stk);
	std::vector<cell> oldargs(stk, stk + amx->paramcount);

	cell heap, *heap_addr;
	amx_Allot(amx, 0, &heap, &heap_addr);
	for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
	{
		it->push(amx, id);
	}

	int err = amx_Exec(amx, retval, index);

	amx_Release(amx, heap);
	for(auto it = oldargs.rbegin(); it != oldargs.rend(); it++)
	{
		amx_Push(amx, *it);
	}

	if(err) amx_RaiseError(amx, err);

	return true;
}
