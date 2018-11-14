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
	// native bool:iter_set(IterTag:iter, ...);
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

	// native iter_get(IterTag:iter, ...);
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
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return Factory(amx, spair->second, params[Indices]...);
			}
		}
		return 0;
	}

	// native bool:iter_insert(IterTag:iter, ...);
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

	// native iter_get_key(IterTag:iter, ...);
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
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return Factory(amx, spair->first, params[Indices]...);
			}
		}
		return 0;
	}
};

template <size_t... KeyIndices>
class key_at
{
	using key_ftype = typename dyn_factory<KeyIndices...>::type;

public:
	// native Iter:map_iter_at(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_iter_at(AMX *amx, cell *params)
	{
		std::shared_ptr<map_t> ptr;
		if(map_pool.get_by_id(params[1], ptr))
		{
			auto &iter = iter_pool.add(std::make_unique<map_iterator_t>(ptr, ptr->find(KeyFactory(amx, params[KeyIndices]...))));
			return iter_pool.get_id(iter);
		}
		return 0;
	}
};

// native bool:iter_set_cell(IterTag:iter, offset, AnyTag:value, ...);
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
	// native Iter:list_iter(List:list, index=0);
	AMX_DEFINE_NATIVE(list_iter, 1)
	{
		std::shared_ptr<list_t> ptr;
		if(list_pool.get_by_id(params[1], ptr))
		{
			auto &iter = iter_pool.add(std::make_unique<list_iterator_t>(ptr));
			cell index = optparam(2, 0);
			if(index < 0)
			{
				iter->reset();
			}else{
				for(cell i = 0; i < index; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}
			return iter_pool.get_id(iter);
		}
		return 0;
	}

	// native Iter:map_iter(Map:map, index=0);
	AMX_DEFINE_NATIVE(map_iter, 1)
	{
		std::shared_ptr<map_t> ptr;
		if(map_pool.get_by_id(params[1], ptr))
		{
			auto &iter = iter_pool.add(std::make_unique<map_iterator_t>(ptr));
			cell index = optparam(2, 0);
			if(index < 0)
			{
				iter->reset();
			}else{
				for(cell i = 0; i < index; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}
			return iter_pool.get_id(iter);
		}
		return 0;
	}

	// native Iter:map_iter_at(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_iter_at, 3)
	{
		return key_at<2, 3>::map_iter_at<dyn_func>(amx, params);
	}

	// native Iter:map_iter_at_arr(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_iter_at_arr, 4)
	{
		return key_at<2, 3, 4>::map_iter_at<dyn_func_arr>(amx, params);
	}

	// native Iter:map_iter_at_str(Map:map, const key[]);
	AMX_DEFINE_NATIVE(map_iter_at_str, 2)
	{
		return key_at<2>::map_iter_at<dyn_func_str>(amx, params);
	}

	// native Iter:map_iter_at_var(Map:map, ConstVariantTag:key);
	AMX_DEFINE_NATIVE(map_iter_at_var, 2)
	{
		return key_at<2>::map_iter_at<dyn_func_var>(amx, params);
	}

	// native Iter:linked_list_iter(LinkedList:linked_list, index=0);
	AMX_DEFINE_NATIVE(linked_list_iter, 1)
	{
		std::shared_ptr<linked_list_t> ptr;
		if(linked_list_pool.get_by_id(params[1], ptr))
		{
			auto &iter = iter_pool.add(std::make_unique<linked_list_iterator_t>(ptr));
			cell index = optparam(2, 0);
			if(index < 0)
			{
				iter->reset();
			}else{
				for(cell i = 0; i < index; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}
			return iter_pool.get_id(iter);
		}
		return 0;
	}

	// native bool:iter_valid(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_valid, 1)
	{
		dyn_iterator *iter;
		return iter_pool.get_by_id(params[1], iter);
	}

	// native Iter:iter_acquire(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_acquire, 1)
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(params[1], iter)) return 0;
		if(!iter_pool.acquire_ref(*iter)) return 0;
		return params[1];
	}

	// native Iter:iter_release(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_release, 1)
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(params[1], iter)) return 0;
		if(!iter_pool.release_ref(*iter)) return 0;
		return params[1];
	}

	// native bool:iter_delete(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_delete, 1)
	{
		return iter_pool.remove_by_id(params[1]);
	}

	// native bool:iter_linked(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_linked, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return !iter->expired();
		}
		return 0;
	}

	// native bool:iter_inside(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_inside, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return iter->valid();
		}
		return 0;
	}

	// native Iterator:iter_erase(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_erase, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->erase();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_reset(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_reset, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			iter->reset();
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_clone(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_clone, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			return iter_pool.get_id(iter_pool.add(std::unique_ptr<object_pool<dyn_iterator>::ref_container>(&dynamic_cast<object_pool<dyn_iterator>::ref_container&>(*iter->clone().release()))));
		}
		return 0;
	}

	// native bool:iter_eq(IterTag:iter1, IterTag:iter2);
	AMX_DEFINE_NATIVE(iter_eq, 2)
	{
		dyn_iterator *iter1;
		if(!iter_pool.get_by_id(params[1], iter1)) return 0;
		dyn_iterator *iter2;
		if(!iter_pool.get_by_id(params[2], iter2)) return 0;
		return *iter1 == *iter2;
	}

	// native Iterator:iter_move_next(IterTag:iter, steps=1);
	AMX_DEFINE_NATIVE(iter_move_next, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			cell steps = optparam(2, 1);
			if(steps < 0)
			{
				for(cell i = steps; i < 0; i++)
				{
					if(!iter->move_previous())
					{
						break;
					}
				}
			}else{
				for(cell i = 0; i < steps; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_move_previous(IterTag:iter, steps=1);
	AMX_DEFINE_NATIVE(iter_move_previous, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			cell steps = optparam(2, 1);
			if(steps < 0)
			{
				for(cell i = steps; i < 0; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}else{
				for(cell i = 0; i < steps; i++)
				{
					if(!iter->move_previous())
					{
						break;
					}
				}
			}
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_to_first(IterTag:iter, index=0);
	AMX_DEFINE_NATIVE(iter_to_first, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			cell index = optparam(2, 0);
			if(index < 0)
			{
				iter->reset();
			}else{
				iter->set_to_first();
				for(cell i = 0; i < index; i++)
				{
					if(!iter->move_next())
					{
						break;
					}
				}
			}
			return params[1];
		}
		return 0;
	}

	// native Iterator:iter_to_last(IterTag:iter, index=0);
	AMX_DEFINE_NATIVE(iter_to_last, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			cell index = optparam(2, 0);
			if(index < 0)
			{
				iter->reset();
			}else{
				iter->set_to_last();
				for(cell i = 0; i < index; i++)
				{
					if(!iter->move_previous())
					{
						break;
					}
				}
			}
			return params[1];
		}
		return 0;
	}

	// native iter_get(IterTag:iter, offset=0);
	AMX_DEFINE_NATIVE(iter_get, 2)
	{
		return value_at<2>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_arr, 3)
	{
		return value_at<2, 3>::iter_get<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_var(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_var, 1)
	{
		return value_at<>::iter_get<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_safe(IterTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_arr_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set(IterTag:iter, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set, 3)
	{
		return value_at<2, 3>::iter_set<dyn_func>(amx, params);
	}

	// native bool:iter_set_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set_arr, 4)
	{
		return value_at<2, 3, 4>::iter_set<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_str(IterTag:iter, const value[]);
	AMX_DEFINE_NATIVE(iter_set_str, 2)
	{
		return value_at<2>::iter_set<dyn_func_str>(amx, params);
	}

	// native bool:iter_set_var(IterTag:iter, VariantTag:value);
	AMX_DEFINE_NATIVE(iter_set_var, 2)
	{
		return value_at<2>::iter_set<dyn_func_var>(amx, params);
	}

	// native bool:iter_set_cell(IterTag:iter, offset, AnyTag:value);
	AMX_DEFINE_NATIVE(iter_set_cell, 3)
	{
		return ::iter_set_cell(amx, params);
	}

	// native bool:iter_set_cell_safe(IterTag:iter, offset, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set_cell_safe, 4)
	{
		return ::iter_set_cell<4>(amx, params);
	}

	// native bool:iter_insert(IterTag:iter, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_insert, 3)
	{
		return value_at<2, 3>::iter_insert<dyn_func>(amx, params);
	}

	// native bool:iter_insert_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_insert_arr, 4)
	{
		return value_at<2, 3, 4>::iter_insert<dyn_func_arr>(amx, params);
	}

	// native bool:iter_insert_str(IterTag:iter, const value[]);
	AMX_DEFINE_NATIVE(iter_insert_str, 2)
	{
		return value_at<2>::iter_insert<dyn_func_str>(amx, params);
	}

	// native bool:iter_insert_var(IterTag:iter, VariantTag:value);
	AMX_DEFINE_NATIVE(iter_insert_var, 2)
	{
		return value_at<2>::iter_insert<dyn_func_var>(amx, params);
	}

	// native iter_get_key(IterTag:iter, offset=0);
	AMX_DEFINE_NATIVE(iter_get_key, 2)
	{
		return value_at<2>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_key_arr, 3)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_key_var(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_key_var, 1)
	{
		return value_at<>::iter_get_key<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_key_safe(IterTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_arr_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native iter_tagof(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_tagof, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			dyn_object *obj;
			if(iter->extract(obj))
			{
				return obj->get_tag(amx);
			}
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return pair->second.get_tag(amx);
			}
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return pair->second.get_tag(amx);
			}
		}
		return 0;
	}

	// native iter_sizeof(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_sizeof, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			dyn_object *obj;
			if(iter->extract(obj))
			{
				return obj->get_size();
			}
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return pair->second.get_size();
			}
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return pair->second.get_size();
			}
		}
		return 0;
	}

	// native iter_tagof_key(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_tagof_key, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return pair->first.get_tag(amx);
			}
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return pair->first.get_tag(amx);
			}
		}
		return 0;
	}

	// native iter_sizeof_key(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_sizeof_key, 1)
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(params[1], iter))
		{
			std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				return pair->first.get_size();
			}
			std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
			if(iter->extract(spair))
			{
				return pair->first.get_size();
			}
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_iter),
	AMX_DECLARE_NATIVE(map_iter),
	AMX_DECLARE_NATIVE(map_iter_at),
	AMX_DECLARE_NATIVE(map_iter_at_arr),
	AMX_DECLARE_NATIVE(map_iter_at_str),
	AMX_DECLARE_NATIVE(map_iter_at_var),
	AMX_DECLARE_NATIVE(linked_list_iter),
	AMX_DECLARE_NATIVE(iter_valid),
	AMX_DECLARE_NATIVE(iter_acquire),
	AMX_DECLARE_NATIVE(iter_release),
	AMX_DECLARE_NATIVE(iter_delete),
	AMX_DECLARE_NATIVE(iter_linked),
	AMX_DECLARE_NATIVE(iter_inside),
	AMX_DECLARE_NATIVE(iter_erase),
	AMX_DECLARE_NATIVE(iter_reset),
	AMX_DECLARE_NATIVE(iter_clone),
	AMX_DECLARE_NATIVE(iter_eq),
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
	AMX_DECLARE_NATIVE(iter_sizeof),
	AMX_DECLARE_NATIVE(iter_tagof),
	AMX_DECLARE_NATIVE(iter_sizeof_key),
	AMX_DECLARE_NATIVE(iter_tagof_key),
};

int RegisterIterNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
