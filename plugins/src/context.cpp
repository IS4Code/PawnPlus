#include "context.h"

#include <unordered_map>
#include <forward_list>
#include <exception>

int globalExecLevel = 0;

struct AMX_STATE : public amx::extra
{
	std::stack<amx::context> contexts;

	AMX_STATE(AMX *amx) : amx::extra(amx)
	{

	}
};

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

int amx::push(AMX *amx, int index)
{
	globalExecLevel++;
	amx::object obj;
	get_state(amx, obj).contexts.emplace(amx, index);
	return globalExecLevel;
}

int amx::pop(AMX *amx)
{
	if(globalExecLevel == 0)
	{
		throw std::exception("[PP] Context stack imbalance.");
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

bool amx::has_context(AMX *amx)
{
	amx::object obj;
	return get_state(amx, obj).contexts.size() > 0;
}

amx::context &amx::get_context(AMX *amx, amx::object &obj)
{
	return get_state(amx, obj).contexts.top();
}

void amx::restore(AMX *amx, context &&context)
{
	amx::object obj;
	get_state(amx, obj).contexts.top() = std::move(context);
}

void amx::on_bottom(const std::function<void(AMX*)> &callback)
{
	ground_callbacks.add(callback);
}
