#include "../natives.h"
#include "../variants.h"
#include <vector>

typedef std::vector<dyn_object> list_t;

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native list_add(List:list, value, index=-1, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_add(AMX *amx, cell *params)
	{
		if(params[3] < -1) return -1;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return -1;
		if(params[3] == -1)
		{
			ptr->push_back(Factory(amx, params[Indices]...));
			return static_cast<cell>(ptr->size() - 1);
		} else if(static_cast<ucell>(params[3]) > ptr->size())
		{
			return -1;
		} else {
			ptr->insert(ptr->begin() + params[3], Factory(amx, params[Indices]...));
			return params[3];
		}
	}

	// native list_set(List:list, index, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		(*ptr)[params[2]] = Factory(amx, params[Indices]...);
		return 1;
	}

	// native list_get(List:list, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		return Factory(amx, (*ptr)[params[2]], params[Indices]...);
	}
};

// native bool:list_set_cell(List:list, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0 || params[3] < 0) return 0;
	auto ptr = reinterpret_cast<list_t*>(params[1]);
	if(ptr == nullptr) return 0;
	if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
	auto &obj = (*ptr)[params[2]];
	if(TagIndex && !obj.check_tag(amx, params[TagIndex])) return 0;
	return obj.set_cell(params[3], params[4]);
}

namespace Natives
{
	// native List:list_new();
	static cell AMX_NATIVE_CALL list_new(AMX *amx, cell *params)
	{
		auto ptr = new list_t();
		return reinterpret_cast<cell>(ptr);
	}

	// native List:list_new_args_tagged(tag_id=tagof(arg0), AnyTag:arg0=cellmin, AnyTag:...);
	static cell AMX_NATIVE_CALL list_new_args_tagged(AMX *amx, cell *params)
	{
		auto ptr = new list_t();
		cell numargs = (params[0] / sizeof(cell)) - 1;
		if(numargs >= 1 && params[2] != 0x80000000)
		{
			for(cell arg = 1; arg <= numargs; arg++)
			{
				ptr->push_back(dyn_object(amx, params[1 + arg], params[1]));
			}
		}
		return reinterpret_cast<cell>(ptr);
	}

	// native list_delete(List:list);
	static cell AMX_NATIVE_CALL list_delete(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		delete ptr;
		return 1;
	}

	// native list_size(List:list);
	static cell AMX_NATIVE_CALL list_size(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return -1;
		return static_cast<cell>(ptr->size());
	}

	// native list_clear(List:list);
	static cell AMX_NATIVE_CALL list_clear(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		ptr->clear();
		return 1;
	}

	// native list_add(List:list, AnyTag:value, index=-1, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add(AMX *amx, cell *params)
	{
		return value_at<2, 4>::list_add<dyn_func>(amx, params);
	}

	// native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_add_arr(AMX *amx, cell *params)
	{
		return value_at<2, 4, 5>::list_add<dyn_func_arr>(amx, params);
	}

	// native list_add_str(List:list, const value[], index=-1);
	static cell AMX_NATIVE_CALL list_add_str(AMX *amx, cell *params)
	{
		return value_at<2>::list_add<dyn_func_str>(amx, params);
	}

	// native list_add_var(List:list, VariantTag:value, index=-1);
	static cell AMX_NATIVE_CALL list_add_var(AMX *amx, cell *params)
	{
		return value_at<2>::list_add<dyn_func_var>(amx, params);
	}

	// native list_add_list(List:list, List:range, index=-1);
	static cell AMX_NATIVE_CALL list_add_list(AMX *amx, cell *params)
	{
		if(params[3] < -1) return -1;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		auto ptr2 = reinterpret_cast<list_t*>(params[2]);
		if(ptr == nullptr || ptr2 == nullptr) return -1;
		if(params[3] == -1)
		{
			size_t index = ptr->size();
			ptr->insert(ptr->end(), ptr2->begin(), ptr2->end());
			return static_cast<cell>(index);
		} else if(static_cast<ucell>(params[3]) > ptr->size())
		{
			return -1;
		} else {
			ptr->insert(ptr->begin() + params[3], ptr2->begin(), ptr2->end());
			return params[3];
		}
	}

	// native list_add_args_tagged(tag_id=tagof(arg0), List:list, AnyTag:arg0=cellmin, AnyTag:...);
	static cell AMX_NATIVE_CALL list_add_args_tagged(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<list_t*>(params[2]);
		if(ptr == nullptr) return -1;
		cell numargs = (params[0] / sizeof(cell)) - 2;
		if(numargs >= 1 && params[3] != 0x80000000)
		{
			for(cell arg = 1; arg <= numargs; arg++)
			{
				ptr->push_back(dyn_object(amx, params[2 + arg], params[1]));
			}
		}
		return numargs;
	}

	// native bool:list_remove(List:list, index);
	static cell AMX_NATIVE_CALL list_remove(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		ptr->erase(ptr->begin() + params[1]);
		return 1;
	}

	// native list_get(List:list, index, offset=0);
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		return value_at<3>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL list_get_arr(AMX *amx, cell *params)
	{
		return value_at<3, 4>::list_get<dyn_func_arr>(amx, params);
	}

	// native Variant:list_get_var(List:list, index);
	static cell AMX_NATIVE_CALL list_get_var(AMX *amx, cell *params)
	{
		return value_at<>::list_get<dyn_func_var>(amx, params);
	}

	// native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_safe(AMX *amx, cell *params)
	{
		return value_at<3, 4, 5>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_get_arr_safe(AMX *amx, cell *params)
	{
		return value_at<3, 4, 5>::list_get<dyn_func_arr>(amx, params);
	}

	// native list_set(List:list, index, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		return value_at<3, 4>::list_set<dyn_func>(amx, params);
	}

	// native list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set_arr(AMX *amx, cell *params)
	{
		return value_at<3, 4, 5>::list_set<dyn_func_arr>(amx, params);
	}

	// native list_set_str(List:list, index, const value[]);
	static cell AMX_NATIVE_CALL list_set_str(AMX *amx, cell *params)
	{
		return value_at<3>::list_set<dyn_func_str>(amx, params);
	}

	// native list_set_var(List:list, index, VariantTag:value);
	static cell AMX_NATIVE_CALL list_set_var(AMX *amx, cell *params)
	{
		return value_at<3>::list_set<dyn_func_var>(amx, params);
	}

	// native list_set_cell(List:list, index, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
	{
		return ::list_set_cell(amx, params);
	}

	// native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL list_set_cell_safe(AMX *amx, cell *params)
	{
		return ::list_set_cell<5>(amx, params);
	}

	// native list_tagof(List:list, index);
	static cell AMX_NATIVE_CALL list_tagof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native list_sizeof(List:list, index);
	static cell AMX_NATIVE_CALL list_sizeof(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		auto ptr = reinterpret_cast<list_t*>(params[1]);
		if(ptr == nullptr) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_new),
	AMX_DECLARE_NATIVE(list_new_args_tagged),
	AMX_DECLARE_NATIVE(list_delete),
	AMX_DECLARE_NATIVE(list_size),
	AMX_DECLARE_NATIVE(list_clear),
	AMX_DECLARE_NATIVE(list_add),
	AMX_DECLARE_NATIVE(list_add_arr),
	AMX_DECLARE_NATIVE(list_add_str),
	AMX_DECLARE_NATIVE(list_add_var),
	AMX_DECLARE_NATIVE(list_add_list),
	AMX_DECLARE_NATIVE(list_add_args_tagged),
	AMX_DECLARE_NATIVE(list_remove),
	AMX_DECLARE_NATIVE(list_get),
	AMX_DECLARE_NATIVE(list_get_arr),
	AMX_DECLARE_NATIVE(list_get_var),
	AMX_DECLARE_NATIVE(list_get_safe),
	AMX_DECLARE_NATIVE(list_get_arr_safe),
	AMX_DECLARE_NATIVE(list_set),
	AMX_DECLARE_NATIVE(list_set_arr),
	AMX_DECLARE_NATIVE(list_set_str),
	AMX_DECLARE_NATIVE(list_set_var),
	AMX_DECLARE_NATIVE(list_set_cell),
	AMX_DECLARE_NATIVE(list_set_cell_safe),
	AMX_DECLARE_NATIVE(list_tagof),
	AMX_DECLARE_NATIVE(list_sizeof),
};

int RegisterListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
