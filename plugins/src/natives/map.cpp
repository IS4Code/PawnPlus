#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include "modules/expressions.h"
#include <iterator>
#include <algorithm>

template <size_t... KeyIndices>
class key_at
{
	using key_ftype = typename dyn_factory<KeyIndices...>::type;

public:
	template <size_t... ValueIndices>
	class value_at
	{
		using value_ftype = typename dyn_factory<ValueIndices...>::type;
		using result_ftype = typename dyn_result<ValueIndices...>::type;

	public:
		// native bool:map_add(Map:map, key, value, ...);
		template <key_ftype KeyFactory, value_ftype ValueFactory>
		static cell AMX_NATIVE_CALL map_add(AMX *amx, cell *params)
		{
			map_t *ptr;
			if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
			auto ret = ptr->insert(KeyFactory(amx, params[KeyIndices]...), ValueFactory(amx, params[ValueIndices]...));
			return static_cast<cell>(ret.second);
		}

		// native map_set(Map:map, key, value, ...);
		template <key_ftype KeyFactory, value_ftype ValueFactory>
		static cell AMX_NATIVE_CALL map_set(AMX *amx, cell *params)
		{
			map_t *ptr;
			if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
			(*ptr)[KeyFactory(amx, params[KeyIndices]...)] = ValueFactory(amx, params[ValueIndices]...);
			return 1;
		}

		// native map_get(Map:map, key, ...);
		template <key_ftype KeyFactory, result_ftype ValueFactory>
		static cell AMX_NATIVE_CALL map_get(AMX *amx, cell *params)
		{
			map_t *ptr;
			if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
			auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
			if(it != ptr->end())
			{
				return ValueFactory(amx, it->second, params[ValueIndices]...);
			}
			amx_LogicError(errors::element_not_present);
			return 0;
		}
	};

	// native bool:map_remove(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_remove(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native bool:map_remove_deep(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_remove_deep(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			it->first.release();
			it->second.release();
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native bool:map_has_key(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_has_key(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			return 1;
		}
		return 0;
	}
	
	// native map_set_cell(Map:map, key, offset, AnyTag:value, ...);
	template <key_ftype KeyFactory, size_t TagIndex = 0>
	static cell AMX_NATIVE_CALL map_set_cell(AMX *amx, cell *params)
	{
		if(params[3] < 0) amx_LogicError(errors::out_of_range, "offset");
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return 0;
			obj.set_cell(params[3], params[4]);
			return 1;
		}
		amx_LogicError(errors::element_not_present);
		return 0;
	}

	// native map_tagof(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_tagof(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			return it->second.get_tag(amx);
		}
		amx_LogicError(errors::element_not_present);
		return 0;
	}

	// native map_sizeof(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_sizeof(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto it = ptr->find(KeyFactory(amx, params[KeyIndices]...));
		if(it != ptr->end())
		{
			return it->second.get_size();
		}
		amx_LogicError(errors::element_not_present);
		return 0;
	}
};

template <size_t... ValueIndices>
class value_at
{
	using value_ftype = typename dyn_factory<ValueIndices...>::type;
	using result_ftype = typename dyn_result<ValueIndices...>::type;

public:
	// native map_key_at(Map:map, index, ...);
	template <result_ftype ValueFactory>
	static cell AMX_NATIVE_CALL map_key_at(AMX *amx, cell *params)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		if(static_cast<size_t>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto it = ptr->begin();
		std::advance(it, index);
		if(it != ptr->end())
		{
			return ValueFactory(amx, it->first, params[ValueIndices]...);
		}
		amx_LogicError(errors::element_not_present);
		return 0;
	}

	// native map_value_at(Map:map, index, ...);
	template <result_ftype ValueFactory>
	static cell AMX_NATIVE_CALL map_value_at(AMX *amx, cell *params)
	{
		cell index = params[2];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		if(static_cast<size_t>(index) >= ptr->size()) amx_LogicError(errors::out_of_range, "index");
		auto it = ptr->begin();
		std::advance(it, index);
		if(it != ptr->end())
		{
			return ValueFactory(amx, it->second, params[ValueIndices]...);
		}
		amx_LogicError(errors::element_not_present);
		return 0;
	}

	// native map_count(Map:map, value, ...);
	template <value_ftype ValueFactory>
	static cell AMX_NATIVE_CALL map_count(AMX *amx, cell *params)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto find = ValueFactory(amx, params[ValueIndices]...);
		return std::count_if(ptr->begin(), ptr->end(), [&](const std::pair<const dyn_object, dyn_object> &pair)
		{
			return pair.second == find;
		});
	}
};

namespace Natives
{
	// native Map:map_new(bool:ordered=false);
	AMX_DEFINE_NATIVE_TAG(map_new, 0, map)
	{
		bool ordered = optparam(1, 0);
		return map_pool.get_id(map_pool.emplace(ordered));
	}

	// native Map:map_new_args(key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), AnyTag:arg0, AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell) - 2;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 3, numargs + 2);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[3 + arg], value = params[3 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(amx, *key_addr, params[1]), dyn_object(amx, *value_addr, params[2]));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_args_str(key_tag_id=tagof(arg0), AnyTag:arg0, arg1[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args_str, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
			}
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(dyn_object(amx, *key_addr, params[1]), dyn_object(value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_args_var(key_tag_id=tagof(arg0), AnyTag:arg0, VariantTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args_var, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(amx, *key_addr, params[1]), variants::get(*value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_args_packed(ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args_packed, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[1 + arg]);
			value = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), dyn_object(amx, value[0], value[1]));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_args_var_packed({ArgTag,ConstVariantTags}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args_var_packed, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[1 + arg]);
			value = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), variants::get(*value));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_args_str_packed({ArgTag,_}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_args_str_packed, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[1 + arg]);
			value = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), dyn_object(value));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_str_args(value_tag_id=tagof(arg1), arg0[], AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_str_args, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			if(arg == 0)
			{
				value_addr = &value;
			}else{
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(key_addr), dyn_object(amx, *value_addr, params[1]));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_str_args_str(arg0[], arg1[], ...);
	AMX_DEFINE_NATIVE_TAG(map_new_str_args_str, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[1 + arg], value = params[1 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(dyn_object(key_addr), dyn_object(value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_str_args_var(arg0[], VariantTag:arg1, {_,VariantTags}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_str_args_var, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[1 + arg], value = params[1 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			if(arg == 0)
			{
				value_addr = &value;
			}else{
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(key_addr), variants::get(*value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_str_args_packed({_,ArgTag}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_str_args_packed, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[1 + arg]);
			value = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->insert(dyn_object(key), dyn_object(amx, value[0], value[1]));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_var_args(value_tag_id=tagof(arg1), VariantTag:arg0, AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_var_args, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(variants::get(*key_addr), dyn_object(amx, *value_addr, params[1]));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_var_args_str(VariantTag:arg0, arg1[], {_,VariantTags}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_var_args_str, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[1 + arg], value = params[1 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
			}
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(variants::get(*key_addr), dyn_object(value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_var_args_var(VariantTag:arg0, VariantTag:arg1, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(map_new_var_args_var, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[1 + arg], value = params[1 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(variants::get(*key_addr), variants::get(*value_addr));
		}
		return map_pool.get_id(ptr);
	}

	// native Map:map_new_var_args_packed({_,ArgTag}:...);
	AMX_DEFINE_NATIVE_TAG(map_new_var_args_packed, 0, map)
	{
		auto ptr = map_pool.add();
		cell numargs = params[0] / sizeof(cell);
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 1, numargs);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[1 + arg]);
			value = amx_GetAddrSafe(amx, params[2 + arg]);
			ptr->insert(variants::get(*key), dyn_object(amx, value[0], value[1]));
		}
		return map_pool.get_id(ptr);
	}

	// native bool:map_valid(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_valid, 1, bool)
	{
		map_t *ptr;
		return map_pool.get_by_id(params[1], ptr);
	}

	// native map_delete(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_delete, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		map_t old(ptr->ordered());
		ptr->swap(old);
		return map_pool.remove(ptr);
	}

	// native map_delete_deep(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_delete_deep, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		map_t old(ptr->ordered());
		ptr->swap(old);
		map_pool.remove(ptr);
		for(auto &pair : old)
		{
			pair.first.release();
			pair.second.release();
		}
		return 1;
	}

	// native Map:map_clone(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_clone, 1, map)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		auto m = map_pool.add();
		m->set_ordered(ptr->ordered());
		for(auto &&pair : *ptr)
		{
			m->insert(pair.first.clone(), pair.second.clone());
		}
		return map_pool.get_id(m);
	}

	// native map_size(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_size, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		return static_cast<cell>(ptr->size());
	}

	// native map_capacity(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_capacity, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		return static_cast<cell>(ptr->capacity());
	}

	// native map_reserve(Map:map, capacity);
	AMX_DEFINE_NATIVE_TAG(map_reserve, 2, cell)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "capacity");
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		ucell size = params[2];
		ptr->reserve(size);
		return 1;
	}

	// native map_clear(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_clear, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		map_t(ptr->ordered()).swap(*ptr);
		return 1;
	}

	// native map_clear_deep(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_clear_deep, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		map_t old(ptr->ordered());
		ptr->swap(old);
		for(auto &pair : old)
		{
			pair.first.release();
			pair.second.release();
		}
		return 1;
	}

	// native map_set_ordered(Map:map, bool:ordered);
	AMX_DEFINE_NATIVE_TAG(map_set_ordered, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		ptr->set_ordered(params[2]);
		return 1;
	}

	// native bool:map_is_ordered(Map:map);
	AMX_DEFINE_NATIVE_TAG(map_is_ordered, 1, bool)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		return ptr->ordered();
	}

	// native bool:map_add(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_add, 5, bool)
	{
		return key_at<2, 4>::value_at<3, 5>::map_add<dyn_func, dyn_func>(amx, params);
	}

	// native bool:map_add_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_add_arr, 6, bool)
	{
		return key_at<2, 5>::value_at<3, 4, 6>::map_add<dyn_func, dyn_func_arr>(amx, params);
	}

	// native bool:map_add_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_add_str, 4, bool)
	{
		return key_at<2, 4>::value_at<3>::map_add<dyn_func, dyn_func_str>(amx, params);
	}

	// native bool:map_add_var(Map:map, AnyTag:key, VariantTag:value, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_add_var, 4, bool)
	{
		return key_at<2, 4>::value_at<3>::map_add<dyn_func, dyn_func_var>(amx, params);
	}

	// native bool:map_arr_add(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_add, 6, bool)
	{
		return key_at<2, 4, 5>::value_at<3, 6>::map_add<dyn_func_arr, dyn_func>(amx, params);
	}

	// native bool:map_arr_add_arr(Map:map, const AnyTag:key[], const AnyTag:value[], key_size=sizeof(key), value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_add_arr, 7, bool)
	{
		return key_at<2, 4, 6>::value_at<3, 5, 7>::map_add<dyn_func_arr, dyn_func_arr>(amx, params);
	}

	// native bool:map_arr_add_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_add_str, 5, bool)
	{
		return key_at<2, 4, 5>::value_at<3>::map_add<dyn_func_arr, dyn_func_str>(amx, params);
	}

	// native bool:map_arr_add_var(Map:map, const AnyTag:key[], VariantTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_add_var, 5, bool)
	{
		return key_at<2, 4, 5>::value_at<3>::map_add<dyn_func_arr, dyn_func_var>(amx, params);
	}

	// native bool:map_str_add(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_add, 4, bool)
	{
		return key_at<2>::value_at<3, 4>::map_add<dyn_func_str, dyn_func>(amx, params);
	}

	// native bool:map_str_add_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_add_arr, 5, bool)
	{
		return key_at<2>::value_at<3, 4, 5>::map_add<dyn_func_str, dyn_func_arr>(amx, params);
	}

	// native bool:map_str_add_str(Map:map, const key[], const value[]);
	AMX_DEFINE_NATIVE_TAG(map_str_add_str, 3, bool)
	{
		return key_at<2>::value_at<3>::map_add<dyn_func_str, dyn_func_str>(amx, params);
	}

	// native bool:map_str_add_var(Map:map, const key[], VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(map_str_add_var, 3, bool)
	{
		return key_at<2>::value_at<3>::map_add<dyn_func_str, dyn_func_var>(amx, params);
	}

	// native bool:map_var_add(Map:map, VariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_add, 4, bool)
	{
		return key_at<2>::value_at<3, 4>::map_add<dyn_func_var, dyn_func>(amx, params);
	}

	// native bool:map_var_add_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_add_arr, 5, bool)
	{
		return key_at<2>::value_at<3, 4, 5>::map_add<dyn_func_var, dyn_func_arr>(amx, params);
	}

	// native bool:map_var_add_str(Map:map, VariantTag:key, const value[]);
	AMX_DEFINE_NATIVE_TAG(map_var_add_str, 3, bool)
	{
		return key_at<2>::value_at<3>::map_add<dyn_func_var, dyn_func_str>(amx, params);
	}

	// native bool:map_str_add_var(Map:map, VariantTag:key, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(map_var_add_var, 3, bool)
	{
		return key_at<2>::value_at<3>::map_add<dyn_func_var, dyn_func_var>(amx, params);
	}

	// native map_add_map(Map:map, Map:other, bool:overwrite);
	AMX_DEFINE_NATIVE_TAG(map_add_map, 3, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		map_t *ptr2;
		if(!map_pool.get_by_id(params[1], ptr2)) amx_LogicError(errors::pointer_invalid, "map", params[2]);
		if(params[3])
		{
			for(auto &pair : *ptr2)
			{
				(*ptr)[pair.first] = pair.second;
			}
		}else{
			ptr->insert(ptr2->begin(), ptr2->end());
		}
		return ptr->size() - ptr2->size();
	}

	// native map_add_args(key_tag_id=tagof(arg0), TagTag:value_tag_id=tagof(arg1), Map:map, AnyTag:arg0, AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args, 3, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[3], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 3;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 4, numargs + 3);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[4 + arg], value = params[4 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(amx, *key_addr, params[1]), dyn_object(amx, *value_addr, params[2]));
		}
		return numargs / 2;
	}

	// native map_add_args_str(key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, arg1[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args_str, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 2;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 3, numargs + 2);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[3 + arg], value = params[3 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
			}
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(dyn_object(amx, *key_addr, params[1]), dyn_object(value_addr));
		}
		return numargs / 2;
	}

	// native map_add_args_var(key_tag_id=tagof(arg0), Map:map, AnyTag:arg0, VariantTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args_var, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 2;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 3, numargs + 2);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[3 + arg], value = params[3 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(amx, *key_addr, params[1]), variants::get(*value_addr));
		}
		return numargs / 2;
	}

	// native map_add_args_packed(Map:map, ArgTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args_packed, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[2 + arg]);
			value = amx_GetAddrSafe(amx, params[3 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), dyn_object(amx, value[0], value[1]));
		}
		return numargs / 2;
	}

	// native map_add_args_var_packed(Map:map, {ArgTag,_}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args_var_packed, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[2 + arg]);
			value = amx_GetAddrSafe(amx, params[3 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), variants::get(*value));
		}
		return numargs / 2;
	}

	// native map_add_args_str_packed(Map:map, {ArgTag,_}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_args_str_packed, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[2 + arg]);
			value = amx_GetAddrSafe(amx, params[3 + arg]);
			ptr->insert(dyn_object(amx, key[0], key[1]), dyn_object(value));
		}
		return numargs / 2;
	}

	// native map_add_str_args(value_tag_id=tagof(arg1), Map:map, arg0[], AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_str_args, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 2;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 3, numargs + 2);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[3 + arg], value = params[3 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			if(arg == 0)
			{
				value_addr = &value;
			}else{
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(key_addr), dyn_object(amx, *value_addr, params[1]));
		}
		return numargs / 2;
	}

	// native map_add_str_args_str(Map:map, arg0[], arg1[], ...);
	AMX_DEFINE_NATIVE_TAG(map_add_str_args_str, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(dyn_object(key_addr), dyn_object(value_addr));
		}
		return numargs / 2;
	}

	// native map_add_str_args_var(Map:map, arg0[], VariantTag:arg1, {_,VariantTags}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_str_args_var, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			key_addr = amx_GetAddrSafe(amx, key);
			if(arg == 0)
			{
				value_addr = &value;
			}else{
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(dyn_object(key_addr), variants::get(*value_addr));
		}
		return numargs / 2;
	}
	
	// native map_add_str_args_packed(Map:map, {_,ArgTag}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_str_args_packed, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[2 + arg]);
			value = amx_GetAddrSafe(amx, params[3 + arg]);
			ptr->insert(dyn_object(key), dyn_object(amx, value[0], value[1]));
		}
		return numargs / 2;
	}

	// native map_add_var_args(value_tag_id=tagof(arg1), Map:map, VariantTag:arg0, AnyTag:arg1, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_var_args, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[2], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 2;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 3, numargs + 2);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[3 + arg], value = params[3 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(variants::get(*key_addr), dyn_object(amx, *value_addr, params[1]));
		}
		return numargs / 2;
	}

	// native map_add_var_args_str(Map:map, VariantTag:arg0, arg1[], {_,VariantTags}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_var_args_str, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
			}
			value_addr = amx_GetAddrSafe(amx, value);
			ptr->insert(variants::get(*key_addr), dyn_object(value_addr));
		}
		return numargs / 2;
	}

	// native map_add_var_args_var(Map:map, VariantTag:arg0, VariantTag:arg1, VariantTag:...);
	AMX_DEFINE_NATIVE_TAG(map_add_var_args_var, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell key = params[2 + arg], value = params[2 + arg + 1], *key_addr, *value_addr;
			if(arg == 0)
			{
				key_addr = &key;
				value_addr = &value;
			}else{
				key_addr = amx_GetAddrSafe(amx, key);
				value_addr = amx_GetAddrSafe(amx, value);
			}
			ptr->insert(variants::get(*key_addr), variants::get(*value_addr));
		}
		return numargs / 2;
	}

	// native map_add_var_args_packed(Map:map, {ConstVariantTags,ArgTag}:...);
	AMX_DEFINE_NATIVE_TAG(map_add_var_args_packed, 1, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		cell numargs = params[0] / sizeof(cell) - 1;
		if(numargs % 2 != 0)
		{
			amx_FormalError(errors::not_enough_args, numargs + 2, numargs + 1);
		}
		for(cell arg = 0; arg < numargs; arg += 2)
		{
			cell *key, *value;
			key = amx_GetAddrSafe(amx, params[2 + arg]);
			value = amx_GetAddrSafe(amx, params[3 + arg]);
			ptr->insert(variants::get(*key), dyn_object(amx, value[0], value[1]));
		}
		return numargs / 2;
	}

	// native bool:map_remove(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_remove, 3, bool)
	{
		return key_at<2, 3>::map_remove<dyn_func>(amx, params);
	}

	// native bool:map_arr_remove(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_remove, 4, bool)
	{
		return key_at<2, 3, 4>::map_remove<dyn_func_arr>(amx, params);
	}

	// native bool:map_str_remove(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_remove, 2, bool)
	{
		return key_at<2>::map_remove<dyn_func_str>(amx, params);
	}

	// native bool:map_str_remove(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_remove, 2, bool)
	{
		return key_at<2>::map_remove<dyn_func_var>(amx, params);
	}

	// native bool:map_remove_deep(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_remove_deep, 3, bool)
	{
		return key_at<2, 3>::map_remove_deep<dyn_func>(amx, params);
	}

	// native bool:map_arr_remove_deep(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_remove_deep, 4, bool)
	{
		return key_at<2, 3, 4>::map_remove_deep<dyn_func_arr>(amx, params);
	}

	// native bool:map_str_remove_deep(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_remove_deep, 2, bool)
	{
		return key_at<2>::map_remove_deep<dyn_func_str>(amx, params);
	}

	// native bool:map_str_remove_deep(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_remove_deep, 2, bool)
	{
		return key_at<2>::map_remove_deep<dyn_func_var>(amx, params);
	}

	// native map_remove_if(Map:map, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(map_remove_if, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
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
				args.push_back(std::cref(it->second));
				args.push_back(std::cref(it->first));
			}else{
				args[0] = std::cref(it->second);
				args[1] = std::cref(it->first);
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

	// native map_remove_if_deep(Map:map, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(map_remove_if_deep, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
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
				args.push_back(std::cref(it->second));
				args.push_back(std::cref(it->first));
			}else{
				args[0] = std::cref(it->second);
				args[1] = std::cref(it->first);
			}
			if(expr->execute_bool(args, info))
			{
				it->first.release();
				it->second.release();
				it = ptr->erase(it);
				count++;
			}else{
				++it;
			}
		}
		return count;
	}

	// native bool:map_has_key(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_has_key, 3, bool)
	{
		return key_at<2, 3>::map_has_key<dyn_func>(amx, params);
	}

	// native bool:map_has_arr_key(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_has_arr_key, 4, bool)
	{
		return key_at<2, 3, 4>::map_has_key<dyn_func_arr>(amx, params);
	}

	// native bool:map_has_str_key(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_has_str_key, 2, bool)
	{
		return key_at<2>::map_has_key<dyn_func_str>(amx, params);
	}

	// native bool:map_has_var_key(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_has_var_key, 2, bool)
	{
		return key_at<2>::map_has_key<dyn_func_var>(amx, params);
	}

	// native map_get(Map:map, AnyTag:key, offset=0, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_get, 4)
	{
		return key_at<2, 4>::value_at<3>::map_get<dyn_func, dyn_func>(amx, params);
	}

	// native map_get_arr(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_get_arr, 5, cell)
	{
		return key_at<2, 5>::value_at<3, 4>::map_get<dyn_func, dyn_func_arr>(amx, params);
	}

	// native String:map_get_str_s(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_get_str_s, 3, string)
	{
		return key_at<2, 3>::value_at<>::map_get<dyn_func, dyn_func_str_s>(amx, params);
	}

	// native Variant:map_get_var(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_get_var, 3, variant)
	{
		return key_at<2, 3>::value_at<>::map_get<dyn_func, dyn_func_var>(amx, params);
	}

	// native bool:map_get_safe(Map:map, AnyTag:key, &AnyTag:value, offset=0, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_get_safe, 6, bool)
	{
		return key_at<2, 5>::value_at<3, 4, 6>::map_get<dyn_func, dyn_func>(amx, params);
	}

	// native map_get_arr_safe(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_get_arr_safe, 6, cell)
	{
		return key_at<2, 5>::value_at<3, 4, 6>::map_get<dyn_func, dyn_func_arr>(amx, params);
	}

	// native map_get_str_safe(Map:map, AnyTag:key, value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_get_str_safe, 5, cell)
	{
		return key_at<2, 5>::value_at<3, 4>::map_get<dyn_func, dyn_func_str>(amx, params);
	}

	// native String:map_get_str_safe_s(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_get_str_safe_s, 3, string)
	{
		return key_at<2, 3>::value_at<0>::map_get<dyn_func, dyn_func_str_s>(amx, params);
	}

	// native map_arr_get(Map:map, const AnyTag:key[], offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_arr_get, 5)
	{
		return key_at<2, 4, 5>::value_at<3>::map_get<dyn_func_arr, dyn_func>(amx, params);
	}

	// native map_arr_get_arr(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_arr, 6, cell)
	{
		return key_at<2, 5, 6>::value_at<3, 4>::map_get<dyn_func_arr, dyn_func_arr>(amx, params);
	}

	// native String:map_arr_get_str_s(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_str_s, 4, string)
	{
		return key_at<2, 3, 4>::value_at<>::map_get<dyn_func_arr, dyn_func_str_s>(amx, params);
	}

	// native Variant:map_arr_get_var(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_var, 4, variant)
	{
		return key_at<2, 3, 4>::value_at<>::map_get<dyn_func_arr, dyn_func_var>(amx, params);
	}

	// native bool:map_arr_get_safe(Map:map, const AnyTag:key[], &AnyTag:value, offset=0, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_safe, 7, bool)
	{
		return key_at<2, 5, 6>::value_at<3, 4, 7>::map_get<dyn_func_arr, dyn_func>(amx, params);
	}

	// native map_arr_get_arr_safe(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_arr_safe, 7, cell)
	{
		return key_at<2, 5, 6>::value_at<3, 4, 7>::map_get<dyn_func_arr, dyn_func_arr>(amx, params);
	}

	// native map_arr_get_str_safe(Map:map, const AnyTag:key[], value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_str_safe, 6, cell)
	{
		return key_at<2, 5, 6>::value_at<3, 4>::map_get<dyn_func_arr, dyn_func_str>(amx, params);
	}

	// native String:map_arr_get_str_safe_s(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_get_str_safe_s, 4, string)
	{
		return key_at<2, 3, 4>::value_at<0>::map_get<dyn_func_arr, dyn_func_str_s>(amx, params);
	}

	// native map_str_get(Map:map, const key[], offset=0);
	AMX_DEFINE_NATIVE(map_str_get, 3)
	{
		return key_at<2>::value_at<3>::map_get<dyn_func_str, dyn_func>(amx, params);
	}

	// native map_str_get_arr(Map:map, const key[], AnyTag:value[], value_size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_get_arr, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_get<dyn_func_str, dyn_func_arr>(amx, params);
	}

	// native String:map_str_get_str_s(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_get_str_s, 2, string)
	{
		return key_at<2>::value_at<>::map_get<dyn_func_str, dyn_func_str_s>(amx, params);
	}

	// native Variant:map_str_get_var(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_get_var, 2, variant)
	{
		return key_at<2>::value_at<>::map_get<dyn_func_str, dyn_func_var>(amx, params);
	}

	// native bool:map_str_get_safe(Map:map, const key[], &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_get_safe, 5, bool)
	{
		return key_at<2>::value_at<3, 4, 5>::map_get<dyn_func_str, dyn_func>(amx, params);
	}

	// native map_str_get_arr_safe(Map:map, const key[], AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_get_arr_safe, 5, cell)
	{
		return key_at<2>::value_at<3, 4, 5>::map_get<dyn_func_str, dyn_func_arr>(amx, params);
	}

	// native map_str_get_str_safe(Map:map, const key[], value[], value_size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_get_str_safe, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_get<dyn_func_str, dyn_func_str>(amx, params);
	}

	// native String:map_str_get_str_safe_s(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_get_str_safe_s, 2, string)
	{
		return key_at<2>::value_at<0>::map_get<dyn_func_str, dyn_func_str_s>(amx, params);
	}

	// native map_var_get(Map:map, VariantTag:key, offset=0);
	AMX_DEFINE_NATIVE(map_var_get, 3)
	{
		return key_at<2>::value_at<3>::map_get<dyn_func_var, dyn_func>(amx, params);
	}

	// native map_var_get_arr(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_get_arr, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_get<dyn_func_var, dyn_func_arr>(amx, params);
	}

	// native String:map_var_get_str_s(Map:map, ConstVariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_get_str_s, 2, string)
	{
		return key_at<2>::value_at<>::map_get<dyn_func_var, dyn_func_str_s>(amx, params);
	}

	// native Variant:map_var_get_var(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_get_var, 2, variant)
	{
		return key_at<2>::value_at<>::map_get<dyn_func_var, dyn_func_var>(amx, params);
	}

	// native bool:map_var_get_safe(Map:map, VariantTag:key, &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_get_safe, 5, bool)
	{
		return key_at<2>::value_at<3, 4, 5>::map_get<dyn_func_var, dyn_func>(amx, params);
	}

	// native map_var_get_arr_safe(Map:map, VariantTag:key, AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_get_arr_safe, 5, cell)
	{
		return key_at<2>::value_at<3, 4, 5>::map_get<dyn_func_var, dyn_func_arr>(amx, params);
	}

	// native map_var_get_str_safe(Map:map, ConstVariantTag:key, value[], value_size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_get_str_safe, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_get<dyn_func_var, dyn_func_str>(amx, params);
	}

	// native String:map_var_get_str_safe_s(Map:map, ConstVariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_get_str_safe_s, 2, string)
	{
		return key_at<2>::value_at<0>::map_get<dyn_func_var, dyn_func_str_s>(amx, params);
	}

	// native map_set(Map:map, AnyTag:key, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_set, 5, cell)
	{
		return key_at<2, 4>::value_at<3, 5>::map_set<dyn_func, dyn_func>(amx, params);
	}

	// native map_set_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_set_arr, 6, cell)
	{
		return key_at<2, 5>::value_at<3, 4, 6>::map_set<dyn_func, dyn_func_arr>(amx, params);
	}

	// native map_set_str(Map:map, AnyTag:key, const value[], TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_set_str, 4, cell)
	{
		return key_at<2, 4>::value_at<3>::map_set<dyn_func, dyn_func_str>(amx, params);
	}

	// native map_set_var(Map:map, AnyTag:key, VariantTag:value, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_set_var, 4, cell)
	{
		return key_at<2, 4>::value_at<3>::map_set<dyn_func, dyn_func_var>(amx, params);
	}

	// native map_set_cell(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_set_cell, 5, cell)
	{
		return key_at<2, 5>::map_set_cell<dyn_func>(amx, params);
	}

	// native bool:map_set_cell_safe(Map:map, AnyTag:key, offset, AnyTag:value, TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_set_cell_safe, 5, bool)
	{
		return key_at<2, 5>::map_set_cell<dyn_func, 6>(amx, params);
	}

	// native map_arr_set(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_set, 5, cell)
	{
		return key_at<2, 4, 5>::value_at<3, 6>::map_set<dyn_func_arr, dyn_func>(amx, params);
	}

	// native map_arr_set_arr(Map:map, const AnyTag:key[], const AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_set_arr, 7, cell)
	{
		return key_at<2, 5, 6>::value_at<3, 4, 7>::map_set<dyn_func_arr, dyn_func_arr>(amx, params);
	}

	// native map_arr_set_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_set_str, 5, cell)
	{
		return key_at<2, 4, 5>::value_at<3>::map_set<dyn_func_arr, dyn_func_str>(amx, params);
	}

	// native map_arr_set_var(Map:map, const AnyTag:key[], VariantTags:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_set_var, 5, cell)
	{
		return key_at<2, 4, 5>::value_at<3>::map_set<dyn_func_arr, dyn_func_var>(amx, params);
	}

	// native map_arr_set_cell(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_set_cell, 6, cell)
	{
		return key_at<2, 5, 6>::map_set_cell<dyn_func_arr>(amx, params);
	}

	// native bool:map_arr_set_cell_safe(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), TagTag:key_tag_id=tagof(key), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_set_cell_safe, 6, bool)
	{
		return key_at<2, 5, 6>::map_set_cell<dyn_func_arr, 7>(amx, params);
	}

	// native map_str_set(Map:map, const key[], AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_set, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_set<dyn_func_str, dyn_func>(amx, params);
	}

	// native map_str_set_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_set_arr, 5, cell)
	{
		return key_at<2>::value_at<3, 4, 5>::map_set<dyn_func_str, dyn_func_arr>(amx, params);
	}

	// native map_str_set_str(Map:map, const key[], const value[]);
	AMX_DEFINE_NATIVE_TAG(map_str_set_str, 3, cell)
	{
		return key_at<2>::value_at<3>::map_set<dyn_func_str, dyn_func_str>(amx, params);
	}

	// native map_str_set_var(Map:map, const key[], VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(map_str_set_var, 3, cell)
	{
		return key_at<2>::value_at<3>::map_set<dyn_func_str, dyn_func_var>(amx, params);
	}

	// native map_str_set_cell(Map:map, const key[], offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(map_str_set_cell, 4, cell)
	{
		return key_at<2>::map_set_cell<dyn_func_str>(amx, params);
	}

	// native bool:map_str_set_cell_safe(Map:map, const key[], offset, AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_str_set_cell_safe, 5, bool)
	{
		return key_at<2>::map_set_cell<dyn_func_str, 5>(amx, params);
	}

	// native map_var_set(Map:map, VariantTag:key, AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_set, 4, cell)
	{
		return key_at<2>::value_at<3, 4>::map_set<dyn_func_var, dyn_func>(amx, params);
	}

	// native map_var_set_arr(Map:map, VariantTag:key, const AnyTag:value[], value_size=sizeof(value), TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_set_arr, 5, cell)
	{
		return key_at<2>::value_at<3, 4, 5>::map_set<dyn_func_var, dyn_func_arr>(amx, params);
	}

	// native map_var_set_str(Map:map, VariantTag:key, const value[]);
	AMX_DEFINE_NATIVE_TAG(map_var_set_str, 3, cell)
	{
		return key_at<2>::value_at<3>::map_set<dyn_func_var, dyn_func_str>(amx, params);
	}

	// native map_var_set_var(Map:map, VariantTag:key, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(map_var_set_var, 3, cell)
	{
		return key_at<2>::value_at<3>::map_set<dyn_func_var, dyn_func_var>(amx, params);
	}

	// native map_var_set_cell(Map:map, VariantTag:key, offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(map_var_set_cell, 4, cell)
	{
		return key_at<2>::map_set_cell<dyn_func_var>(amx, params);
	}

	// native bool:map_var_set_cell_safe(Map:map, VariantTag:key, offset, AnyTag:value, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_var_set_cell_safe, 5, bool)
	{
		return key_at<2>::map_set_cell<dyn_func_var, 5>(amx, params);
	}

	// native map_key_at(Map:map, index, offset=0);
	AMX_DEFINE_NATIVE(map_key_at, 3)
	{
		return value_at<3>::map_key_at<dyn_func>(amx, params);
	}

	// native map_arr_key_at(Map:map, index, AnyTag:key[], key_size=sizeof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_key_at, 4, cell)
	{
		return value_at<3, 4>::map_key_at<dyn_func_arr>(amx, params);
	}

	// native Variant:map_var_key_at(Map:map, index);
	AMX_DEFINE_NATIVE_TAG(map_var_key_at, 2, variant)
	{
		return value_at<>::map_key_at<dyn_func_var>(amx, params);
	}

	// native bool:map_key_at_safe(Map:map, index, &AnyTag:key, offset=0, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_key_at_safe, 5, bool)
	{
		return value_at<3, 4, 5>::map_key_at<dyn_func>(amx, params);
	}

	// native map_arr_key_at_safe(Map:map, index, AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_key_at_safe, 5, cell)
	{
		return value_at<3, 4, 5>::map_key_at<dyn_func_arr>(amx, params);
	}

	// native map_value_at(Map:map, index, offset=0);
	AMX_DEFINE_NATIVE(map_value_at, 3)
	{
		return value_at<3>::map_value_at<dyn_func>(amx, params);
	}

	// native map_arr_value_at(Map:map, index, AnyTag:value[], value_size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_value_at, 4, cell)
	{
		return value_at<3, 4>::map_value_at<dyn_func_arr>(amx, params);
	}

	// native Variant:map_var_value_at(Map:map, index);
	AMX_DEFINE_NATIVE_TAG(map_var_value_at, 2, variant)
	{
		return value_at<>::map_value_at<dyn_func_var>(amx, params);
	}

	// native bool:map_value_at_safe(Map:map, index, &AnyTag:value, offset=0, TagTag:value_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_value_at_safe, 5, bool)
	{
		return value_at<3, 4, 5>::map_value_at<dyn_func>(amx, params);
	}

	// native map_arr_value_at_safe(Map:map, index, AnyTag:value[], value_size=sizeof(value), TagTag:key_tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_arr_value_at_safe, 5, cell)
	{
		return value_at<3, 4, 5>::map_value_at<dyn_func_arr>(amx, params);
	}

	// native map_count(Map:map, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_count, 3, cell)
	{
		return value_at<2, 3>::map_count<dyn_func>(amx, params);
	}

	// native map_count_arr(Map:map, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(map_count_arr, 4, cell)
	{
		return value_at<2, 3, 4>::map_count<dyn_func_arr>(amx, params);
	}

	// native map_count_str(Map:map, const value[]);
	AMX_DEFINE_NATIVE_TAG(map_count_str, 2, cell)
	{
		return value_at<2>::map_count<dyn_func_str>(amx, params);
	}

	// native map_count_var(Map:map, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(map_count_var, 2, cell)
	{
		return value_at<2>::map_count<dyn_func_var>(amx, params);
	}

	// native map_count_if(Map:map, Expression:pred);
	AMX_DEFINE_NATIVE_TAG(map_count_if, 2, cell)
	{
		map_t *ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		expression *expr;
		if(!expression_pool.get_by_id(params[2], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		
		expression::args_type args;
		expression::exec_info info(amx);

		cell count = 0;
		for(auto it = ptr->begin(); it != ptr->end(); ++it)
		{
			if(args.size() == 0)
			{
				args.push_back(std::cref(it->second));
				args.push_back(std::cref(it->first));
			}else{
				args[0] = std::cref(it->second);
				args[1] = std::cref(it->first);
			}
			if(expr->execute_bool(args, info))
			{
				count++;
			}
		}
		return count;
	}

	// native map_tagof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_tagof, 3, cell)
	{
		return key_at<2, 3>::map_tagof<dyn_func>(amx, params);
	}

	// native map_sizeof(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_sizeof, 3, cell)
	{
		return key_at<2, 3>::map_sizeof<dyn_func>(amx, params);
	}

	// native map_arr_tagof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_tagof, 4, cell)
	{
		return key_at<2, 3, 4>::map_tagof<dyn_func_arr>(amx, params);
	}

	// native map_arr_sizeof(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE_TAG(map_arr_sizeof, 4, cell)
	{
		return key_at<2, 3, 4>::map_sizeof<dyn_func_arr>(amx, params);
	}

	// native map_str_tagof(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_tagof, 2, cell)
	{
		return key_at<2>::map_tagof<dyn_func_str>(amx, params);
	}

	// native map_str_sizeof(Map:map, const key[]);
	AMX_DEFINE_NATIVE_TAG(map_str_sizeof, 2, cell)
	{
		return key_at<2>::map_sizeof<dyn_func_str>(amx, params);
	}

	// native map_var_tagof(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_tagof, 2, cell)
	{
		return key_at<2>::map_tagof<dyn_func_var>(amx, params);
	}

	// native map_var_sizeof(Map:map, VariantTag:key);
	AMX_DEFINE_NATIVE_TAG(map_var_sizeof, 2, cell)
	{
		return key_at<2>::map_sizeof<dyn_func_var>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(map_new),
	AMX_DECLARE_NATIVE(map_new_args),
	AMX_DECLARE_NATIVE(map_new_args_str),
	AMX_DECLARE_NATIVE(map_new_args_var),
	AMX_DECLARE_NATIVE(map_new_args_packed),
	AMX_DECLARE_NATIVE(map_new_args_str_packed),
	AMX_DECLARE_NATIVE(map_new_args_var_packed),
	AMX_DECLARE_NATIVE(map_new_str_args),
	AMX_DECLARE_NATIVE(map_new_str_args_str),
	AMX_DECLARE_NATIVE(map_new_str_args_var),
	AMX_DECLARE_NATIVE(map_new_str_args_packed),
	AMX_DECLARE_NATIVE(map_new_var_args),
	AMX_DECLARE_NATIVE(map_new_var_args_str),
	AMX_DECLARE_NATIVE(map_new_var_args_var),
	AMX_DECLARE_NATIVE(map_new_var_args_packed),

	AMX_DECLARE_NATIVE(map_valid),
	AMX_DECLARE_NATIVE(map_delete),
	AMX_DECLARE_NATIVE(map_delete_deep),
	AMX_DECLARE_NATIVE(map_clone),
	AMX_DECLARE_NATIVE(map_size),
	AMX_DECLARE_NATIVE(map_capacity),
	AMX_DECLARE_NATIVE(map_reserve),
	AMX_DECLARE_NATIVE(map_clear),
	AMX_DECLARE_NATIVE(map_clear_deep),
	AMX_DECLARE_NATIVE(map_set_ordered),
	AMX_DECLARE_NATIVE(map_is_ordered),

	AMX_DECLARE_NATIVE(map_add),
	AMX_DECLARE_NATIVE(map_add_arr),
	AMX_DECLARE_NATIVE(map_add_str),
	AMX_DECLARE_NATIVE(map_add_var),
	AMX_DECLARE_NATIVE(map_arr_add),
	AMX_DECLARE_NATIVE(map_arr_add_arr),
	AMX_DECLARE_NATIVE(map_arr_add_str),
	AMX_DECLARE_NATIVE(map_arr_add_var),
	AMX_DECLARE_NATIVE(map_str_add),
	AMX_DECLARE_NATIVE(map_str_add_arr),
	AMX_DECLARE_NATIVE(map_str_add_str),
	AMX_DECLARE_NATIVE(map_str_add_var),
	AMX_DECLARE_NATIVE(map_var_add),
	AMX_DECLARE_NATIVE(map_var_add_arr),
	AMX_DECLARE_NATIVE(map_var_add_str),
	AMX_DECLARE_NATIVE(map_var_add_var),
	AMX_DECLARE_NATIVE(map_add_map),
	AMX_DECLARE_NATIVE(map_add_args),
	AMX_DECLARE_NATIVE(map_add_args_str),
	AMX_DECLARE_NATIVE(map_add_args_var),
	AMX_DECLARE_NATIVE(map_add_args_packed),
	AMX_DECLARE_NATIVE(map_add_str_args),
	AMX_DECLARE_NATIVE(map_add_str_args_str),
	AMX_DECLARE_NATIVE(map_add_str_args_var),
	AMX_DECLARE_NATIVE(map_add_str_args_packed),
	AMX_DECLARE_NATIVE(map_add_var_args),
	AMX_DECLARE_NATIVE(map_add_var_args_str),
	AMX_DECLARE_NATIVE(map_add_var_args_var),
	AMX_DECLARE_NATIVE(map_add_var_args_packed),

	AMX_DECLARE_NATIVE(map_remove),
	AMX_DECLARE_NATIVE(map_arr_remove),
	AMX_DECLARE_NATIVE(map_str_remove),
	AMX_DECLARE_NATIVE(map_var_remove),
	AMX_DECLARE_NATIVE(map_remove_deep),
	AMX_DECLARE_NATIVE(map_arr_remove_deep),
	AMX_DECLARE_NATIVE(map_str_remove_deep),
	AMX_DECLARE_NATIVE(map_var_remove_deep),
	AMX_DECLARE_NATIVE(map_remove_if),
	AMX_DECLARE_NATIVE(map_remove_if_deep),

	AMX_DECLARE_NATIVE(map_has_key),
	AMX_DECLARE_NATIVE(map_has_arr_key),
	AMX_DECLARE_NATIVE(map_has_str_key),
	AMX_DECLARE_NATIVE(map_has_var_key),

	AMX_DECLARE_NATIVE(map_get),
	AMX_DECLARE_NATIVE(map_get_arr),
	AMX_DECLARE_NATIVE(map_get_str_s),
	AMX_DECLARE_NATIVE(map_get_var),
	AMX_DECLARE_NATIVE(map_get_safe),
	AMX_DECLARE_NATIVE(map_get_arr_safe),
	AMX_DECLARE_NATIVE(map_get_str_safe),
	AMX_DECLARE_NATIVE(map_get_str_safe_s),
	AMX_DECLARE_NATIVE(map_arr_get),
	AMX_DECLARE_NATIVE(map_arr_get_arr),
	AMX_DECLARE_NATIVE(map_arr_get_str_s),
	AMX_DECLARE_NATIVE(map_arr_get_var),
	AMX_DECLARE_NATIVE(map_arr_get_safe),
	AMX_DECLARE_NATIVE(map_arr_get_arr_safe),
	AMX_DECLARE_NATIVE(map_arr_get_str_safe),
	AMX_DECLARE_NATIVE(map_arr_get_str_safe_s),
	AMX_DECLARE_NATIVE(map_str_get),
	AMX_DECLARE_NATIVE(map_str_get_arr),
	AMX_DECLARE_NATIVE(map_str_get_str_s),
	AMX_DECLARE_NATIVE(map_str_get_var),
	AMX_DECLARE_NATIVE(map_str_get_safe),
	AMX_DECLARE_NATIVE(map_str_get_arr_safe),
	AMX_DECLARE_NATIVE(map_str_get_str_safe),
	AMX_DECLARE_NATIVE(map_str_get_str_safe_s),
	AMX_DECLARE_NATIVE(map_var_get),
	AMX_DECLARE_NATIVE(map_var_get_arr),
	AMX_DECLARE_NATIVE(map_var_get_str_s),
	AMX_DECLARE_NATIVE(map_var_get_var),
	AMX_DECLARE_NATIVE(map_var_get_safe),
	AMX_DECLARE_NATIVE(map_var_get_arr_safe),
	AMX_DECLARE_NATIVE(map_var_get_str_safe),
	AMX_DECLARE_NATIVE(map_var_get_str_safe_s),

	AMX_DECLARE_NATIVE(map_set),
	AMX_DECLARE_NATIVE(map_set_arr),
	AMX_DECLARE_NATIVE(map_set_str),
	AMX_DECLARE_NATIVE(map_set_var),
	AMX_DECLARE_NATIVE(map_set_cell),
	AMX_DECLARE_NATIVE(map_set_cell_safe),
	AMX_DECLARE_NATIVE(map_arr_set),
	AMX_DECLARE_NATIVE(map_arr_set_arr),
	AMX_DECLARE_NATIVE(map_arr_set_str),
	AMX_DECLARE_NATIVE(map_arr_set_var),
	AMX_DECLARE_NATIVE(map_arr_set_cell),
	AMX_DECLARE_NATIVE(map_arr_set_cell_safe),
	AMX_DECLARE_NATIVE(map_str_set),
	AMX_DECLARE_NATIVE(map_str_set_arr),
	AMX_DECLARE_NATIVE(map_str_set_str),
	AMX_DECLARE_NATIVE(map_str_set_var),
	AMX_DECLARE_NATIVE(map_str_set_cell),
	AMX_DECLARE_NATIVE(map_str_set_cell_safe),
	AMX_DECLARE_NATIVE(map_var_set),
	AMX_DECLARE_NATIVE(map_var_set_arr),
	AMX_DECLARE_NATIVE(map_var_set_str),
	AMX_DECLARE_NATIVE(map_var_set_var),
	AMX_DECLARE_NATIVE(map_var_set_cell),
	AMX_DECLARE_NATIVE(map_var_set_cell_safe),
	AMX_DECLARE_NATIVE(map_key_at),
	AMX_DECLARE_NATIVE(map_arr_key_at),
	AMX_DECLARE_NATIVE(map_var_key_at),
	AMX_DECLARE_NATIVE(map_key_at_safe),
	AMX_DECLARE_NATIVE(map_arr_key_at_safe),
	AMX_DECLARE_NATIVE(map_value_at),
	AMX_DECLARE_NATIVE(map_arr_value_at),
	AMX_DECLARE_NATIVE(map_var_value_at),
	AMX_DECLARE_NATIVE(map_value_at_safe),
	AMX_DECLARE_NATIVE(map_arr_value_at_safe),

	AMX_DECLARE_NATIVE(map_count),
	AMX_DECLARE_NATIVE(map_count_arr),
	AMX_DECLARE_NATIVE(map_count_str),
	AMX_DECLARE_NATIVE(map_count_var),
	AMX_DECLARE_NATIVE(map_count_if),

	AMX_DECLARE_NATIVE(map_tagof),
	AMX_DECLARE_NATIVE(map_sizeof),
	AMX_DECLARE_NATIVE(map_arr_tagof),
	AMX_DECLARE_NATIVE(map_arr_sizeof),
	AMX_DECLARE_NATIVE(map_str_tagof),
	AMX_DECLARE_NATIVE(map_str_sizeof),
	AMX_DECLARE_NATIVE(map_var_tagof),
	AMX_DECLARE_NATIVE(map_var_sizeof),
};

int RegisterMapNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
