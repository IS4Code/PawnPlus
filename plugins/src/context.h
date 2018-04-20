#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "sdk/amx/amx.h"
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

struct AMX_CONTEXT
{
	size_t task_object = -1;
	cell result = 0;
};

struct AMX_STATE
{
	std::stack<AMX_CONTEXT> contexts;
};

namespace Context
{
	void Init(AMX *amx);
	void remove_callback(AMX *amx);

	int Push(AMX *amx);
	int Pop(AMX *amx);

	bool IsValid(AMX *amx);
	AMX_STATE &GetState(AMX *amx);

	void Restore(AMX *amx, const AMX_CONTEXT &context);
	bool IsPresent(AMX *amx);
	AMX_CONTEXT &Get(AMX *amx);
	AMX_CONTEXT TryGet(AMX *amx);

	void RegisterGroundCallback(const std::function<void(AMX*)> &callback);

	template <class Func>
	void RegisterGroundCallback(const Func &callback)
	{
		return RegisterGroundCallback(std::function<void(AMX*)>(callback));
	}
}

#endif
