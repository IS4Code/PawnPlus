#include "context.h"
#include "modules/tasks.h"

#include <unordered_map>
#include <forward_list>

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

int globalExecLevel = 0;

AMX_STATE &get_state(AMX *amx, amx::object &obj)
{
	obj = amx::load_lock(amx);
	return obj->get_extra<AMX_STATE>();
}

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

int Context::Push(AMX *amx)
{
	globalExecLevel++;
	amx::object obj;
	get_state(amx, obj).contexts.emplace();
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

	amx::object obj;
	get_state(amx, obj).contexts.pop();

	return globalExecLevel;
}

bool Context::IsPresent(AMX *amx)
{
	amx::object obj;
	return get_state(amx, obj).contexts.size() > 0;
}

AMX_STATE &Context::GetState(AMX *amx, amx::object &obj)
{
	return get_state(amx, obj);
}

AMX_CONTEXT &Context::Get(AMX *amx, amx::object &obj)
{
	return get_state(amx, obj).contexts.top();
}

void Context::Restore(AMX *amx, AMX_CONTEXT &&context)
{
	amx::object obj;
	get_state(amx, obj).contexts.top() = std::move(context);
}

void Context::RegisterGroundCallback(const std::function<void(AMX*)> &callback)
{
	ground_callbacks.add(callback);
}
