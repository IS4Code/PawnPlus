#include "variants.h"

object_pool<dyn_object> variants::pool;

cell variants::create(dyn_object &&obj)
{
	if(obj.empty()) return 0;
	return reinterpret_cast<cell>(pool.add(std::move(obj), true));
}

dyn_object variants::get(cell ptr)
{
	if(ptr != 0) return *reinterpret_cast<dyn_object*>(ptr);
	return dyn_object();
}

dyn_object dyn_func(AMX *amx, cell value, cell tag_id)
{
	return dyn_object(amx, value, tag_id);
}

dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell tag_id)
{
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return dyn_object(amx, addr, size, tag_id);
}

dyn_object dyn_func_str(AMX *amx, cell amx_addr)
{
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return dyn_object(amx, addr);
}

dyn_object dyn_func_var(AMX *amx, cell ptr)
{
	return variants::get(ptr);
}


cell dyn_func(AMX *amx, const dyn_object &obj, cell offset)
{
	cell result;
	if(obj.get_cell(offset, result)) return result;
	return 0;
}

cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offset, cell tag_id)
{
	if(!obj.check_tag(amx, tag_id)) return 0;
	cell *addr;
	amx_GetAddr(amx, result, &addr);
	if(obj.get_cell(offset, *addr)) return 1;
	return 0;
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size)
{
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return obj.get_array(addr, size);
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size, cell tag_id)
{
	if(!obj.check_tag(amx, tag_id)) return 0;
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return obj.get_array(addr, size);
}

cell dyn_func_var(AMX *amx, const dyn_object &obj)
{
	return variants::create(obj);
}
