#include "variants.h"
#include "strings.h"

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

dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell size2, cell tag_id)
{
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return dyn_object(amx, addr, size, size2, tag_id);
}

dyn_object dyn_func_str(AMX *amx, cell amx_addr)
{
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return dyn_object(amx, addr);
}

dyn_object dyn_func_str_s(AMX *amx, cell str)
{
	if(str == 0)
	{
		return dyn_object(&str, 1, tags::find_tag(tags::tag_char));
	}
	auto ptr = reinterpret_cast<strings::cell_string*>(str);
	return dyn_object(ptr->data(), ptr->size() + 1, tags::find_tag(tags::tag_char));
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
	if(!obj.tag_assignable(amx, tag_id)) return 0;
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
	if(!obj.tag_assignable(amx, tag_id)) return 0;
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return obj.get_array(addr, size);
}

cell dyn_func_str(AMX *amx, const dyn_object &obj, cell amx_addr, cell size)
{
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	cell *addr;
	amx_GetAddr(amx, amx_addr, &addr);
	return obj.get_array(addr, size);
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj)
{
	if(obj.get_size() == 0) return 0;
	return reinterpret_cast<cell>(strings::create(&obj[0], true, false, false));
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell unused)
{
	if(obj.get_size() == 0) return 0;
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	return reinterpret_cast<cell>(strings::create(&obj[0], true, false, false));
}

cell dyn_func_var(AMX *amx, const dyn_object &obj)
{
	return variants::create(obj);
}
