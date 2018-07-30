#include "natives.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include "objects/dyn_object.h"
#include "fixes/linux.h"
#include <vector>
#include <unordered_map>
//#include <unordered_set>
//#include <deque>
#include <type_traits>

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native bool:iter_set(IteratorTag:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_set(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			dyn_object *obj;
			if(iter->extract(obj))
			{
				*obj = Factory(amx, params[Indices]...);
				return 1;
			}
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				pair->second = Factory(amx, params[Indices]...);
				return 1;
			}
		}
		return 0;
	}

	// native iter_get(IteratorTag:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			dyn_object *obj;
			if(iter->extract(obj))
			{
				return Factory(amx, *obj, params[Indices]...);
			}
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return Factory(amx, pair->second, params[Indices]...);
			}
		}
		return 0;
	}

	// native bool:iter_insert(IteratorTag:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_insert(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			if(iter->insert(Factory(amx, params[Indices]...)))
			{
				return 1;
			}
		}
		return 0;
	}

	// native iter_get_key(IteratorTag:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get_key(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return Factory(amx, pair->first, params[Indices]...);
			}
		}
		return 0;
	}
};

// native bool:iter_set_cell(IteratorTag:iter, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL iter_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) return 0;
	dyn_iterator *iter;
	if(iter_pool.get_by_id(params[1], iter))
	{
		dyn_object *obj;
		if(iter->extract(obj))
		{
			if(TagIndex && !obj->tag_assignable(amx, params[TagIndex])) return 0;
			return obj->set_cell(params[2], params[3]);
		}
		std::pair<const dyn_object, dyn_object> *pair;
		if(iter->extract(pair))
		{
			auto &obj = pair->second;
			if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
			return obj.set_cell(params[2], params[3]);
		}
	}
	return 0;
}

namespace Natives
{
	// native ListIterator:list_iter(List:list);
	static cell AMX_NATIVE_CALL list_iter(AMX *amx, cell *params)
	{
		std::shared_ptr<list_t> ptr;
		if(list_pool.get_by_id(params[1], ptr))
		{
			auto iter = iter_pool.add(std::make_unique<list_iterator_t>(ptr), true);
			return iter_pool.get_id(iter);
		}
		return 0;
	}

	// native MapIterator:map_iter(Map:map);
	static cell AMX_NATIVE_CALL map_iter(AMX *amx, cell *params)
	{
		std::shared_ptr<map_t> ptr;
		if(map_pool.get_by_id(params[1], ptr))
		{
			auto iter = iter_pool.add(std::make_unique<map_iterator_t>(ptr), true);
			return iter_pool.get_id(iter);
		}
		return 0;
	}

	// native bool:iter_valid(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_valid(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		return iter_pool.get_by_id(params[1], iter);
	}

	// native GlobalIterator:str_to_global(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_to_global(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter_pool.move_to_global(iter);
			return params[1];
		}
		return 0;
	}

	// native String:str_to_local(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_to_local(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter_pool.move_to_local(iter);
			return params[1];
		}
		return 0;
	}

	// native bool:iter_delete(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_delete(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return iter_pool.remove(iter);
		}
		return 0;
	}

	// native bool:iter_linked(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_linked(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return !iter->expired();
		}
		return 0;
	}

	// native bool:iter_inside(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_inside(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return iter->valid();
		}
		return 0;
	}

	// native Iterator:iter_erase(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_erase(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->erase();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_reset(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_reset(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->reset();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_clone(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_clone(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return iter_pool.get_id(iter_pool.clone(iter));
		}
		return 0;
	}

	// native bool:iter_equal(IteratorTag:iter1, IteratorTag:iter2);
	static cell AMX_NATIVE_CALL iter_equal(AMX *amx, cell *params)
	{
		if(params[1] == params[2]) return 1;
		dyn_iterator *iter1, *iter2;
		if(iter_pool.get_by_id(params[1], iter1) && iter_pool.get_by_id(params[2], iter2))
		{
			return *iter1 == *iter2;
		}
		return 0;
	}

	// native Iterator:iter_move_next(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_move_next(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->move_next();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_move_previous(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_move_previous(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->move_previous();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_to_first(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_to_first(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->set_to_first();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_to_last(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_to_last(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->set_to_last();
			return params[1];
		}
		return 0;
	}

	// native iter_get(IteratorTag:iter, offset=0);
	static cell AMX_NATIVE_CALL iter_get(AMX *amx, cell *params)
	{
		return value_at<2>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr(IteratorTag:iter, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL iter_get_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_get<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_var(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_get_var(AMX *amx, cell *params)
	{
		return value_at<>::iter_get<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_safe(IteratorTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr_safe(IteratorTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set(IteratorTag:iter, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_set<dyn_func>(amx, params);
	}

	// native bool:iter_set_arr(IteratorTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_set<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_str(IteratorTag:iter, const value[]);
	static cell AMX_NATIVE_CALL iter_set_str(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set<dyn_func_str>(amx, params);
	}

	// native bool:iter_set_var(IteratorTag:iter, VariantTag:value);
	static cell AMX_NATIVE_CALL iter_set_var(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set<dyn_func_var>(amx, params);
	}

	// native bool:iter_set_cell(IteratorTag:iter, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL iter_set_cell(AMX *amx, cell *params)
	{
		return ::iter_set_cell(amx, params);
	}

	// native bool:iter_set_cell_safe(IteratorTag:iter, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_cell_safe(AMX *amx, cell *params)
	{
		return ::iter_set_cell<4>(amx, params);
	}

	// native bool:iter_insert(IteratorTag:iter, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_insert(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_insert<dyn_func>(amx, params);
	}

	// native bool:iter_insert_arr(IteratorTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_insert_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_insert<dyn_func_arr>(amx, params);
	}

	// native bool:iter_insert_str(IteratorTag:iter, const value[]);
	static cell AMX_NATIVE_CALL iter_insert_str(AMX *amx, cell *params)
	{
		return value_at<2>::iter_insert<dyn_func_str>(amx, params);
	}

	// native bool:iter_insert_var(IteratorTag:iter, VariantTag:value);
	static cell AMX_NATIVE_CALL iter_insert_var(AMX *amx, cell *params)
	{
		return value_at<2>::iter_insert<dyn_func_var>(amx, params);
	}

	// native iter_get_key(IteratorTag:iter, offset=0);
	static cell AMX_NATIVE_CALL iter_get_key(AMX *amx, cell *params)
	{
		return value_at<2>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr(IteratorTag:iter, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL iter_get_key_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_key_var(IteratorTag:iter);
	static cell AMX_NATIVE_CALL iter_get_key_var(AMX *amx, cell *params)
	{
		return value_at<>::iter_get_key<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_key_safe(IteratorTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_key_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr_safe(IteratorTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_key_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func_arr>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_iter),
	AMX_DECLARE_NATIVE(map_iter),
	AMX_DECLARE_NATIVE(iter_valid),
	AMX_DECLARE_NATIVE(iter_delete),
	AMX_DECLARE_NATIVE(iter_linked),
	AMX_DECLARE_NATIVE(iter_inside),
	AMX_DECLARE_NATIVE(iter_erase),
	AMX_DECLARE_NATIVE(iter_reset),
	AMX_DECLARE_NATIVE(iter_clone),
	AMX_DECLARE_NATIVE(iter_equal),
	AMX_DECLARE_NATIVE(iter_move_next),
	AMX_DECLARE_NATIVE(iter_move_previous),
	AMX_DECLARE_NATIVE(iter_to_first),
	AMX_DECLARE_NATIVE(iter_to_last),
	AMX_DECLARE_NATIVE(iter_get),
	AMX_DECLARE_NATIVE(iter_get_arr),
	AMX_DECLARE_NATIVE(iter_get_var),
	AMX_DECLARE_NATIVE(iter_get_safe),
	AMX_DECLARE_NATIVE(iter_get_arr_safe),
	AMX_DECLARE_NATIVE(iter_set),
	AMX_DECLARE_NATIVE(iter_set_arr),
	AMX_DECLARE_NATIVE(iter_set_str),
	AMX_DECLARE_NATIVE(iter_set_var),
	AMX_DECLARE_NATIVE(iter_set_cell),
	AMX_DECLARE_NATIVE(iter_set_cell_safe),
	AMX_DECLARE_NATIVE(iter_insert),
	AMX_DECLARE_NATIVE(iter_insert_arr),
	AMX_DECLARE_NATIVE(iter_insert_str),
	AMX_DECLARE_NATIVE(iter_insert_var),
	AMX_DECLARE_NATIVE(iter_get_key),
	AMX_DECLARE_NATIVE(iter_get_key_arr),
	AMX_DECLARE_NATIVE(iter_get_key_var),
	AMX_DECLARE_NATIVE(iter_get_key_safe),
	AMX_DECLARE_NATIVE(iter_get_key_arr_safe),
};

int RegisterIterNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
