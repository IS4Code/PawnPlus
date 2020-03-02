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
	// native linked_list_add(LinkedList:linked_list, value, index=-1, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL linked_list_add(AMX *amx, cell *params)
	{
		cell index = optparam(3, -1);
		if(index < -1) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(index == -1)
		{
			ptr->push_back(Factory(amx, params[Indices]...));
			return static_cast<cell>(ptr->size() - 1);
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "index");
			return 0;
		}else{
			auto it = ptr->begin();
			std::advance(it, index);
			ptr->insert(it, Factory(amx, params[Indices]...));
			return index;
		}
	}

	// native linked_list_set(LinkedList:linked_list, index, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL linked_list_set(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		(*ptr)[params[2]] = Factory(amx, params[Indices]...);
		return 1;
	}

	// native linked_list_get(LinkedList:linked_list, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL linked_list_get(AMX *amx, cell *params)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		return Factory(amx, (*ptr)[params[2]], params[Indices]...);
	}

	// native linked_list_count(LinkedList:linked_list, value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL linked_list_count(AMX *amx, cell *params)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		auto find = Factory(amx, params[Indices]...);
		return std::count_if(ptr->begin(), ptr->end(), [&](const std::shared_ptr<dyn_object> &obj)
		{
			return *obj == find;
		});
	}
};

// native linked_list_set_cell(LinkedList:linked_list, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL linked_list_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
	if(params[3] < 0) amx_LogicError(errors::out_of_range, "offset");
	linked_list_t *ptr;
	if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
	if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
	auto &obj = (*ptr)[params[2]];
	if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
	obj.set_cell({params[3]}, params[4]);
	return 1;
}

namespace Natives
{
	// native LinkedList:linked_list_new();
	AMX_DEFINE_NATIVE_TAG(linked_list_new, 0, linked_list)
	{
		return linked_list_pool.get_id(linked_list_pool.add());
	}

	// native LinkedList:linked_list_new_arr(AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
	AMX_DEFINE_NATIVE_TAG(linked_list_new_arr, 3, linked_list)
	{
		auto ptr = linked_list_pool.add();
		cell *arr = amx_GetAddrSafe(amx, params[1]);

		for(cell i = 0; i < params[2]; i++)
		{
			ptr->push_back(dyn_object(amx, arr[i], params[3]));
		}
		return linked_list_pool.get_id(ptr);
	}

	// native LinkedList:linked_list_new_args(tag_id=tagof(arg0), AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_new_args, 0, linked_list)
	{
		auto ptr = linked_list_pool.add();
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
		return linked_list_pool.get_id(ptr);
	}

	// native LinkedList:linked_list_new_args_str(arg0[], ...);
	AMX_DEFINE_NATIVE_TAG(linked_list_new_args_str, 0, linked_list)
	{
		auto ptr = linked_list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell param = params[1 + arg];
			cell *addr = amx_GetAddrSafe(amx, param);
			ptr->push_back(dyn_object(addr));
		}
		return linked_list_pool.get_id(ptr);
	}

	// native LinkedList:linked_list_new_args_var(VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_new_args_var, 0, linked_list)
	{
		auto ptr = linked_list_pool.add();
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
		return linked_list_pool.get_id(ptr);
	}

	// native LinkedList:linked_list_new_args_packed(ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_new_args_packed, 0, linked_list)
	{
		auto ptr = linked_list_pool.add();
		cell numargs = params[0] / sizeof(cell);
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[1 + arg]);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return linked_list_pool.get_id(ptr);
	}

	// native bool:linked_list_valid(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_valid, 1, bool)
	{
		linked_list_t *ptr;
		return linked_list_pool.get_by_id(params[1], ptr);
	}

	// native linked_list_delete(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_delete, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		return linked_list_pool.remove(ptr);
	}

	// native linked_list_delete_deep(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_delete_deep, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		linked_list_t old;
		ptr->swap(old);
		linked_list_pool.remove(ptr);
		for(auto &obj : old)
		{
			obj->release();
		}
		return 1;
	}

	// native LinkedList:linked_list_clone(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_clone, 1, linked_list)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		auto l = linked_list_pool.add();
		for(auto &&obj : *ptr)
		{
			l->push_back(obj->clone());
		}
		return linked_list_pool.get_id(l);
	}

	// native linked_list_size(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_size, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native linked_list_clear(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_clear, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		linked_list_t().swap(*ptr);
		return 1;
	}

	// native linked_list_clear_deep(LinkedList:linked_list);
	AMX_DEFINE_NATIVE_TAG(linked_list_clear_deep, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		linked_list_t old;
		ptr->swap(old);
		for(auto &obj : old)
		{
			obj->release();
		}
		return 1;
	}

	// native linked_list_add(LinkedList:linked_list, AnyTag:value, index=-1, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_add, 4, cell)
	{
		return value_at<2, 4>::linked_list_add<dyn_func>(amx, params);
	}

	// native linked_list_add_arr(LinkedList:linked_list, const AnyTag:value[], index=-1, size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_add_arr, 5, cell)
	{
		return value_at<2, 4, 5>::linked_list_add<dyn_func_arr>(amx, params);
	}

	// native linked_list_add_str(LinkedList:linked_list, const value[], index=-1);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_str, 2, cell)
	{
		return value_at<2>::linked_list_add<dyn_func_str>(amx, params);
	}

	// native linked_list_add_var(LinkedList:linked_list, VariantTag:value, index=-1);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_var, 2, cell)
	{
		return value_at<2>::linked_list_add<dyn_func_var>(amx, params);
	}

	// native linked_list_add_linked_list(LinkedList:linked_list, LinkedList:range, index=-1);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_linked_list, 2, cell)
	{
		if(params[3] < -1) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		linked_list_t *ptr2;
		if(!linked_list_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "linked list", params[2]);
		if(params[3] == -1)
		{
			size_t index = ptr->size();
			ptr->insert(ptr->end(), ptr2->begin(), ptr2->end());
			return static_cast<cell>(index);
		}else if(static_cast<ucell>(params[3]) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "index");
			return 0;
		}else{
			auto it = ptr->begin();
			std::advance(it, params[3]);
			ptr->insert(it, ptr2->begin(), ptr2->end());
			return params[3];
		}
	}

	// native linked_list_add_args(tag_id=tagof(arg0), LinkedList:linked_list, AnyTag:arg0, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_args, 2, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
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

	// native linked_list_add_args_str(LinkedList:linked_list, arg0[], ...);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_args_str, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell param = params[2 + arg];
			cell *addr = amx_GetAddrSafe(amx, param);
			ptr->push_back(dyn_object(addr));
		}
		return numargs;
	}

	// native linked_list_add_args_var(LinkedList:linked_list, VariantTag:arg0, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_args_var, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
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

	// native linked_list_add_args_packed(LinkedList:linked_list, ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(linked_list_add_args_packed, 1, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		for(cell arg = 0; arg < numargs; arg++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->push_back(dyn_object(amx, addr[0], addr[1]));
		}
		return numargs;
	}

	// native linked_list_remove(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_remove, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto it = ptr->begin();
		std::advance(it, params[2]);
		ptr->erase(it);
		return 1;
	}

	// native linked_list_remove_deep(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_remove_deep, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto it = ptr->begin();
		std::advance(it, params[2]);
		(*it)->release();
		ptr->erase(it);
		return 1;
	}

	// native linked_list_get(LinkedList:linked_list, index, offset=0);
	AMX_DEFINE_NATIVE(linked_list_get, 3)
	{
		return value_at<3>::linked_list_get<dyn_func>(amx, params);
	}

	// native linked_list_get_arr(LinkedList:linked_list, index, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_get_arr, 4, cell)
	{
		return value_at<3, 4>::linked_list_get<dyn_func_arr>(amx, params);
	}

	// native String:linked_list_get_str_s(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_get_str_s, 2, string)
	{
		return value_at<>::linked_list_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:linked_list_get_var(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_get_var, 2, variant)
	{
		return value_at<>::linked_list_get<dyn_func_var>(amx, params);
	}

	// native bool:linked_list_get_safe(LinkedList:linked_list, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_get_safe, 5, bool)
	{
		return value_at<3, 4, 5>::linked_list_get<dyn_func>(amx, params);
	}

	// native linked_list_get_arr_safe(LinkedList:linked_list, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_get_arr_safe, 5, cell)
	{
		return value_at<3, 4, 5>::linked_list_get<dyn_func_arr>(amx, params);
	}

	// native linked_list_get_str_safe(LinkedList:linked_list, index, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_get_str_safe, 4, cell)
	{
		return value_at<3, 4>::linked_list_get<dyn_func_str>(amx, params);
	}

	// native String:linked_list_get_str_safe_s(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_get_str_safe_s, 2, string)
	{
		return value_at<0>::linked_list_get<dyn_func_str_s>(amx, params);
	}

	// native linked_list_set(LinkedList:linked_list, index, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_set, 4, cell)
	{
		return value_at<3, 4>::linked_list_set<dyn_func>(amx, params);
	}

	// native linked_list_set_arr(LinkedList:linked_list, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_set_arr, 5, cell)
	{
		return value_at<3, 4, 5>::linked_list_set<dyn_func_arr>(amx, params);
	}

	// native linked_list_set_str(LinkedList:linked_list, index, const value[]);
	AMX_DEFINE_NATIVE_TAG(linked_list_set_str, 3, cell)
	{
		return value_at<3>::linked_list_set<dyn_func_str>(amx, params);
	}

	// native linked_list_set_var(LinkedList:linked_list, index, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(linked_list_set_var, 3, cell)
	{
		return value_at<3>::linked_list_set<dyn_func_var>(amx, params);
	}

	// native linked_list_set_cell(LinkedList:linked_list, index, offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(linked_list_set_cell, 4, cell)
	{
		return ::linked_list_set_cell(amx, params);
	}

	// native bool:linked_list_set_cell_safe(LinkedList:linked_list, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_set_cell_safe, 5, cell)
	{
		return ::linked_list_set_cell<5>(amx, params);
	}

	// native linked_list_tagof(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_tagof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_tag(amx);
	}

	// native linked_list_sizeof(LinkedList:linked_list, index);
	AMX_DEFINE_NATIVE_TAG(linked_list_sizeof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto &obj = (*ptr)[params[2]];
		return obj.get_size();
	}

	// native linked_list_remove_if(LinkedList:linked_list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(linked_list_remove_if, 2, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);

		expression::args_type args;
		expression::exec_info info(amx);

		cell count = 0;
		auto it = ptr->begin();
		while(it != ptr->end())
		{
			if(args.size() == 0)
			{
				args.push_back(std::cref(**it));
			}else{
				args[0] = std::cref(**it);
			}
			if(expr->execute_bool(args, info))
			{
				it = ptr->erase(it);
				count++;
			}else{
				++it;
			}
		}
		return count;
	}

	// native linked_list_remove_if_deep(LinkedList:linked_list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(linked_list_remove_if_deep, 2, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		
		expression::args_type args;
		expression::exec_info info(amx);

		cell count = 0;
		auto it = ptr->begin();
		while(it != ptr->end())
		{
			if(args.size() == 0)
			{
				args.push_back(std::cref(**it));
			}else{
				args[0] = std::cref(**it);
			}
			if(expr->execute_bool(args, info))
			{
				(*it)->release();
				it = ptr->erase(it);
				count++;
			}else{
				++it;
			}
		}
		return count;
	}

	// native linked_list_count(LinkedList:linked_list, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_count, 3, cell)
	{
		return value_at<2, 3>::linked_list_count<dyn_func>(amx, params);
	}

	// native linked_list_count_arr(LinkedList:linked_list, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(linked_list_count_arr, 4, cell)
	{
		return value_at<2, 3, 4>::linked_list_count<dyn_func_arr>(amx, params);
	}

	// native linked_list_count_str(LinkedList:linked_list, const value[]);
	AMX_DEFINE_NATIVE_TAG(linked_list_count_str, 2, cell)
	{
		return value_at<2>::linked_list_count<dyn_func_str>(amx, params);
	}

	// native linked_list_count_var(LinkedList:linked_list, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(linked_list_count_var, 2, cell)
	{
		return value_at<2>::linked_list_count<dyn_func_var>(amx, params);
	}

	// native linked_list_count_if(LinkedList:linked_list, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(linked_list_count_if, 2, cell)
	{
		linked_list_t *ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		
		expression::args_type args;
		expression::exec_info info(amx);

		return std::count_if(ptr->begin(), ptr->end(), [&](const std::shared_ptr<dyn_object> &obj)
		{
			if(args.size() == 0)
			{
				args.push_back(std::cref(*obj));
			}else{
				args[0] = std::cref(*obj);
			}
			return expr->execute_bool(args, info);
		});
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(linked_list_new),
	AMX_DECLARE_NATIVE(linked_list_new_arr),
	AMX_DECLARE_NATIVE(linked_list_new_args),
	AMX_DECLARE_NATIVE(linked_list_new_args_str),
	AMX_DECLARE_NATIVE(linked_list_new_args_var),
	AMX_DECLARE_NATIVE(linked_list_new_args_packed),

	AMX_DECLARE_NATIVE(linked_list_valid),
	AMX_DECLARE_NATIVE(linked_list_delete),
	AMX_DECLARE_NATIVE(linked_list_delete_deep),
	AMX_DECLARE_NATIVE(linked_list_clone),
	AMX_DECLARE_NATIVE(linked_list_size),
	AMX_DECLARE_NATIVE(linked_list_clear),
	AMX_DECLARE_NATIVE(linked_list_clear_deep),

	AMX_DECLARE_NATIVE(linked_list_add),
	AMX_DECLARE_NATIVE(linked_list_add_arr),
	AMX_DECLARE_NATIVE(linked_list_add_str),
	AMX_DECLARE_NATIVE(linked_list_add_var),
	AMX_DECLARE_NATIVE(linked_list_add_linked_list),
	AMX_DECLARE_NATIVE(linked_list_add_args),
	AMX_DECLARE_NATIVE(linked_list_add_args_str),
	AMX_DECLARE_NATIVE(linked_list_add_args_var),
	AMX_DECLARE_NATIVE(linked_list_add_args_packed),

	AMX_DECLARE_NATIVE(linked_list_remove),
	AMX_DECLARE_NATIVE(linked_list_remove_deep),
	AMX_DECLARE_NATIVE(linked_list_remove_if),
	AMX_DECLARE_NATIVE(linked_list_remove_if_deep),

	AMX_DECLARE_NATIVE(linked_list_get),
	AMX_DECLARE_NATIVE(linked_list_get_arr),
	AMX_DECLARE_NATIVE(linked_list_get_str_s),
	AMX_DECLARE_NATIVE(linked_list_get_var),
	AMX_DECLARE_NATIVE(linked_list_get_safe),
	AMX_DECLARE_NATIVE(linked_list_get_arr_safe),
	AMX_DECLARE_NATIVE(linked_list_get_str_safe),
	AMX_DECLARE_NATIVE(linked_list_get_str_safe_s),

	AMX_DECLARE_NATIVE(linked_list_set),
	AMX_DECLARE_NATIVE(linked_list_set_arr),
	AMX_DECLARE_NATIVE(linked_list_set_str),
	AMX_DECLARE_NATIVE(linked_list_set_var),
	AMX_DECLARE_NATIVE(linked_list_set_cell),
	AMX_DECLARE_NATIVE(linked_list_set_cell_safe),

	AMX_DECLARE_NATIVE(linked_list_count),
	AMX_DECLARE_NATIVE(linked_list_count_arr),
	AMX_DECLARE_NATIVE(linked_list_count_str),
	AMX_DECLARE_NATIVE(linked_list_count_var),
	AMX_DECLARE_NATIVE(linked_list_count_if),

	AMX_DECLARE_NATIVE(linked_list_tagof),
	AMX_DECLARE_NATIVE(linked_list_sizeof),
};

int RegisterLinkedListNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
