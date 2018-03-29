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
		if(ptr == nullptr) return 0;
		delete ptr;
		return 1;
	}

	// native list_size(List:list);
	static cell AMX_NATIVE_CALL list_size(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		return static_cast<cell>(ptr->size());
	}

	// native list_size(List:list);
	static cell AMX_NATIVE_CALL list_clear(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		ptr->clear();
		return 1;
	}

	// native List:list_add(List:list, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		ptr->push_back(dyn_object(amx, params[2], params[3]));
		return static_cast<cell>(ptr->size() - 1);
	}

	// native List:list_add_arr(List:list, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
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
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		ptr->erase(ptr->begin() + params[1]);
		return 1;
	}

	// native list_get(List:list, index, offset=0);
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell result;
		if(obj.get_cell(params[3], result)) return result;
		return 0;
	}

	// native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL list_get_arr(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		return obj.get_array(0, addr, static_cast<size_t>(params[4]));
	}

	// native bool:list_get_checked(List:list, index, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_checked(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell tag_id = params[5];
		if(!obj.check_tag(amx, tag_id)) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		if(obj.get_cell(params[4], *addr)) return 1;
		return 0;
	}

	// native list_get_arr_checked(List:list, index, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_arr_checked(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell tag_id = params[5];
		if(!obj.check_tag(amx, tag_id)) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		return obj.get_array(0, addr, params[4]);
	}

	// native list_set(List:list, index, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		(*ptr)[params[2]] = dyn_object(amx, params[3], params[4]);
		return 1;
	}

	// native list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set_arr(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[params[2]] = dyn_object(amx, addr, static_cast<size_t>(params[4]), params[5]);
		return 1;
	}

	// native list_set_arr_elem(List:list, index, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[3] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.set_cell(params[3], params[4]);
	}

	// native bool:list_set_cell_checked(List:list, index, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set_cell_checked(AMX *amx, cell *params)
	{
		if(params[2] < 0 || params[3] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		cell tag_id = params[5];
		if(!obj.check_tag(amx, tag_id)) return 0;
		return obj.set_cell(params[3], params[4]);
	}

	// native list_tagof(List:list, index);
	static cell AMX_NATIVE_CALL list_tagof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native list_sizeof(List:list, index);
	static cell AMX_NATIVE_CALL list_sizeof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
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
	AMX_DECLARE_NATIVE(list_set_cell),
	AMX_DECLARE_NATIVE(list_set_cell_checked),
	AMX_DECLARE_NATIVE(list_tagof),
	AMX_DECLARE_NATIVE(list_sizeof),
};

int RegisterListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
