#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "utils/set_pool.h"
#include "objects/dyn_object.h"
#include "sdk/amx/amx.h"
#include "amxinfo.h"
#include <functional>
#include <stack>
#include <unordered_map>

constexpr cell SleepReturnTypeMask = 0xFF000000;
constexpr cell SleepReturnValueMask = 0x00FFFFFF;

constexpr cell SleepReturnAwait = 0xA0000000;
constexpr cell SleepReturnWaitMs = 0xA1000000;
constexpr cell SleepReturnWaitTicks = 0xA2000000;
constexpr cell SleepReturnWaitInf = 0xA3000000;
constexpr cell SleepReturnDetach = 0xB0000000;
constexpr cell SleepReturnAttach = 0xB1000000;
constexpr cell SleepReturnSync = 0xB2000000;

struct AMX_CONTEXT : public amx::extra
{
	size_t task_object = -1;
	cell result = 0;
	aux::set_pool<dyn_object> guards;

	AMX_CONTEXT() : amx::extra(nullptr)
	{

	}

	AMX_CONTEXT(AMX *amx) : amx::extra(amx)
	{

	}

	AMX_CONTEXT(const AMX_CONTEXT &obj) = delete;
	AMX_CONTEXT(AMX_CONTEXT &&obj) : amx::extra(nullptr), guards(std::move(obj.guards))
	{
		obj.guards.clear();
	}

	AMX_CONTEXT &operator=(const AMX_CONTEXT &obj) = delete;
	AMX_CONTEXT &operator=(AMX_CONTEXT &&obj)
	{
		if(this != &obj)
		{
			guards = std::move(obj.guards);
			obj.guards.clear();
		}
		return *this;
	}

	virtual ~AMX_CONTEXT()
	{
		for(auto obj : guards)
		{
			obj->free();
		}
	}
};

struct AMX_STATE
{
	std::stack<AMX_CONTEXT> contexts;
};

namespace Context
{
	int Push(AMX *amx);
	int Pop(AMX *amx);

	bool IsValid(AMX *amx);
	AMX_STATE &GetState(AMX *amx);

	void Restore(AMX *amx, AMX_CONTEXT &&context);
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
