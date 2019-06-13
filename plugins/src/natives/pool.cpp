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
		if(index < 0) amx_LogicError(errors::out_of_range, "pool index");
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
		if(index < 0) amx_LogicError(errors::out_of_range, "pool index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(index);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		return Factory(amx, *it, params[Indices]...);
	}
};

// native pool_set_cell(Pool:pool, index, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL pool_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "pool index");
	if(params[3] < 0) amx_LogicError(errors::out_of_range, "array offset");
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
	// native Pool:pool_new(capacity=-1);
	AMX_DEFINE_NATIVE(pool_new, 0)
	{
		cell size = optparam(1, -1);
		if(size == -1)
		{
			size = 1;
		}else if(size < 0)
		{
			amx_LogicError(errors::out_of_range, "size");
		}
		auto &pool = pool_pool.add();
		pool->resize(size);
		return pool_pool.get_id(pool);
	}

	// native bool:pool_valid(Pool:pool);
	AMX_DEFINE_NATIVE(pool_valid, 1)
	{
		pool_t *ptr;
		return pool_pool.get_by_id(params[1], ptr);
	}

	// native pool_delete(Pool:pool);
	AMX_DEFINE_NATIVE(pool_delete, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return pool_pool.remove(ptr);
	}

	// native pool_delete_deep(Pool:pool);
	AMX_DEFINE_NATIVE(pool_delete_deep, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t old;
		ptr->swap(old);
		pool_pool.remove(ptr);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native Pool:pool_clone(Pool:pool);
	AMX_DEFINE_NATIVE(pool_clone, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto l = pool_pool.add();
		for(auto &&obj : *ptr)
		{
			l->push_back(obj.clone());
		}
		return pool_pool.get_id(l);
	}

	// native pool_size(Pool:pool);
	AMX_DEFINE_NATIVE(pool_size, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return static_cast<cell>(ptr->num_elements());
	}

	// native pool_capacity(Pool:pool);
	AMX_DEFINE_NATIVE(pool_capacity, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native pool_resize(Pool:pool, newsize);
	AMX_DEFINE_NATIVE(pool_resize, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "newsize");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ucell newsize = params[2];
		ptr->resize(newsize);
		return 1;
	}

	// native pool_reserve(Pool:pool, capacity);
	AMX_DEFINE_NATIVE(pool_reserve, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "capacity");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ucell newsize = params[2];
		ptr->reserve(newsize);
		return 1;
	}

	// native pool_clear(Pool:pool);
	AMX_DEFINE_NATIVE(pool_clear, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t().swap(*ptr);
		return 1;
	}

	// native pool_clear_deep(Pool:pool);
	AMX_DEFINE_NATIVE(pool_clear_deep, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		pool_t old;
		ptr->swap(old);
		pool_pool.remove(ptr);
		for(auto &obj : old)
		{
			obj.release();
		}
		return 1;
	}

	// native pool_set_ordered(Pool:pool, bool:ordered);
	AMX_DEFINE_NATIVE(pool_set_ordered, 2)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		ptr->set_ordered(params[2]);
		return 1;
	}

	// native bool:pool_is_ordered(Pool:pool);
	AMX_DEFINE_NATIVE(pool_is_ordered, 1)
	{
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		return ptr->ordered();
	}

	// native pool_add(Pool:pool, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_add, 3)
	{
		return value_at<2, 3>::pool_add<dyn_func>(amx, params);
	}

	// native pool_add_arr(Pool:pool, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_add_arr, 4)
	{
		return value_at<2, 3, 4>::pool_add<dyn_func_arr>(amx, params);
	}

	// native pool_add_str(Pool:pool, const value[]);
	AMX_DEFINE_NATIVE(pool_add_str, 2)
	{
		return value_at<2>::pool_add<dyn_func_str>(amx, params);
	}

	// native pool_add_var(Pool:pool, VariantTag:value);
	AMX_DEFINE_NATIVE(pool_add_var, 2)
	{
		return value_at<2>::pool_add<dyn_func_var>(amx, params);
	}

	// native bool:pool_remove(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_remove, 2)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "pool index");
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
	AMX_DEFINE_NATIVE(pool_remove_deep, 2)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "pool index");
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


	// native bool:pool_has(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_has, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "pool index");
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
	AMX_DEFINE_NATIVE(pool_get_arr, 4)
	{
		return value_at<3, 4>::pool_get<dyn_func_arr>(amx, params);
	}

	// native String:pool_get_str_s(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_get_str_s, 2)
	{
		return value_at<>::pool_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:pool_get_var(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_get_var, 2)
	{
		return value_at<>::pool_get<dyn_func_var>(amx, params);
	}

	// native bool:pool_get_safe(Pool:pool, index, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_get_safe, 5)
	{
		return value_at<3, 4, 5>::pool_get<dyn_func>(amx, params);
	}

	// native pool_get_arr_safe(Pool:pool, index, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_get_arr_safe, 5)
	{
		return value_at<3, 4, 5>::pool_get<dyn_func_arr>(amx, params);
	}

	// native pool_get_str_safe(Pool:pool, index, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(pool_get_str_safe, 4)
	{
		return value_at<3, 4>::pool_get<dyn_func_str>(amx, params);
	}

	// native String:pool_get_str_safe_s(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_get_str_safe_s, 2)
	{
		return value_at<0>::pool_get<dyn_func_str_s>(amx, params);
	}

	// native pool_set(Pool:pool, index, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_set, 4)
	{
		return value_at<3, 4>::pool_set<dyn_func>(amx, params);
	}

	// native pool_set_arr(Pool:pool, index, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_set_arr, 5)
	{
		return value_at<3, 4, 5>::pool_set<dyn_func_arr>(amx, params);
	}

	// native pool_set_str(Pool:pool, index, const value[]);
	AMX_DEFINE_NATIVE(pool_set_str, 3)
	{
		return value_at<3>::pool_set<dyn_func_str>(amx, params);
	}

	// native pool_set_var(Pool:pool, index, VariantTag:value);
	AMX_DEFINE_NATIVE(pool_set_var, 3)
	{
		return value_at<3>::pool_set<dyn_func_var>(amx, params);
	}

	// native pool_set_cell(Pool:pool, index, offset, AnyTag:value);
	AMX_DEFINE_NATIVE(pool_set_cell, 3)
	{
		return ::pool_set_cell(amx, params);
	}

	// native bool:pool_set_cell_safe(Pool:pool, index, offset, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(pool_set_cell_safe, 5)
	{
		return ::pool_set_cell<5>(amx, params);
	}

	// native pool_tagof(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_tagof, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "pool index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(params[2]);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		auto &obj = *it;
		return obj.get_tag(amx);
	}

	// native pool_sizeof(Pool:pool, index);
	AMX_DEFINE_NATIVE(pool_sizeof, 2)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "pool index");
		pool_t *ptr;
		if(!pool_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "pool", params[1]);
		auto it = ptr->find(params[2]);
		if(it == ptr->end()) amx_LogicError(errors::element_not_present);
		auto &obj = *it;
		return obj.get_size();
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

	AMX_DECLARE_NATIVE(pool_tagof),
	AMX_DECLARE_NATIVE(pool_sizeof),
};

int RegisterPoolNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
