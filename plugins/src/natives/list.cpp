#include "natives.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include <vector>

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
		cell index = optparam(3, -1);
		if(index < -1) return -1;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		if(index == -1)
		{
			ptr->push_back(Factory(amx, params[Indices]...));
			return static_cast<cell>(ptr->size() - 1);
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			return -1;
		}else{
			ptr->insert(ptr->begin() + index, Factory(amx, params[Indices]...));
			return index;
		}
	}

	// native list_set(List:list, index, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		(*ptr)[params[2]] = Factory(amx, params[Indices]...);
		return 1;
	}

	// native list_get(List:list, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) return 0;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		return Factory(amx, (*ptr)[params[2]], params[Indices]...);
	}
};

// native bool:list_set_cell(List:list, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0 || params[3] < 0) return 0;
	list_t *ptr;
	if(!list_pool.get_by_id(params[1], ptr)) return -1;
	if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
	auto &obj = (*ptr)[params[2]];
	if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
	return obj.set_cell({params[3]}, params[4]);
}

namespace Natives
{
	// native List:list_new();
	AMX_DEFINE_NATIVE(list_new, 0)
	{
		return list_pool.get_id(list_pool.add());
	}

	// native List:list_new_arr(AnyTag:values[], size=sizeof(values), tag_id=tagof(values));
	AMX_DEFINE_NATIVE(list_new_arr, 3)
	{
		auto ptr = list_pool.add();
		cell *arr;
		amx_GetAddr(amx, params[1], &arr);

		for(cell i = 0; i < params[2]; i++)
		{
			ptr->push_back(dyn_object(amx, arr[i], params[3]));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args(tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE(list_new_args, 0)
	{
		auto ptr = list_pool.add();
		cell numargs = (params[0] / sizeof(cell)) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[2 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				amx_GetAddr(amx, param, &addr);
			}
			ptr->push_back(dyn_object(amx, *addr, params[1]));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args_str(arg0[], ...);
	AMX_DEFINE_NATIVE(list_new_args_str, 0)
	{
		auto ptr = list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[1 + arg];
			amx_GetAddr(amx, param, &addr);
			ptr->push_back(dyn_object(amx, addr));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args_var(VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE(list_new_args_var, 0)
	{
		auto ptr = list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[1 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				amx_GetAddr(amx, param, &addr);
			}
			ptr->push_back(variants::get(*addr));
		}
		return list_pool.get_id(ptr);
	}

	// native bool:list_valid(List:list);
	AMX_DEFINE_NATIVE(list_valid, 1)
	{
		list_t *ptr;
		return list_pool.get_by_id(params[1], ptr);
	}

	// native bool:list_delete(List:list);
	AMX_DEFINE_NATIVE(list_delete, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		return list_pool.remove(ptr);
	}

	// native bool:list_delete_deep(List:list);
	AMX_DEFINE_NATIVE(list_delete_deep, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		for(auto &obj : *ptr)
		{
			obj.release();
		}
		return list_pool.remove(ptr);
	}

	// native List:list_clone(List:list);
	AMX_DEFINE_NATIVE(list_clone, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		auto l = list_pool.add();
		for(auto &&obj : *ptr)
		{
			l->push_back(obj.clone());
		}
		return list_pool.get_id(l);
	}

	// native list_size(List:list);
	AMX_DEFINE_NATIVE(list_size, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		return static_cast<cell>(ptr->size());
	}

	// native list_clear(List:list);
	AMX_DEFINE_NATIVE(list_clear, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		ptr->clear();
		return 1;
	}

	// native list_add(List:list, AnyTag:value, index=-1, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_add, 4)
	{
		return value_at<2, 4>::list_add<dyn_func>(amx, params);
	}

	// native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_add_arr, 5)
	{
		return value_at<2, 4, 5>::list_add<dyn_func_arr>(amx, params);
	}

	// native list_add_str(List:list, const value[], index=-1);
	AMX_DEFINE_NATIVE(list_add_str, 2)
	{
		return value_at<2>::list_add<dyn_func_str>(amx, params);
	}

	// native list_add_var(List:list, VariantTag:value, index=-1);
	AMX_DEFINE_NATIVE(list_add_var, 2)
	{
		return value_at<2>::list_add<dyn_func_var>(amx, params);
	}

	// native list_add_list(List:list, List:range, index=-1);
	AMX_DEFINE_NATIVE(list_add_list, 2)
	{
		if(params[3] < -1) return -1;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		list_t *ptr2;
		if(!list_pool.get_by_id(params[2], ptr2)) return -1;
		if(params[3] == -1)
		{
			size_t index = ptr->size();
			ptr->insert(ptr->end(), ptr2->begin(), ptr2->end());
			return static_cast<cell>(index);
		}else if(static_cast<ucell>(params[3]) > ptr->size())
		{
			return -1;
		}else{
			ptr->insert(ptr->begin() + params[3], ptr2->begin(), ptr2->end());
			return params[3];
		}
	}

	// native list_add_args(tag_id=tagof(arg0), List:list, AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE(list_add_args, 2)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[2], ptr)) return -1;
		cell numargs = (params[0] / sizeof(cell)) - 2;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[3 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				amx_GetAddr(amx, param, &addr);
			}
			ptr->push_back(dyn_object(amx, *addr, params[1]));
		}
		return numargs;
	}

	// native list_add_args_str(List:list, arg0[], ...);
	AMX_DEFINE_NATIVE(list_add_args_str, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[2 + arg];
			amx_GetAddr(amx, param, &addr);
			ptr->push_back(dyn_object(amx, addr));
		}
		return numargs;
	}

	// native list_add_args_var(List:list, VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE(list_add_args_var, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return -1;
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[2 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				amx_GetAddr(amx, param, &addr);
			}
			ptr->push_back(variants::get(*addr));
		}
		return numargs;
	}

	// native bool:list_remove(List:list, index);
	AMX_DEFINE_NATIVE(list_remove, 2)
	{
		if(params[2] < 0) return 0;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		ptr->erase(ptr->begin() + params[2]);
		return 1;
	}

	// native list_get(List:list, index, offset=0);
	AMX_DEFINE_NATIVE(list_get, 3)
	{
		return value_at<3>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(list_get_arr, 4)
	{
		return value_at<3, 4>::list_get<dyn_func_arr>(amx, params);
	}

	// native Variant:list_get_var(List:list, index);
	AMX_DEFINE_NATIVE(list_get_var, 2)
	{
		return value_at<>::list_get<dyn_func_var>(amx, params);
	}

	// native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_get_safe, 5)
	{
		return value_at<3, 4, 5>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_get_arr_safe, 5)
	{
		return value_at<3, 4, 5>::list_get<dyn_func_arr>(amx, params);
	}

	// native list_set(List:list, index, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_set, 4)
	{
		return value_at<3, 4>::list_set<dyn_func>(amx, params);
	}

	// native list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_set_arr, 5)
	{
		return value_at<3, 4, 5>::list_set<dyn_func_arr>(amx, params);
	}

	// native list_set_str(List:list, index, const value[]);
	AMX_DEFINE_NATIVE(list_set_str, 3)
	{
		return value_at<3>::list_set<dyn_func_str>(amx, params);
	}

	// native list_set_var(List:list, index, VariantTag:value);
	AMX_DEFINE_NATIVE(list_set_var, 3)
	{
		return value_at<3>::list_set<dyn_func_var>(amx, params);
	}

	// native list_set_cell(List:list, index, offset, AnyTag:value);
	AMX_DEFINE_NATIVE(list_set_cell, 3)
	{
		return ::list_set_cell(amx, params);
	}

	// native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_set_cell_safe, 5)
	{
		return ::list_set_cell<5>(amx, params);
	}

	// native list_tagof(List:list, index);
	AMX_DEFINE_NATIVE(list_tagof, 2)
	{
		if(params[2] < 0) return 0;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native list_sizeof(List:list, index);
	AMX_DEFINE_NATIVE(list_sizeof, 2)
	{
		if(params[2] < 0) return 0;
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) return 0;
		if(static_cast<ucell>(params[2]) >= ptr->size()) return 0;
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_new),
	AMX_DECLARE_NATIVE(list_new_arr),
	AMX_DECLARE_NATIVE(list_new_args),
	AMX_DECLARE_NATIVE(list_new_args_str),
	AMX_DECLARE_NATIVE(list_new_args_var),
	AMX_DECLARE_NATIVE(list_valid),
	AMX_DECLARE_NATIVE(list_delete),
	AMX_DECLARE_NATIVE(list_delete_deep),
	AMX_DECLARE_NATIVE(list_clone),
	AMX_DECLARE_NATIVE(list_size),
	AMX_DECLARE_NATIVE(list_clear),
	AMX_DECLARE_NATIVE(list_add),
	AMX_DECLARE_NATIVE(list_add_arr),
	AMX_DECLARE_NATIVE(list_add_str),
	AMX_DECLARE_NATIVE(list_add_var),
	AMX_DECLARE_NATIVE(list_add_list),
	AMX_DECLARE_NATIVE(list_add_args),
	AMX_DECLARE_NATIVE(list_add_args_str),
	AMX_DECLARE_NATIVE(list_add_args_var),
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
