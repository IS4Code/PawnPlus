#include "../natives.h"
#include "../utils/dyn_object.h"
#include <vector>
#include <cstring>

namespace Natives
{
	// native List:list_new();
	static cell AMX_NATIVE_CALL list_new(AMX *amx, cell *params)
	{
		auto ptr = new std::vector<dyn_object>();
		return reinterpret_cast<cell>(ptr);
	}

	// native list_delete(List:list);
	static cell AMX_NATIVE_CALL list_delete(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		delete ptr;
		return 0;
	}

	// native list_size(List:list);
	static cell AMX_NATIVE_CALL list_size(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native list_size(List:list);
	static cell AMX_NATIVE_CALL list_clear(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		ptr->clear();
		return 0;
	}

	// native List:list_add(List:list, ListItemTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		ptr->push_back(dyn_object(amx, params[2], params[3]));
		return static_cast<cell>(ptr->size() - 1);
	}

	// native List:list_add_arr(List:list, const ListItemTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		ptr->push_back(dyn_object(amx, addr, static_cast<size_t>(params[3]), params[4]));
		return static_cast<cell>(ptr->size() - 1);
	}

	// native bool:list_remove(List:list, index);
	static cell AMX_NATIVE_CALL list_remove(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		ptr->erase(ptr->begin() + params[1]);
		return 1;
	}

	// native list_get(List:list, index);
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		if(obj.is_array)
		{
			return obj.array_value[0];
		} else {
			return obj.cell_value;
		}
	}

	// native list_get_arr(List:list, index, ListItemTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL list_get_arr(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		if(obj.is_array)
		{
			size_t size = static_cast<size_t>(params[4]);
			if(size > obj.array_size) size = obj.array_size;
			std::memcpy(addr, obj.array_value.get(), size * sizeof(cell));
			return static_cast<cell>(size);
		}else{
			*addr = obj.cell_value;
			return 1;
		}
	}

	// native bool:list_get_checked(List:list, index, &ListItemTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_checked(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		cell tag_id = params[4];
		if(!obj.check_tag(amx, tag_id)) return 0;
		if(obj.is_array)
		{
			*addr = obj.array_value[0];
			return 1;
		}else{
			*addr = obj.cell_value;
			return 1;
		}
	}

	// native list_get_arr_checked(List:list, index, ListItemTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_arr_checked(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		cell tag_id = params[5];
		if(!obj.check_tag(amx, tag_id)) return 0;
		if(obj.is_array)
		{
			size_t size = static_cast<size_t>(params[4]);
			if(size > obj.array_size) size = obj.array_size;
			std::memcpy(addr, obj.array_value.get(), size * sizeof(cell));
			return static_cast<cell>(size);
		}else{
			*addr = obj.cell_value;
			return 1;
		}
	}

	// native List:list_set(List:list, index, ListItemTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		(*ptr)[params[2]] = dyn_object(amx, params[3], params[4]);
		return 1;
	}

	// native List:list_set_arr(List:list, index, const ListItemTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set_arr(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[params[2]] = dyn_object(amx, addr, static_cast<size_t>(params[4]), params[5]);
		return 1;
	}

	// native list_tagof(List:list, index);
	static cell AMX_NATIVE_CALL list_tagof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];

		if(obj.tag_name.empty()) return 0x80000000;

		int len;
		amx_NameLength(amx, &len);
		char *tagname = static_cast<char*>(alloca(len));

		int num;
		amx_NumTags(amx, &num);
		for(int i = 0; i < num; i++)
		{
			cell tag_id;
			if(!amx_GetTag(amx, i, tagname, &tag_id))
			{
				if(obj.tag_name == tagname)
				{
					return tag_id | 0x80000000;
				}
			}
		}
		return 0;
	}

	// native list_sizeof(List:list, index);
	static cell AMX_NATIVE_CALL list_sizeof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];

		if(obj.is_array)
		{
			return obj.array_size;
		}else{
			return 1;
		}
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_new),
	AMX_DECLARE_NATIVE(list_delete),
	AMX_DECLARE_NATIVE(list_size),
	AMX_DECLARE_NATIVE(list_clear),
	AMX_DECLARE_NATIVE(list_add),
	AMX_DECLARE_NATIVE(list_add_arr),
	AMX_DECLARE_NATIVE(list_remove),
	AMX_DECLARE_NATIVE(list_get),
	AMX_DECLARE_NATIVE(list_get_arr),
	AMX_DECLARE_NATIVE(list_get_checked),
	AMX_DECLARE_NATIVE(list_get_arr_checked),
	AMX_DECLARE_NATIVE(list_set),
	AMX_DECLARE_NATIVE(list_set_arr),
	AMX_DECLARE_NATIVE(list_tagof),
	AMX_DECLARE_NATIVE(list_sizeof),
};

int RegisterListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
