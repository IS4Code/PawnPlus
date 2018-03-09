#include "events.h"
#include "hooks.h"
#include "context.h"

#include <unordered_map>
#include <string.h>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

namespace Events
{

	ptrdiff_t Register(const char *callback, AMX *amx, const char *function, const char *format, const cell *params, int numargs)
	{
		try{
			auto &registered_events = Context::GetState(amx).registered_events;
			auto it = registered_events.insert(std::make_pair(std::string(callback), EventInfo(amx, function, format, params, numargs)));
			return std::distance(registered_events.begin(), it);
		}catch(int)
		{
			return -1;
		}
	}

	bool Remove(AMX *amx, ptrdiff_t index)
	{
		auto &registered_events = Context::GetState(amx).registered_events;
		auto it = registered_events.begin();
		std::advance(it, index);
		bool active = it->second.IsActive();
		it->second.Deactivate();
		return active;
	}


	int GetCallbackId(AMX *amx, const char *callback)
	{
		auto &registered_events = Context::GetState(amx).registered_events;
		auto &callback_ids = Context::GetState(amx).callback_ids;

		std::string name(callback);
		if(registered_events.find(name) != registered_events.end())
		{
			int id = callback_ids.size();
			callback_ids.emplace_back(std::move(name));
			return id;
		}
		return -1;
	}

	const char *Invoke(int id, AMX *amx, cell *retval)
	{
		auto &registered_events = Context::GetState(amx).registered_events;
		auto &callback_ids = Context::GetState(amx).callback_ids;
		if(id < 0 || static_cast<size_t>(id) >= callback_ids.size()) return nullptr;

		auto range = registered_events.equal_range(callback_ids[id]);
		bool ok = false;
		while(range.first != range.second)
		{
			auto &ev = range.first->second;
			ptrdiff_t eid = std::distance(registered_events.begin(), range.first);
			if(ev.Invoke(amx, retval, eid)) ok = true;
			range.first++;
		}

		return ok ? callback_ids[id].c_str() : nullptr;
	}

	EventParam::EventParam() : Type(ParamType::EventId)
	{

	}

	EventParam::EventParam(cell val) : Type(ParamType::Cell), CellValue(val)
	{

	}

	EventParam::EventParam(std::basic_string<cell> &&str) : Type(ParamType::String), StringValue(std::move(str))
	{

	}

	EventParam::EventParam(const EventParam &obj) : Type(obj.Type), CellValue(obj.CellValue)
	{
		if(Type == ParamType::String)
		{
			new (&StringValue) std::basic_string<cell>(obj.StringValue);
		}
	}

	EventParam::~EventParam()
	{
		if(Type == ParamType::String)
		{
			StringValue.~basic_string();
		}
	}

	EventInfo::EventInfo(AMX *amx, const char *function, const char *format, const cell *args, size_t numargs)
		: handler(function)
	{
		if(format != nullptr)
		{
			size_t argi = -1;
			size_t len = strlen(format);
			for(size_t i = 0; i < len; i++)
			{
				cell *addr, *addr2;
				switch(format[i])
				{
					case 'a':
						if(++argi >= numargs) throw 0;
						amx_GetAddr(amx, args[argi], &addr);
						if(++argi >= numargs) throw 0;
						amx_GetAddr(amx, args[argi], &addr2);
						argvalues.emplace_back(std::basic_string<cell>(addr, *addr2));
						//logprintf("str is %d", argvalues.emplace_back(std::basic_string<cell>(addr, args[argi])).StringValue.size());
						break;
					case 's':
						if(++argi >= numargs) throw 0;
						amx_GetAddr(amx, args[argi], &addr);
						argvalues.emplace_back(std::basic_string<cell>(addr));
						break;
					case 'e':
						argvalues.emplace_back();
						break;
					default:
						if(++argi >= numargs) throw 0;
						amx_GetAddr(amx, args[argi], &addr);
						argvalues.emplace_back(*addr);
						break;
				}
			}
		}
	}


	void EventInfo::Deactivate()
	{
		active = false;
		argvalues.clear();
		handler.clear();
	}

	bool EventInfo::IsActive()
	{
		return active;
	}

	bool EventInfo::Invoke(AMX *amx, cell *retval, ptrdiff_t id)
	{
		if(!active) return false;
		int index;
		if(amx_FindPublicOrig(amx, handler.c_str(), &index)) return false;

		cell *stk = reinterpret_cast<cell*>((amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + amx->stk);
		std::vector<cell> oldargs(stk, stk + amx->paramcount);

		cell heap, *heap_addr;
		amx_Allot(amx, 0, &heap, &heap_addr);
		int pos = argvalues.size() - 1;
		while(pos >= 0)
		{
			auto &arg = argvalues[pos];
			switch(arg.Type)
			{
				case ParamType::Cell:
					amx_Push(amx, arg.CellValue);
					break;
				case ParamType::EventId:
					amx_Push(amx, id);
					break;
				case ParamType::String:
					cell addr, *phys_addr;
					amx_PushArray(amx, &addr, &phys_addr, arg.StringValue.c_str(), arg.StringValue.size()+1);
					break;
			}
			pos--;
		}

		int err = amx_Exec(amx, retval, index);

		amx_Release(amx, heap);

		pos = oldargs.size() - 1;
		while(pos >= 0)
		{
			amx_Push(amx, oldargs[pos]);
			pos--;
		}

		if(err) amx_RaiseError(amx, err);

		return true;
	}
}
