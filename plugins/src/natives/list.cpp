#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"

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
		if(index < -1) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(index == -1)
		{
			ptr->push_back(Factory(amx, params[Indices]...));
			return static_cast<cell>(ptr->size() - 1);
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "list index");
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
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
		(*ptr)[params[2]] = Factory(amx, params[Indices]...);
		return 1;
	}

	// native list_get(List:list, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
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
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
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
		cell index = optparam(3, 0);
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(index < 0)
		{
			index = ptr->size() + index;
		}
		if(index != -1)
		{
			if(index < 0 || static_cast<ucell>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
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
};

// native bool:list_set_cell(List:list, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL list_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
	if(params[3] < 0) amx_LogicError(errors::out_of_range, "array offset");
	list_t *ptr;
	if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
	if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
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
			ptr->push_back(dyn_object(addr));
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

	// native List:list_new_args_packed(ArgTag:...);
	AMX_DEFINE_NATIVE(list_new_args_packed, 0)
	{
		auto ptr = list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr;
			amx_GetAddr(amx, params[1 + arg], &addr);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return list_pool.get_id(ptr);
	}

	// native bool:list_valid(List:list);
	AMX_DEFINE_NATIVE(list_valid, 1)
	{
		list_t *ptr;
		return list_pool.get_by_id(params[1], ptr);
	}

	// native list_delete(List:list);
	AMX_DEFINE_NATIVE(list_delete, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return list_pool.remove(ptr);
	}

	// native list_delete_deep(List:list);
	AMX_DEFINE_NATIVE(list_delete_deep, 1)
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
	AMX_DEFINE_NATIVE(list_clone, 1)
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
	AMX_DEFINE_NATIVE(list_size, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native list_clear(List:list);
	AMX_DEFINE_NATIVE(list_clear, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		list_t().swap(*ptr);
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
		cell index = optparam(3, -1);
		if(index < -1) amx_LogicError(errors::out_of_range, "list index");
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
			amx_LogicError(errors::out_of_range, "list index");
			return 0;
		}else{
			ptr->insert(ptr->begin() + index, ptr2->begin(), ptr2->end());
			return index;
		}
	}

	// native list_add_args(tag_id=tagof(arg0), List:list, AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE(list_add_args, 2)
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
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr, param = params[2 + arg];
			amx_GetAddr(amx, param, &addr);
			ptr->push_back(dyn_object(addr));
		}
		return numargs;
	}

	// native list_add_args_var(List:list, VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE(list_add_args_var, 1)
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
				amx_GetAddr(amx, param, &addr);
			}
			ptr->push_back(variants::get(*addr));
		}
		return numargs;
	}

	// native list_add_args_packed(List:list, ArgTag:...);
	AMX_DEFINE_NATIVE(list_add_args_packed, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr;
			amx_GetAddr(amx, params[2 + arg], &addr);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return numargs;
	}

	// native bool:list_remove(List:list, index);
	AMX_DEFINE_NATIVE(list_remove, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
		ptr->erase(ptr->begin() + params[2]);
		return 1;
	}

	// native bool:list_remove_deep(List:list, index);
	AMX_DEFINE_NATIVE(list_remove_deep, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");

		auto it = ptr->begin() + params[2];
		it->release();
		ptr->erase(it);
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

	// native list_resize(List:list, newsize, AnyTag:padding, TagTag:tag_id=tagof(padding));
	AMX_DEFINE_NATIVE(list_resize, 4)
	{
		return value_at<3, 4>::list_resize<dyn_func>(amx, params);
	}

	// native list_resize_arr(List:list, newsize, const AnyTag:padding[], size=sizeof(padding), TagTag:tag_id=tagof(padding));
	AMX_DEFINE_NATIVE(list_resize_arr, 5)
	{
		return value_at<3, 4, 5>::list_resize<dyn_func_arr>(amx, params);
	}

	// native list_resize_str(List:list, newsize, const padding[]);
	AMX_DEFINE_NATIVE(list_resize_str, 3)
	{
		return value_at<3>::list_resize<dyn_func_str>(amx, params);
	}

	// native list_resize_var(List:list, newsize, ConstVariantTag:padding);
	AMX_DEFINE_NATIVE(list_resize_var, 3)
	{
		return value_at<3>::list_resize<dyn_func_var>(amx, params);
	}

	// native list_tagof(List:list, index);
	AMX_DEFINE_NATIVE(list_tagof, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native list_sizeof(List:list, index);
	AMX_DEFINE_NATIVE(list_sizeof, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "list index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
	}

	// native list_find(List:list, AnyTag:value, index=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_find, 4)
	{
		return value_at<2, 4>::list_find<dyn_func>(amx, params);
	}

	// native list_find_arr(List:list, const AnyTag:value[], index=0, size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_find_arr, 5)
	{
		return value_at<2, 4, 5>::list_find<dyn_func_arr>(amx, params);
	}

	// native list_find_str(List:list, const value[], index=0);
	AMX_DEFINE_NATIVE(list_find_str, 2)
	{
		return value_at<2>::list_find<dyn_func_str>(amx, params);
	}

	// native list_find_var(List:list, VariantTag:value, index=0);
	AMX_DEFINE_NATIVE(list_find_var, 2)
	{
		return value_at<2>::list_find<dyn_func_var>(amx, params);
	}

	// native list_find_last(List:list, AnyTag:value, index=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_find_last, 4)
	{
		return value_at<2, 4>::list_find_last<dyn_func>(amx, params);
	}

	// native list_find_last_arr(List:list, const AnyTag:value[], index=0, size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(list_find_last_arr, 5)
	{
		return value_at<2, 4, 5>::list_find_last<dyn_func_arr>(amx, params);
	}

	// native list_find_last_str(List:list, const value[], index=0);
	AMX_DEFINE_NATIVE(list_find_last_str, 2)
	{
		return value_at<2>::list_find_last<dyn_func_str>(amx, params);
	}

	// native list_find_last_var(List:list, VariantTag:value, index=0);
	AMX_DEFINE_NATIVE(list_find_last_var, 2)
	{
		return value_at<2>::list_find_last<dyn_func_var>(amx, params);
	}

	struct cell_sorter
	{
		cell offset;

		cell_sorter(cell offset) : offset(offset)
		{

		}

		bool operator()(const dyn_object &a, const dyn_object &b)
		{
			cell ac, bc;
			if(!b.get_cell(offset, bc))
			{
				return false;
			}
			if(!a.get_cell(offset, ac))
			{
				return true;
			}
			cell at = a.get_tag()->find_top_base()->uid;
			cell bt = b.get_tag()->find_top_base()->uid;
			if(at < bt) return true;
			if(at > bt) return false;
			auto tag = a.get_tag();
			const auto &ops = tag->get_ops();
			return ops.lt(tag, ac, bc);
		}
	};

	// native list_sort(List:list, offset=-1, bool:reverse=false, bool:stable=true);
	AMX_DEFINE_NATIVE(list_sort, 1)
	{
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr))
		{
			amx_LogicError(errors::pointer_invalid, "list", params[1]);
		}

		cell offset = optparam(2, -1);

		if(offset < -1)
		{
			amx_LogicError(errors::out_of_range, "offset");
		}

		bool reverse = optparam(3, 0);
		bool stable = optparam(4, 1);

		if(!reverse)
		{
			auto begin = ptr->begin(), end = ptr->end();
			if(stable)
			{
				if(offset == -1)
				{
					std::stable_sort(begin, end);
				}else{
					std::stable_sort(begin, end, cell_sorter(offset));
				}
			}else{
				if(offset == -1)
				{
					std::sort(begin, end);
				}else{
					std::sort(begin, end, cell_sorter(offset));
				}
			}
		}else{
			auto begin = ptr->rbegin(), end = ptr->rend();
			if(stable)
			{
				if(offset == -1)
				{
					std::stable_sort(begin, end);
				}else{
					std::stable_sort(begin, end, cell_sorter(offset));
				}
			}else{
				if(offset == -1)
				{
					std::sort(begin, end);
				}else{
					std::sort(begin, end, cell_sorter(offset));
				}
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
	AMX_DECLARE_NATIVE(list_clear),

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

	AMX_DECLARE_NATIVE(list_sort),

	AMX_DECLARE_NATIVE(list_tagof),
	AMX_DECLARE_NATIVE(list_sizeof),
};

int RegisterListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
