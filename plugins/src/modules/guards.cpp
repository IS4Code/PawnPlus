#include "guards.h"
#include "context.h"

#include "utils/id_set_pool.h"
#include <memory>

struct guards_extra : public amx::extra
{
	aux::id_set_pool<handle_t> pool;

	guards_extra(AMX *amx) : amx::extra(amx)
	{

	}
};

size_t guards::count(AMX *amx)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.size();
}

handle_t *guards::add(AMX *amx, dyn_object &&obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	auto ptr = guards.pool.add(handle_t(std::move(obj)));
	return ptr;
}

cell guards::get_id(AMX *amx, const handle_t *obj)
{
	return reinterpret_cast<cell>(obj);
}

bool guards::get_by_id(AMX *amx, cell id, handle_t *&obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.get_by_id(id, obj);
}

bool guards::contains(AMX *amx, const handle_t *obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();
	return guards.pool.contains(obj);
}

bool guards::free(AMX *amx, handle_t *obj)
{
	amx::object owner;
	auto &ctx = amx::get_context(amx, owner);
	auto &guards = ctx.get_extra<guards_extra>();

	auto it = guards.pool.find(obj);
	if(it != guards.pool.end())
	{
		guards.pool.erase(it);
	}
	return false;
}

size_t amx_guards::count(AMX *amx)
{
	const amx::object &owner = amx::load_lock(amx);
	auto &guards = owner->get_extra<guards_extra>();
	return guards.pool.size();
}

handle_t *amx_guards::add(AMX *amx, dyn_object &&obj)
{
	const amx::object &owner = amx::load_lock(amx);
	auto &guards = owner->get_extra<guards_extra>();
	auto ptr = guards.pool.add(handle_t(std::move(obj)));
	return ptr;
}

cell amx_guards::get_id(AMX *amx, const handle_t *obj)
{
	return reinterpret_cast<cell>(obj);
}

bool amx_guards::get_by_id(AMX *amx, cell id, handle_t *&obj)
{
	const amx::object &owner = amx::load_lock(amx);
	auto &guards = owner->get_extra<guards_extra>();
	return guards.pool.get_by_id(id, obj);
}

bool amx_guards::contains(AMX *amx, const handle_t *obj)
{
	const amx::object &owner = amx::load_lock(amx);
	auto &guards = owner->get_extra<guards_extra>();
	return guards.pool.contains(obj);
}

bool amx_guards::free(AMX *amx, handle_t *obj)
{
	const amx::object &owner = amx::load_lock(amx);
	auto &guards = owner->get_extra<guards_extra>();

	auto it = guards.pool.find(obj);
	if(it != guards.pool.end())
	{
		guards.pool.erase(it);
	}
	return false;
}
