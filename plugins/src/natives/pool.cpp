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
	// native pool_add(Pool:pool, value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL pool_add(AMX *amx, cell *params)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return static_cast<cell>(ptr->push_back(Factory(amx, params[Indices]...)));
	}

	// native pool_set(Pool:pool, index, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL pool_set(AMX *amx, cell *params)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ptr->insert_or_set(index, Factory(amx, params[Indices]...));
		return 1;
	}

	// native pool_get(Pool:pool, index, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL pool_get(AMX *amx, cell *params)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(index);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		return Factory(amx, *it, params[Indices]...);
	}

	// native pool_find(Pool:pool, value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL pool_find(AMX *amx, cell *params)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto find = Factory(amx, params[Indices]...);
		for(auto it = ptr->begin(); it != ptr->end(); ++it)
		{
			if(*it == find)
			{
				return ptr->index_of(it);
			}
		}
		return -1;
	}

	// native pool_count(Pool:pool, value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL pool_count(AMX *amx, cell *params)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return std::count(ptr->begin(), ptr->end(), Factory(amx, params[Indices]...));
	}
};

// native pool_set_cell(Pool:pool, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL pool_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
	if(params[3] < 0) amx_LogicError(errors::out_of_range, "offset");
	pool_t *ptr;
	if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
	auto it = ptr->find(params[2]);
	if(it == ptr->end()) amx_LogicError(errors::element_not_present);
	auto &obj = *it;
	if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
	obj.set_cell({params[3]}, params[4]);
	return 1;
}

namespace Natives
{
	// native Pool:pool_new(capacity=-1, bool:ordered=false);
	AMX_DEFINE_NATIVE_TAG(pool_new, 0, pool)
	{
		cell size = optparam(1, -1);
		if(size == -1)
		{
			size = 1;
		}else if(size < 0)
		{
			amx_LogicError(errors::out_of_range, "size");
		}
		bool ordered = optparam(2, 0);
		auto &pool = pool_pool.emplace(ordered);
		pool->resize(size);
		return pool_pool.get_id(pool);
	}

	// native bool:pool_valid(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_valid, 1, bool)
	{
		pool_t *ptr;
		return pool_pool.get_by_id(params[1], ptr);
	}

	// native pool_delete(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_delete, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return pool_pool.remove(ptr);
	}

	// native pool_delete_deep(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_delete_deep, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t old(ptr->ordered());
		ptr->swap(old);
		pool_pool.remove(ptr);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native Pool:pool_clone(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_clone, 1, pool)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto l = pool_pool.add();
		l->set_ordered(ptr->ordered());
		for(auto it = ptr->begin(); it != ptr->end(); ++it)
		{
			l->insert_or_set(ptr->index_of(it), it->clone());
		}
		return pool_pool.get_id(l);
	}

	// native pool_size(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_size, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return static_cast<cell>(ptr->num_elements());
	}

	// native pool_capacity(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_capacity, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native pool_resize(Pool:pool, newsize);
	AMX_DEFINE_NATIVE_TAG(pool_resize, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "newsize");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ucell newsize = params[2];
		ptr->resize(newsize);
		return 1;
	}

	// native pool_reserve(Pool:pool, capacity);
	AMX_DEFINE_NATIVE_TAG(pool_reserve, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "capacity");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ucell newsize = params[2];
		ptr->reserve(newsize);
		return 1;
	}

	// native pool_clear(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_clear, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t(ptr->ordered()).swap(*ptr);
		return 1;
	}

	// native pool_clear_deep(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_clear_deep, 1, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t old(ptr->ordered());
		ptr->swap(old);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native pool_set_ordered(Pool:pool, bool:ordered);
	AMX_DEFINE_NATIVE_TAG(pool_set_ordered, 2, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ptr->set_ordered(params[2]);
		return 1;
	}

	// native bool:pool_is_ordered(Pool:pool);
	AMX_DEFINE_NATIVE_TAG(pool_is_ordered, 1, bool)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return ptr->ordered();
	}

	// native pool_add(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_add, 3, cell)
	{
		return value_at<2, 3>::pool_add<dyn_func>(amx, params);
	}

	// native pool_add_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_add_arr, 4, cell)
	{
		return value_at<2, 3, 4>::pool_add<dyn_func_arr>(amx, params);
	}

	// native pool_add_str(Pool:pool, const value[]);
	AMX_DEFINE_NATIVE_TAG(pool_add_str, 2, cell)
	{
		return value_at<2>::pool_add<dyn_func_str>(amx, params);
	}

	// native pool_add_var(Pool:pool, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(pool_add_var, 2, cell)
	{
		return value_at<2>::pool_add<dyn_func_var>(amx, params);
	}

	// native bool:pool_remove(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_remove, 2, bool)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(index);
		if(it != ptr->end())
		{
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native bool:pool_remove_deep(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_remove_deep, 2, bool)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		
		auto it = ptr->find(index);
		if(it != ptr->end())
		{
			it->release();
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native pool_remove_range(Pool:pool, begin, end);
	AMX_DEFINE_NATIVE_TAG(pool_remove_range, 3, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "begin");
		if(params[3] < 0) amx_LogicError(errors::out_of_range, "end");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ucell begin = params[2];
		ucell end = params[3];
		if(begin >= ptr->size()) amx_LogicError(errors::out_of_range, "begin");
		if(end >= ptr->size() || end < begin) amx_LogicError(errors::out_of_range, "end");
		for(ucell i = begin; i <= end; i++)
		{
			auto it = ptr->find(i);
			if(it != ptr->end())
			{
				ptr->erase(it);
			}
		}
		return 1;
	}

	// native pool_remove_range_deep(Pool:pool, begin, end);
	AMX_DEFINE_NATIVE_TAG(pool_remove_range_deep, 3, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		if(static_cast<ucell>(params[2]) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		ucell begin = params[2];
		ucell end = params[3];
		if(begin >= ptr->size()) amx_LogicError(errors::out_of_range, "begin");
		if(end >= ptr->size() || end < begin) amx_LogicError(errors::out_of_range, "end");
		for(ucell i = begin; i <= end; i++)
		{
			auto it = ptr->find(i);
			if(it != ptr->end())
			{
				it->release();
				ptr->erase(it);
			}
		}
		return 1;
	}

	// native bool:pool_has(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_has, 2, bool)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return ptr->find(params[2]) != ptr->end();
	}

	// native pool_get(Pool:pool, index, offset=0);
	AMX_DEFINE_NATIVE(pool_get, 3)
	{
		return value_at<3>::pool_get<dyn_func>(amx, params);
	}

	// native pool_get_arr(Pool:pool, index, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(pool_get_arr, 4, cell)
	{
		return value_at<3, 4>::pool_get<dyn_func_arr>(amx, params);
	}

	// native String:pool_get_str_s(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_get_str_s, 2, string)
	{
		return value_at<>::pool_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:pool_get_var(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_get_var, 2, variant)
	{
		return value_at<>::pool_get<dyn_func_var>(amx, params);
	}

	// native bool:pool_get_safe(Pool:pool, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_get_safe, 5, bool)
	{
		return value_at<3, 4, 5>::pool_get<dyn_func>(amx, params);
	}

	// native pool_get_arr_safe(Pool:pool, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_get_arr_safe, 5, cell)
	{
		return value_at<3, 4, 5>::pool_get<dyn_func_arr>(amx, params);
	}

	// native pool_get_str_safe(Pool:pool, index, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(pool_get_str_safe, 4, cell)
	{
		return value_at<3, 4>::pool_get<dyn_func_str>(amx, params);
	}

	// native String:pool_get_str_safe_s(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_get_str_safe_s, 2, string)
	{
		return value_at<0>::pool_get<dyn_func_str_s>(amx, params);
	}

	// native pool_set(Pool:pool, index, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_set, 4, cell)
	{
		return value_at<3, 4>::pool_set<dyn_func>(amx, params);
	}

	// native pool_set_arr(Pool:pool, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_set_arr, 5, cell)
	{
		return value_at<3, 4, 5>::pool_set<dyn_func_arr>(amx, params);
	}

	// native pool_set_str(Pool:pool, index, const value[]);
	AMX_DEFINE_NATIVE_TAG(pool_set_str, 3, cell)
	{
		return value_at<3>::pool_set<dyn_func_str>(amx, params);
	}

	// native pool_set_var(Pool:pool, index, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(pool_set_var, 3, cell)
	{
		return value_at<3>::pool_set<dyn_func_var>(amx, params);
	}

	// native pool_set_cell(Pool:pool, index, offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(pool_set_cell, 3, cell)
	{
		return ::pool_set_cell(amx, params);
	}

	// native bool:pool_set_cell_safe(Pool:pool, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_set_cell_safe, 5, bool)
	{
		return ::pool_set_cell<5>(amx, params);
	}

	// native pool_tagof(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_tagof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(params[2]);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		auto &obj = *it;
		return obj.get_tag(amx);
	}

	// native pool_sizeof(Pool:pool, index);
	AMX_DEFINE_NATIVE_TAG(pool_sizeof, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(params[2]);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		auto &obj = *it;
		return obj.get_size();
	}

	// native pool_remove_if(Pool:pool, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(pool_remove_if, 2, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);

		cell count = 0;
		auto it = ptr->begin();
		while(it != ptr->end())
		{
			args[0] = std::cref(*it);
			key = dyn_object(ptr->index_of(it), tags::find_tag(tags::tag_cell));
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

	// native pool_remove_if_deep(Pool:pool, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(pool_remove_if_deep, 2, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);

		cell count = 0;
		auto it = ptr->begin();
		while(it != ptr->end())
		{
			args[0] = std::cref(*it);
			key = dyn_object(ptr->index_of(it), tags::find_tag(tags::tag_cell));
			if(expr->execute_bool(args, info))
			{
				it->release();
				it = ptr->erase(it);
				count++;
			}else{
				++it;
			}
		}
		return count;
	}

	// native pool_find(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_find, 3, cell)
	{
		return value_at<2, 3>::pool_find<dyn_func>(amx, params);
	}

	// native pool_find_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_find_arr, 4, cell)
	{
		return value_at<2, 3, 4>::pool_find<dyn_func_arr>(amx, params);
	}

	// native pool_find_str(Pool:pool, const value[]);
	AMX_DEFINE_NATIVE_TAG(pool_find_str, 2, cell)
	{
		return value_at<2>::pool_find<dyn_func_str>(amx, params);
	}

	// native pool_find_var(Pool:pool, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(pool_find_var, 2, cell)
	{
		return value_at<2>::pool_find<dyn_func_var>(amx, params);
	}

	// native pool_find_if(Pool:pool, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(pool_find_if, 2, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);
		for(auto it = ptr->begin(); it != ptr->end(); ++it)
		{
			args[0] = std::cref(*it);
			key = dyn_object(ptr->index_of(it), tags::find_tag(tags::tag_cell));
			if(expr->execute_bool(args, info))
			{
				return ptr->index_of(it);
			}
		}
		return -1;
	}

	// native pool_count(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_count, 3, cell)
	{
		return value_at<2, 3>::pool_count<dyn_func>(amx, params);
	}

	// native pool_count_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(pool_count_arr, 4, cell)
	{
		return value_at<2, 3, 4>::pool_count<dyn_func_arr>(amx, params);
	}

	// native pool_count_str(Pool:pool, const value[]);
	AMX_DEFINE_NATIVE_TAG(pool_count_str, 2, cell)
	{
		return value_at<2>::pool_count<dyn_func_str>(amx, params);
	}

	// native pool_count_var(Pool:pool, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(pool_count_var, 2, cell)
	{
		return value_at<2>::pool_count<dyn_func_var>(amx, params);
	}

	// native pool_count_if(Pool:pool, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(pool_count_if, 2, cell)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		dyn_object key;
		expression::args_type args;
		args.push_back(std::cref(key));
		args.push_back(std::cref(key));
		expression::exec_info info(amx);

		cell count = 0;
		for(auto it = ptr->begin(); it != ptr->end(); ++it)
		{
			args[0] = std::cref(*it);
			key = dyn_object(ptr->index_of(it), tags::find_tag(tags::tag_cell));
			if(expr->execute_bool(args, info))
			{
				count++;
			}
		}
		return count;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(pool_new),
	AMX_DECLARE_NATIVE(pool_valid),
	AMX_DECLARE_NATIVE(pool_delete),
	AMX_DECLARE_NATIVE(pool_delete_deep),
	AMX_DECLARE_NATIVE(pool_set_ordered),
	AMX_DECLARE_NATIVE(pool_is_ordered),
	AMX_DECLARE_NATIVE(pool_clone),
	AMX_DECLARE_NATIVE(pool_size),
	AMX_DECLARE_NATIVE(pool_resize),
	AMX_DECLARE_NATIVE(pool_capacity),
	AMX_DECLARE_NATIVE(pool_reserve),
	AMX_DECLARE_NATIVE(pool_clear),
	AMX_DECLARE_NATIVE(pool_clear_deep),

	AMX_DECLARE_NATIVE(pool_add),
	AMX_DECLARE_NATIVE(pool_add_arr),
	AMX_DECLARE_NATIVE(pool_add_str),
	AMX_DECLARE_NATIVE(pool_add_var),

	AMX_DECLARE_NATIVE(pool_remove),
	AMX_DECLARE_NATIVE(pool_remove_deep),
	AMX_DECLARE_NATIVE(pool_remove_range),
	AMX_DECLARE_NATIVE(pool_remove_range_deep),
	AMX_DECLARE_NATIVE(pool_remove_if),
	AMX_DECLARE_NATIVE(pool_remove_if_deep),

	AMX_DECLARE_NATIVE(pool_has),

	AMX_DECLARE_NATIVE(pool_get),
	AMX_DECLARE_NATIVE(pool_get_arr),
	AMX_DECLARE_NATIVE(pool_get_str_s),
	AMX_DECLARE_NATIVE(pool_get_var),
	AMX_DECLARE_NATIVE(pool_get_safe),
	AMX_DECLARE_NATIVE(pool_get_arr_safe),
	AMX_DECLARE_NATIVE(pool_get_str_safe),
	AMX_DECLARE_NATIVE(pool_get_str_safe_s),

	AMX_DECLARE_NATIVE(pool_set),
	AMX_DECLARE_NATIVE(pool_set_arr),
	AMX_DECLARE_NATIVE(pool_set_str),
	AMX_DECLARE_NATIVE(pool_set_var),
	AMX_DECLARE_NATIVE(pool_set_cell),
	AMX_DECLARE_NATIVE(pool_set_cell_safe),

	AMX_DECLARE_NATIVE(pool_find),
	AMX_DECLARE_NATIVE(pool_find_arr),
	AMX_DECLARE_NATIVE(pool_find_str),
	AMX_DECLARE_NATIVE(pool_find_var),
	AMX_DECLARE_NATIVE(pool_find_if),

	AMX_DECLARE_NATIVE(pool_count),
	AMX_DECLARE_NATIVE(pool_count_arr),
	AMX_DECLARE_NATIVE(pool_count_str),
	AMX_DECLARE_NATIVE(pool_count_var),
	AMX_DECLARE_NATIVE(pool_count_if),

	AMX_DECLARE_NATIVE(pool_tagof),
	AMX_DECLARE_NATIVE(pool_sizeof),
};

int RegisterPoolNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
