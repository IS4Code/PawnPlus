#include "variants.h"
#include "natives.h"
#include "strings.h"
#include "errors.h"

object_pool<dyn_object> variants::pool;

dyn_object dyn_func_str_s(AMX *amx, cell str)
{
	strings::cell_string *ptr;
	if(strings::pool.get_by_id(str, ptr))
	{
		return dyn_object(ptr->data(), ptr->size() + 1, tags::find_tag(tags::tag_char));
	}
	if(str != 0)
	{
		amx_LogicError(errors::pointer_invalid, "string", str);
	}
	str = 0;
	return dyn_object(&str, 1, tags::find_tag(tags::tag_char));
}

cell *get_offsets(AMX *amx, cell offsets, cell &offsets_size)
{
	cell *offsets_addr = amx_GetAddrSafe(amx, offsets);
	for(cell i = 0; i < offsets_size; i++)
	{
		if(offsets_addr[i] == 0x80000000)
		{
			offsets_size = i;
			break;
		}
	}
	return offsets_addr;
}

cell dyn_func(AMX *amx, const dyn_object &obj, cell offset)
{
	native_return_tag = obj.get_tag();
	return obj.get_cell(offset);
}

cell dyn_func(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size)
{
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	native_return_tag = obj.get_tag();
	return obj.get_cell(offsets_addr, offsets_size);
}

cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offset, cell tag_id)
{
	if(!obj.tag_assignable(amx, tag_id)) return 0;
	cell *addr = amx_GetAddrSafe(amx, result);
	*addr = obj.get_cell(offset);
	return 1;
}

cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offsets, cell offsets_size, cell tag_id)
{
	if(!obj.tag_assignable(amx, tag_id)) return 0;
	cell *addr = amx_GetAddrSafe(amx, result);
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	*addr = obj.get_cell(offsets_addr, offsets_size);
	return 1;
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return obj.get_array(addr, size);
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	return obj.get_array(offsets_addr, offsets_size, addr, size);
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size, cell tag_id)
{
	if(!obj.tag_assignable(amx, tag_id)) return 0;
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return obj.get_array(addr, size);
}

cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size, cell tag_id)
{
	if(!obj.tag_assignable(amx, tag_id)) return 0;
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	return obj.get_array(offsets_addr, offsets_size, addr, size);
}

cell dyn_func_str(AMX *amx, const dyn_object &obj, cell amx_addr, cell size)
{
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return obj.get_array(addr, size);
}

cell dyn_func_str(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size)
{
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	return obj.get_array(offsets_addr, offsets_size, addr, size);
}

cell dyn_func_str_s(AMX *amx, const cell *offsets, cell offsets_size, const dyn_object &obj)
{
	cell size;
	auto addr = obj.get_array(offsets, offsets_size, size);
	if(addr)
	{
		if(size == 0)
		{
			return strings::pool.get_id(strings::pool.add());
		}else if(obj.is_cell() && *addr != 0)
		{
			return strings::pool.get_id(strings::pool.add(strings::convert(addr, 1, false)));
		}
		auto str = strings::convert(addr);
		if(str.size() > static_cast<ucell>(size))
		{
			str.resize(size);
		}
		return strings::pool.get_id(strings::pool.add(std::move(str)));
	}else{
		return 0;
	}
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj)
{
	return dyn_func_str_s(amx, nullptr, 0, obj);
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size)
{
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	return dyn_func_str_s(amx, offsets_addr, offsets_size, obj);
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell unused)
{
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	return dyn_func_str_s(amx, nullptr, 0, obj);
}

cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size, cell unused)
{
	if(!obj.tag_assignable(tags::find_tag(tags::tag_char))) return 0;
	cell *offsets_addr = get_offsets(amx, offsets, offsets_size);
	return dyn_func_str_s(amx, offsets_addr, offsets_size, obj);
}
