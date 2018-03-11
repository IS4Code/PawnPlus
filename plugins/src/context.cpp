#include "context.h"
#include "tasks.h"

#include <unordered_map>
#include <forward_list>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

int globalExecLevel = 0;


std::unordered_map<AMX*, AMX_STATE> amx_infos;

template <class Ret, class... Args>
class invocation_list
{
	typedef Ret (Func)(Args...);

	std::forward_list<std::function<Func>> list;
	typename std::forward_list<std::function<Func>>::iterator end = list.before_begin();
public:
	void add(std::function<Func> &&func)
	{
		end = list.emplace_after(end, func);
	}

	void add(const std::function<Func> &func)
	{
		end = list.emplace_after(end, func);
	}

	void invoke(Args... args)
	{
		for(const auto &callback : list)
		{
			callback(std::forward<Args>(args)...);
		}
	}
};

invocation_list<void, AMX*> ground_callbacks;

void Context::Init(AMX *amx)
{
	amx_infos[amx];
}

void Context::Remove(AMX *amx)
{
	amx_infos.erase(amx);
}

int Context::Push(AMX *amx)
{
	globalExecLevel++;
	amx_infos[amx].contexts.emplace();

	return globalExecLevel;
}

int Context::Pop(AMX *amx)
{
	if(globalExecLevel == 0)
	{
		//error
	}
	globalExecLevel--;
	if(globalExecLevel == 0)
	{
		ground_callbacks.invoke(amx);
	}

	auto &info = amx_infos.at(amx);

	info.contexts.pop();

	return globalExecLevel;
}

bool Context::IsValid(AMX *amx)
{
	return amx_infos.find(amx) != amx_infos.end();
}

bool Context::IsPresent(AMX *amx)
{
	return amx_infos.at(amx).contexts.size() > 0;
}

AMX_STATE &Context::GetState(AMX *amx)
{
	return amx_infos[amx];
}

AMX_CONTEXT &Context::Get(AMX *amx)
{
	auto &info = amx_infos.at(amx);
	return info.contexts.top();
}

void Context::Restore(AMX *amx, const AMX_CONTEXT &context)
{
	amx_infos.at(amx).contexts.top() = context;
}

void Context::RegisterGroundCallback(const std::function<void(AMX*)> &callback)
{
	ground_callbacks.add(callback);
}
