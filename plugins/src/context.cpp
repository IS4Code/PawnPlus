#include "context.h"
#include "main.h"

#include <unordered_map>
#include <forward_list>
#include <deque>
#include <stdexcept>

int globalExecLevel = 0;
bool groundRecursion = false;
bool runGroundCallback = false;

const int &amx::context_level = globalExecLevel;

struct AMX_STATE : public amx::extra
{
	std::deque<amx::context> contexts;

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

	std::forward_list<Func*> list;
	typename decltype(list)::iterator end = list.before_begin();
public:
	void add(Func *func)
	{
		end = list.emplace_after(end, std::move(func));
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
	get_state(amx, obj).contexts.emplace_back(amx, index);
	return globalExecLevel;
}

int amx::pop(AMX *amx)
{
	if(globalExecLevel == 0)
	{
		throw std::logic_error("[PawnPlus] Context stack imbalance.");
	}

	amx::object obj;
	auto &contexts = get_state(amx, obj).contexts;
	{
		auto top = std::move(contexts.back());
		contexts.pop_back();
	}

	globalExecLevel--;
	if(globalExecLevel == 0)
	{
		runGroundCallback = true;
		if(!groundRecursion)
		{
			groundRecursion = true;
			while(runGroundCallback)
			{
				runGroundCallback = false;
				ground_callbacks.invoke(amx);
			}
			groundRecursion = false;
		}
	}
	
	return globalExecLevel;
}

bool amx::has_context(AMX *amx)
{
	if(!is_main_thread)
	{
		return false;
	}
	amx::object obj;
	return get_state(amx, obj).contexts.size() > 0;
}

amx::context &amx::get_context(AMX *amx, amx::object &obj)
{
	if(!is_main_thread)
	{
		obj = std::make_shared<amx::instance>(amx);
		auto &state = obj->get_extra<AMX_STATE>();
		state.contexts.emplace_back(amx, 0);
		return state.contexts.back();
	}
	return get_state(amx, obj).contexts.back();
}

bool amx::has_parent_context(AMX *amx)
{
	if(!is_main_thread)
	{
		return false;
	}
	amx::object obj;
	return get_state(amx, obj).contexts.size() > 1;
}

amx::context &amx::get_parent_context(AMX *amx, amx::object &obj)
{
	if(!is_main_thread)
	{
		return get_context(amx, obj);
	}
	return *std::prev(get_state(amx, obj).contexts.end(), 2);
}

void amx::restore(AMX *amx, context &&context)
{
	amx::object obj;
	get_context(amx, obj) = std::move(context);
}

void amx::on_bottom(void(*callback)(AMX*))
{
	ground_callbacks.add(callback);
}
