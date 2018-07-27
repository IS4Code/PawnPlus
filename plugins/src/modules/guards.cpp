#include "guards.h"
#include "context.h"

#include "utils/id_set_pool.h"

struct guards_extra : public amx::extra
{
	aux::id_set_pool<dyn_object> pool;

	guards_extra(AMX *amx) : amx::extra(amx)
	{

	}

	virtual ~guards_extra()
	{
		for(auto &guard : pool)
		{
			guard->free();
		}
	}
};

size_t guards::count(AMX *amx)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.size();
}

dyn_object *guards::add(AMX *amx, dyn_object &&obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.add(std::move(obj));
}

cell guards::get_id(AMX *amx, const dyn_object *obj)
{
	return reinterpret_cast<cell>(obj);
}

bool guards::get_by_id(AMX *amx, cell id, dyn_object *&obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.get_by_id(id, obj);
}

bool guards::contains(AMX *amx, const dyn_object *obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.contains(obj);
}

bool guards::free(AMX *amx, dyn_object *obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();

	auto it = guards.pool.find(obj);
	if(it != guards.pool.end())
	{
		obj->free();
		guards.pool.erase(it);
	}
	return false;
}
