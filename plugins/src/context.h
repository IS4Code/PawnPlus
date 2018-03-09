#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "events.h"

#include "sdk/amx/amx.h"
#include <functional>
#include <stack>
#include <unordered_map>

struct AMX_CONTEXT
{
	size_t awaiting_task = -1;
	size_t task_object = -1;
	cell result = 0;
};

struct AMX_STATE
{
	std::stack<AMX_CONTEXT> contexts;

	std::unordered_multimap<std::string, Events::EventInfo> registered_events;
	std::vector<std::string> callback_ids;
};

namespace Context
{
	void Init(AMX *amx);
	void Remove(AMX *amx);

	int Push(AMX *amx);
	int Pop(AMX *amx);

	bool IsValid(AMX *amx);
	AMX_STATE &GetState(AMX *amx);

	void Restore(AMX *amx, const AMX_CONTEXT &context);
	bool IsPresent(AMX *amx);
	AMX_CONTEXT &Get(AMX *amx);

	void RegisterGroundCallback(const std::function<void(AMX*)> &callback);

	template <class Func>
	void RegisterGroundCallback(const Func &callback)
	{
		return RegisterGroundCallback(std::function<void(AMX*)>(callback));
	}
}

#endif
