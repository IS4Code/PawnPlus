#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include "modules/expressions.h"

#include <vector>
#include <algorithm>

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
		if(index < -1) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(index == -1)
		{
			ptr->push_back(Factory(amx, params[Indices]...));
			return static_cast<cell>(ptr->size() - 1);
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "index");
			return 0;
		}else{
			ptr->insert(ptr->begin() + index, Factory(amx, params[Indices]...));
			return index;
		}
	}

	// native list_set(List:list, index, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		(*ptr)[params[2]] = Factory(amx, params[Indices]...);
		return 1;
	}

	// native list_get(List:list, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		return Factory(amx, (*ptr)[params[2]], params[Indices]...);
	}
	
	// native list_find(List:list, value, index=0, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_find(AMX *amx, cell *params)
	{
		cell index = optparam(3, 0);
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(index < 0)
		{
			index = ptr->size() + index;
		}
		if(index != ptr->size())
		{
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
			auto find = Factory(amx, params[Indices]...);
			for(size_t i = static_cast<size_t>(index); i < ptr->size(); i++)
			{
				if((*ptr)[i] == find)
				{
					return static_cast<cell>(i);
				}
			}
		}
		return -1;
	}

	// native list_find_last(List:list, value, index=-1, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_find_last(AMX *amx, cell *params)
	{
		cell index = optparam(3, -1);
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(index < 0)
		{
			index = ptr->size() + index;
		}
		if(index != -1)
		{
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
			auto find = Factory(amx, params[Indices]...);
			while(index >= 0)
			{
				if((*ptr)[index] == find)
				{
					return index;
				}
				index--;
			}
		}
		return -1;
	}

	// native list_resize(List:list, newsize, padding);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_resize(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "newsize");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		ucell newsize = params[2];
		if(ptr->size() >= newsize)
		{
			ptr->resize(newsize);
		}else{
			ptr->resize(newsize, Factory(amx, params[Indices]...));
		}
		return 1;
	}

	// native list_count(List:list, value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL list_count(AMX *amx, cell *params)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return std::count(ptr->begin(), ptr->end(), Factory(amx, params[Indices]...));
	}
};

// native list_set_cell(List:list, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
	if(params[3] < 0) amx_LogicError(errors::out_of_range, "offset");
	list_t *ptr;
	if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
	if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
	auto &obj = (*ptr)[params[2]];
	if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
	obj.set_cell({params[3]}, params[4]);
	return 1;
}

namespace Natives
{
	// native List:list_new();
	AMX_DEFINE_NATIVE_TAG(list_new, 0, list)
	{
		return list_pool.get_id(list_pool.add());
	}

	// native List:list_new_arr(AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
	AMX_DEFINE_NATIVE_TAG(list_new_arr, 3, list)
	{
		auto ptr = list_pool.add();
		cell *arr = amx_GetAddrSafe(amx, params[1]);

		for(cell i = 0; i < params[2]; i++)
		{
			ptr->push_back(dyn_object(amx, arr[i], params[3]));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args(tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(list_new_args, 0, list)
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
				addr = amx_GetAddrSafe(amx, param);
			}
			ptr->push_back(dyn_object(amx, *addr, params[1]));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args_str(arg0[], ...);
	AMX_DEFINE_NATIVE_TAG(list_new_args_str, 0, list)
	{
		auto ptr = list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell param = params[1 + arg];
			cell *addr = amx_GetAddrSafe(amx, param);
			ptr->push_back(dyn_object(addr));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args_var(VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(list_new_args_var, 0, list)
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
				addr = amx_GetAddrSafe(amx, param);
			}
			ptr->push_back(variants::get(*addr));
		}
		return list_pool.get_id(ptr);
	}

	// native List:list_new_args_packed(ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(list_new_args_packed, 0, list)
	{
		auto ptr = list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[1 + arg]);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return list_pool.get_id(ptr);
	}

	// native bool:list_valid(List:list);
	AMX_DEFINE_NATIVE_TAG(list_valid, 1, bool)
	{
		list_t *ptr;
		return list_pool.get_by_id(params[1], ptr);
	}

	// native list_delete(List:list);
	AMX_DEFINE_NATIVE_TAG(list_delete, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return list_pool.remove(ptr);
	}

	// native list_delete_deep(List:list);
	AMX_DEFINE_NATIVE_TAG(list_delete_deep, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		list_t old;
		ptr->swap(old);
		list_pool.remove(ptr);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native List:list_clone(List:list);
	AMX_DEFINE_NATIVE_TAG(list_clone, 1, list)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		auto l = list_pool.add();
		for(auto &&obj : *ptr)
		{
			l->push_back(obj.clone());
		}
		return list_pool.get_id(l);
	}

	// native list_size(List:list);
	AMX_DEFINE_NATIVE_TAG(list_size, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native list_capacity(List:list);
	AMX_DEFINE_NATIVE_TAG(list_capacity, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return static_cast<cell>(ptr->capacity());
	}

	// native list_clear(List:list);
	AMX_DEFINE_NATIVE_TAG(list_clear, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		list_t().swap(*ptr);
		return 1;
	}

	// native list_clear_deep(List:list);
	AMX_DEFINE_NATIVE_TAG(list_clear_deep, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		list_t old;
		ptr->swap(old);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native list_add(List:list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_add, 4, cell)
	{
		return value_at<2, 4>::list_add<dyn_func>(amx, params);
	}

	// native list_add_arr(List:list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_add_arr, 5, cell)
	{
		return value_at<2, 4, 5>::list_add<dyn_func_arr>(amx, params);
	}

	// native list_add_str(List:list, const value[], index=-1);
	AMX_DEFINE_NATIVE_TAG(list_add_str, 2, cell)
	{
		return value_at<2>::list_add<dyn_func_str>(amx, params);
	}

	// native list_add_var(List:list, VariantTag:value, index=-1);
	AMX_DEFINE_NATIVE_TAG(list_add_var, 2, cell)
	{
		return value_at<2>::list_add<dyn_func_var>(amx, params);
	}

	// native list_add_list(List:list, List:range, index=-1);
	AMX_DEFINE_NATIVE_TAG(list_add_list, 2, cell)
	{
		cell index = optparam(3, -1);
		if(index < -1) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		list_t *ptr2;
		if(!list_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "list", params[2]);
		if(index == -1)
		{
			size_t index = ptr->size();
			ptr->insert(ptr->end(), ptr2->begin(), ptr2->end());
			return static_cast<cell>(index);
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "index");
			return 0;
		}else{
			ptr->insert(ptr->begin() + index, ptr2->begin(), ptr2->end());
			return index;
		}
	}

	// native list_add_args(tag_id=tagof(arg0), List:list, AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(list_add_args, 2, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = (params[0] / sizeof(cell)) - 2;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[3 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				addr = amx_GetAddrSafe(amx, param);
			}
			ptr->push_back(dyn_object(amx, *addr, params[1]));
		}
		return numargs;
	}

	// native list_add_args_str(List:list, arg0[], ...);
	AMX_DEFINE_NATIVE_TAG(list_add_args_str, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell param = params[2 + arg];
			cell *addr = amx_GetAddrSafe(amx, param);
			ptr->push_back(dyn_object(addr));
		}
		return numargs;
	}

	// native list_add_args_var(List:list, VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(list_add_args_var, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[2 + arg];
			if(arg == 0)
			{
				addr = &param;
			}else{
				addr = amx_GetAddrSafe(amx, param);
			}
			ptr->push_back(variants::get(*addr));
		}
		return numargs;
	}

	// native list_add_args_packed(List:list, ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(list_add_args_packed, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return numargs;
	}

	// native list_remove(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_remove, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		ptr->erase(ptr->begin() + params[2]);
		return 1;
	}

	// native list_remove_deep(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_remove_deep, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");

		auto it = ptr->begin() + params[2];
		it->release();
		ptr->erase(it);
		return 1;
	}

	// native list_remove_range(List:list, begin, end);
	AMX_DEFINE_NATIVE_TAG(list_remove_range, 3, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "begin");
		if(params[3] < 0) amx_LogicError(errors::out_of_range, "end");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		ucell begin = params[2];
		ucell end = params[3];
		if(begin >= ptr->size()) amx_LogicError(errors::out_of_range, "begin");
		if(end >= ptr->size() || end < begin) amx_LogicError(errors::out_of_range, "end");
		ptr->erase(ptr->begin() + params[2], ptr->begin() + params[3]);
		return 1;
	}

	// native list_remove_range_deep(List:list, begin, end);
	AMX_DEFINE_NATIVE_TAG(list_remove_range_deep, 3, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		ucell begin = params[2];
		ucell end = params[3];
		if(begin >= ptr->size()) amx_LogicError(errors::out_of_range, "begin");
		if(end >= ptr->size() || end < begin) amx_LogicError(errors::out_of_range, "end");
		for(ucell i = begin; i <= end; i++)
		{
			(*ptr)[i].release();
		}
		ptr->erase(ptr->begin() + params[2], ptr->begin() + params[3]);
		return 1;
	}

	// native list_get(List:list, index, offset=0);
	AMX_DEFINE_NATIVE(list_get, 3)
	{
		return value_at<3>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr(List:list, index, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(list_get_arr, 4, cell)
	{
		return value_at<3, 4>::list_get<dyn_func_arr>(amx, params);
	}

	// native String:list_get_str_s(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_get_str_s, 2, string)
	{
		return value_at<>::list_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:list_get_var(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_get_var, 2, variant)
	{
		return value_at<>::list_get<dyn_func_var>(amx, params);
	}

	// native bool:list_get_safe(List:list, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_get_safe, 5, bool)
	{
		return value_at<3, 4, 5>::list_get<dyn_func>(amx, params);
	}

	// native list_get_arr_safe(List:list, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_get_arr_safe, 5, cell)
	{
		return value_at<3, 4, 5>::list_get<dyn_func_arr>(amx, params);
	}

	// native list_get_str_safe(List:list, index, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(list_get_str_safe, 4, cell)
	{
		return value_at<3, 4>::list_get<dyn_func_str>(amx, params);
	}

	// native String:list_get_str_safe_s(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_get_str_safe_s, 2, string)
	{
		return value_at<0>::list_get<dyn_func_str_s>(amx, params);
	}

	// native list_set(List:list, index, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_set, 4, cell)
	{
		return value_at<3, 4>::list_set<dyn_func>(amx, params);
	}

	// native list_set_arr(List:list, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_set_arr, 5, cell)
	{
		return value_at<3, 4, 5>::list_set<dyn_func_arr>(amx, params);
	}

	// native list_set_str(List:list, index, const value[]);
	AMX_DEFINE_NATIVE_TAG(list_set_str, 3, cell)
	{
		return value_at<3>::list_set<dyn_func_str>(amx, params);
	}

	// native list_set_var(List:list, index, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(list_set_var, 3, cell)
	{
		return value_at<3>::list_set<dyn_func_var>(amx, params);
	}

	// native list_set_cell(List:list, index, offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(list_set_cell, 3, cell)
	{
		return ::list_set_cell(amx, params);
	}

	// native bool:list_set_cell_safe(List:list, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_set_cell_safe, 5, cell)
	{
		return ::list_set_cell<5>(amx, params);
	}

	// native list_resize(List:list, newsize, AnyTag:padding, TagTag:tag_id=tagof(padding));
	AMX_DEFINE_NATIVE_TAG(list_resize, 4, cell)
	{
		return value_at<3, 4>::list_resize<dyn_func>(amx, params);
	}

	// native list_resize_arr(List:list, newsize, const AnyTag:padding[], size=sizeof(padding), TagTag:tag_id=tagof(padding));
	AMX_DEFINE_NATIVE_TAG(list_resize_arr, 5, cell)
	{
		return value_at<3, 4, 5>::list_resize<dyn_func_arr>(amx, params);
	}

	// native list_resize_str(List:list, newsize, const padding[]);
	AMX_DEFINE_NATIVE_TAG(list_resize_str, 3, cell)
	{
		return value_at<3>::list_resize<dyn_func_str>(amx, params);
	}

	// native list_resize_var(List:list, newsize, ConstVariantTag:padding);
	AMX_DEFINE_NATIVE_TAG(list_resize_var, 3, cell)
	{
		return value_at<3>::list_resize<dyn_func_var>(amx, params);
	}

	// native list_reserve(List:list, capacity);
	AMX_DEFINE_NATIVE_TAG(list_reserve, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "capacity");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		ucell size = params[2];
		ptr->reserve(size);
		return 1;
	}

	// native list_tagof(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_tagof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native list_sizeof(List:list, index);
	AMX_DEFINE_NATIVE_TAG(list_sizeof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
	}

	// native list_find(List:list, AnyTag:value, index=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_find, 4, cell)
	{
		return value_at<2, 4>::list_find<dyn_func>(amx, params);
	}

	// native list_find_arr(List:list, const AnyTag:value[], index=0, size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_find_arr, 5, cell)
	{
		return value_at<2, 4, 5>::list_find<dyn_func_arr>(amx, params);
	}

	// native list_find_str(List:list, const value[], index=0);
	AMX_DEFINE_NATIVE_TAG(list_find_str, 2, cell)
	{
		return value_at<2>::list_find<dyn_func_str>(amx, params);
	}

	// native list_find_var(List:list, VariantTag:value, index=0);
	AMX_DEFINE_NATIVE_TAG(list_find_var, 2, cell)
	{
		return value_at<2>::list_find<dyn_func_var>(amx, params);
	}

	// native list_find_last(List:list, AnyTag:value, index=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_find_last, 4, cell)
	{
		return value_at<2, 4>::list_find_last<dyn_func>(amx, params);
	}

	// native list_find_last_arr(List:list, const AnyTag:value[], index=0, size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_find_last_arr, 5, cell)
	{
		return value_at<2, 4, 5>::list_find_last<dyn_func_arr>(amx, params);
	}

	// native list_find_last_str(List:list, const value[], index=0);
	AMX_DEFINE_NATIVE_TAG(list_find_last_str, 2, cell)
	{
		return value_at<2>::list_find_last<dyn_func_str>(amx, params);
	}

	// native list_find_last_var(List:list, VariantTag:value, index=0);
	AMX_DEFINE_NATIVE_TAG(list_find_last_var, 2, cell)
	{
		return value_at<2>::list_find_last<dyn_func_var>(amx, params);
	}

	// native list_find_if(List:list, Expression:pred, index=0);
	AMX_DEFINE_NATIVE_TAG(list_find_if, 2, cell)
	{
		cell index = optparam(3, 0);
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		if(index < 0)
		{
			index = ptr->size() + index;
		}
		if(index != ptr->size())
		{
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
			dyn_object key;
			expression::args_type args;
			args.push_back(std::cref(key));
			args.push_back(std::cref(key));
			expression::exec_info info(amx);
			for(size_t i = static_cast<size_t>(index); i < ptr->size(); i++)
			{
				args[0] = std::cref((*ptr)[i]);
				key = dyn_object(i, tags::find_tag(tags::tag_cell));
				if(expr->execute_bool(args, info))
				{
					return static_cast<cell>(i);
				}
			}
		}
		return -1;
	}

	// native list_find_last_if(List:list, Expression:pred, index=-1);
	AMX_DEFINE_NATIVE_TAG(list_find_last_if, 2, cell)
	{
		cell index = optparam(3, -1);
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		if(index < 0)
		{
			index = ptr->size() + index;
		}
		if(index != -1)
		{
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
			dyn_object key;
			expression::args_type args;
			args.push_back(std::cref(key));
			args.push_back(std::cref(key));
			expression::exec_info info(amx);
			while(index >= 0)
			{
				args[0] = std::cref((*ptr)[index]);
				key = dyn_object(index, tags::find_tag(tags::tag_cell));
				if(expr->execute_bool(args, info))
				{
					return index;
				}
				index--;
			}
		}
		return -1;
	}

	// native list_remove_if(List:list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(list_remove_if, 2, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);
		
		cell count = 0;
		ptr->erase(std::remove_if(ptr->begin(), ptr->end(), [&](const dyn_object &obj)
		{
			args[0] = std::cref(obj);
			key = dyn_object(&obj - &*ptr->cbegin(), tags::find_tag(tags::tag_cell));
			if(expr->execute_bool(args, info))
			{
				count++;
				return true;
			}
			return false;
		}), ptr->end());
		return count;
	}

	// native list_remove_if_deep(List:list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(list_remove_if_deep, 2, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);
		cell count = 0;
		ptr->erase(std::remove_if(ptr->begin(), ptr->end(), [&](const dyn_object &obj)
		{
			args[0] = std::cref(obj);
			key = dyn_object(&obj - &*ptr->cbegin(), tags::find_tag(tags::tag_cell));
			if(expr->execute_bool(args, info))
			{
				obj.release();
				count++;
				return true;
			}
			return false;
		}), ptr->end());
		return count;
	}

	// native list_count(List:list, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_count, 3, cell)
	{
		return value_at<2, 3>::list_count<dyn_func>(amx, params);
	}

	// native list_count_arr(List:list, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(list_count_arr, 4, cell)
	{
		return value_at<2, 3, 4>::list_count<dyn_func_arr>(amx, params);
	}

	// native list_count_str(List:list, const value[]);
	AMX_DEFINE_NATIVE_TAG(list_count_str, 2, cell)
	{
		return value_at<2>::list_count<dyn_func_str>(amx, params);
	}

	// native list_count_var(List:list, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(list_count_var, 2, cell)
	{
		return value_at<2>::list_count<dyn_func_var>(amx, params);
	}

	// native list_count_if(List:list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(list_count_if, 2, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);

		return std::count_if(ptr->begin(), ptr->end(), [&](const dyn_object &obj)
		{
			args[0] = std::cref(obj);
			key = dyn_object(&obj - &*ptr->cbegin(), tags::find_tag(tags::tag_cell));
			return expr->execute_bool(args, info);
		});
	}

	struct cell_sorter
	{
		cell offset, size;

		cell_sorter(cell offset, cell size) : offset(offset), size(size)
		{

		}

		bool operator()(const dyn_object &a, const dyn_object &b)
		{
			cell at = a.get_tag()->find_top_base()->uid;
			cell bt = b.get_tag()->find_top_base()->uid;
			if(at < bt) return true;
			if(at > bt) return false;
			auto tag = a.get_tag();
			const auto &ops = tag->get_ops();
			const cell *addr1, *addr2;
			for(cell i = 0; size == -1 || i < size; i++)
			{
				cell index = offset + i;
				if(!(addr2 = b.get_cell_addr(&index, 1)))
				{
					return false;
				}else if(!(addr1 = a.get_cell_addr(&index, 1)))
				{
					return true;
				}else if(ops.lt(tag, *addr1, *addr2))
				{
					return true;
				}else if(i == size - 1 || ops.lt(tag, *addr2, *addr1))
				{
					return false;
				}
			}
			return false;
		}
	};

	// native list_sort(List:list, offset=0, size=1, bool:reverse=false, bool:stable=true);
	AMX_DEFINE_NATIVE_TAG(list_sort, 1, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr))
		{
			amx_LogicError(errors::pointer_invalid, "list", params[1]);
		}

		cell offset = optparam(2, 0);

		if(offset < 0)
		{
			amx_LogicError(errors::out_of_range, "offset");
		}

		cell size = optparam(3, -1);
		if(size < -1)
		{
			amx_LogicError(errors::out_of_range, "size");
		}

		bool reverse = optparam(4, 0);
		bool stable = optparam(5, 1);

		bool simple = offset == 0 && size == -1;

		if(!reverse)
		{
			auto begin = ptr->begin(), end = ptr->end();
			if(stable)
			{
				if(simple)
				{
					std::stable_sort(begin, end);
				}else{
					std::stable_sort(begin, end, cell_sorter(offset, size));
				}
			}else{
				if(simple)
				{
					std::sort(begin, end);
				}else{
					std::sort(begin, end, cell_sorter(offset, size));
				}
			}
		}else{
			auto begin = ptr->rbegin(), end = ptr->rend();
			if(stable)
			{
				if(simple)
				{
					std::stable_sort(begin, end);
				}else{
					std::stable_sort(begin, end, cell_sorter(offset, size));
				}
			}else{
				if(simple)
				{
					std::sort(begin, end);
				}else{
					std::sort(begin, end, cell_sorter(offset, size));
				}
			}
		}
		return 1;
	}

	struct expr_sorter
	{
		const expression *expr;
		expression::exec_info &info;
		mutable expression::args_type args;

		expr_sorter(const expression *expr, expression::exec_info &info) : expr(expr), info(info)
		{

		}

		bool operator()(const dyn_object &a, const dyn_object &b)
		{
			if(args.size() == 0)
			{
				args.push_back(std::cref(a));
				args.push_back(std::cref(b));
			}else{
				args[0] = std::cref(a);
				args[1] = std::cref(b);
			}
			return expr->execute_bool(args, info);
		}
	};

	// native list_sort_expr(List:list, Expression:expr, bool:reverse=false, bool:stable=true);
	AMX_DEFINE_NATIVE_TAG(list_sort_expr, 2, cell)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);

		bool reverse = optparam(3, 0);
		bool stable = optparam(4, 1);

		expression::exec_info info(amx);
		expr_sorter sorter(expr, info);
		if(!reverse)
		{
			auto begin = ptr->begin(), end = ptr->end();
			if(stable)
			{
				std::stable_sort(begin, end, sorter);
			}else{
				std::sort(begin, end, sorter);
			}
		}else{
			auto begin = ptr->rbegin(), end = ptr->rend();
			if(stable)
			{
				std::stable_sort(begin, end, sorter);
			}else{
				std::sort(begin, end, sorter);
			}
		}
		return 1;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_new),
	AMX_DECLARE_NATIVE(list_new_arr),
	AMX_DECLARE_NATIVE(list_new_args),
	AMX_DECLARE_NATIVE(list_new_args_str),
	AMX_DECLARE_NATIVE(list_new_args_var),
	AMX_DECLARE_NATIVE(list_new_args_packed),

	AMX_DECLARE_NATIVE(list_valid),
	AMX_DECLARE_NATIVE(list_delete),
	AMX_DECLARE_NATIVE(list_delete_deep),
	AMX_DECLARE_NATIVE(list_clone),
	AMX_DECLARE_NATIVE(list_size),
	AMX_DECLARE_NATIVE(list_capacity),
	AMX_DECLARE_NATIVE(list_reserve),
	AMX_DECLARE_NATIVE(list_clear),
	AMX_DECLARE_NATIVE(list_clear_deep),

	AMX_DECLARE_NATIVE(list_add),
	AMX_DECLARE_NATIVE(list_add_arr),
	AMX_DECLARE_NATIVE(list_add_str),
	AMX_DECLARE_NATIVE(list_add_var),
	AMX_DECLARE_NATIVE(list_add_list),
	AMX_DECLARE_NATIVE(list_add_args),
	AMX_DECLARE_NATIVE(list_add_args_str),
	AMX_DECLARE_NATIVE(list_add_args_var),
	AMX_DECLARE_NATIVE(list_add_args_packed),

	AMX_DECLARE_NATIVE(list_remove),
	AMX_DECLARE_NATIVE(list_remove_deep),
	AMX_DECLARE_NATIVE(list_remove_range),
	AMX_DECLARE_NATIVE(list_remove_range_deep),
	AMX_DECLARE_NATIVE(list_remove_if),
	AMX_DECLARE_NATIVE(list_remove_if_deep),

	AMX_DECLARE_NATIVE(list_get),
	AMX_DECLARE_NATIVE(list_get_arr),
	AMX_DECLARE_NATIVE(list_get_str_s),
	AMX_DECLARE_NATIVE(list_get_var),
	AMX_DECLARE_NATIVE(list_get_safe),
	AMX_DECLARE_NATIVE(list_get_arr_safe),
	AMX_DECLARE_NATIVE(list_get_str_safe),
	AMX_DECLARE_NATIVE(list_get_str_safe_s),

	AMX_DECLARE_NATIVE(list_set),
	AMX_DECLARE_NATIVE(list_set_arr),
	AMX_DECLARE_NATIVE(list_set_str),
	AMX_DECLARE_NATIVE(list_set_var),
	AMX_DECLARE_NATIVE(list_set_cell),
	AMX_DECLARE_NATIVE(list_set_cell_safe),

	AMX_DECLARE_NATIVE(list_resize),
	AMX_DECLARE_NATIVE(list_resize_arr),
	AMX_DECLARE_NATIVE(list_resize_str),
	AMX_DECLARE_NATIVE(list_resize_var),

	AMX_DECLARE_NATIVE(list_find),
	AMX_DECLARE_NATIVE(list_find_arr),
	AMX_DECLARE_NATIVE(list_find_str),
	AMX_DECLARE_NATIVE(list_find_var),
	AMX_DECLARE_NATIVE(list_find_last),
	AMX_DECLARE_NATIVE(list_find_last_arr),
	AMX_DECLARE_NATIVE(list_find_last_str),
	AMX_DECLARE_NATIVE(list_find_last_var),
	AMX_DECLARE_NATIVE(list_find_if),
	AMX_DECLARE_NATIVE(list_find_last_if),

	AMX_DECLARE_NATIVE(list_count),
	AMX_DECLARE_NATIVE(list_count_arr),
	AMX_DECLARE_NATIVE(list_count_str),
	AMX_DECLARE_NATIVE(list_count_var),
	AMX_DECLARE_NATIVE(list_count_if),

	AMX_DECLARE_NATIVE(list_sort),
	AMX_DECLARE_NATIVE(list_sort_expr),

	AMX_DECLARE_NATIVE(list_tagof),
	AMX_DECLARE_NATIVE(list_sizeof),
};

int RegisterListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
