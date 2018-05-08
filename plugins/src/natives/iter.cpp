#include "natives.h"
#include "modules/variants.h"
#include "objects/dyn_object.h"
#include <vector>
#include <unordered_map>
//#include <unordered_set>
//#include <deque>
#include <type_traits>

#ifndef _DEBUG
#ifdef _WIN32
template <class Type>
struct is_simple
{
	static constexpr const bool value = sizeof(Type) == sizeof(cell) && std::is_trivially_assignable<Type, Type>::value && std::is_trivially_destructible<Type>::value;
};
#else
template <class Type>
struct is_simple
{
	static constexpr const bool value = sizeof(Type) == sizeof(cell);
};
#endif

// Iterators fit inside a single cell, but not in debug in VC++.
static_assert(is_simple<std::vector<dyn_object>::iterator>::value, "Vector is not simple.");
static_assert(is_simple<std::unordered_map<dyn_object, dyn_object>::iterator>::value, "Map is not simple.");
//static_assert(is_simple_v<std::unordered_set<dyn_object>::iterator>, "Set is not simple.");
#endif

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native iter_set(ListIterator:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_set(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::vector<dyn_object>::iterator&>(params[1]);
		*iter = Factory(amx, params[Indices]...);
		return 1;
	}

	// native iter_get(ListIterator:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::vector<dyn_object>::iterator&>(params[1]);
		return Factory(amx, *iter, params[Indices]...);
	}

	// native iter_get_key(MapIterator:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get_key(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[1]);
		return Factory(amx, iter->first, params[Indices]...);
	}

	// native iter_set_value(MapIterator:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_set_value(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[1]);
		iter->second = Factory(amx, params[Indices]...);
		return 1;
	}

	// native iter_get_value(MapIterator:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get_value(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[1]);
		return Factory(amx, iter->second, params[Indices]...);
	}
};

// native bool:iter_set_cell(ListIterator:iter, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL iter_set_cell(AMX *amx, cell *params)
{
	if(params[1] == 0 || params[2] < 0) return 0;
	auto &obj = *reinterpret_cast<std::vector<dyn_object>::iterator&>(params[1]);
	if(TagIndex && !obj.check_tag(amx, params[TagIndex])) return 0;
	return obj.set_cell(params[2], params[3]);
}

// native bool:iter_set_value_cell(MapIterator:iter, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL iter_set_value_cell(AMX *amx, cell *params)
{
	if(params[1] == 0 || params[2] < 0) return 0;
	auto &obj = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[1])->second;
	if(TagIndex && !obj.check_tag(amx, params[TagIndex])) return 0;
	return obj.set_cell(params[2], params[3]);
}

namespace Natives
{
	// native ListIterator:list_iter(List:list);
	static cell AMX_NATIVE_CALL list_iter(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto iter = reinterpret_cast<std::vector<dyn_object>*>(params[1])->begin();
		return reinterpret_cast<cell&>(iter);
	}

	// native MapIterator:map_iter(Map:map);
	static cell AMX_NATIVE_CALL map_iter(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>*>(params[1])->begin();
		return reinterpret_cast<cell&>(iter);
	}

	// native bool:list_iter_valid(List:list, ListIterator:iter);
	static cell AMX_NATIVE_CALL list_iter_valid(AMX *amx, cell *params)
	{
		if(params[1] == 0 || params[2] == 0) return 0;
		auto ptr = reinterpret_cast<std::vector<dyn_object>*>(params[1]);
		auto &iter = reinterpret_cast<std::vector<dyn_object>::iterator&>(params[2]);
		return iter != ptr->end();
	}

	// native bool:map_iter_valid(Map:map, MapIterator:iter);
	static cell AMX_NATIVE_CALL map_iter_valid(AMX *amx, cell *params)
	{
		if(params[1] == 0 || params[2] == 0) return 0;
		auto ptr = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>*>(params[1]);
		auto &iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[2]);
		return iter != ptr->end();
	}

	// native ListIterator:list_next(ListIterator:iter);
	static cell AMX_NATIVE_CALL list_next(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::vector<dyn_object>::iterator&>(params[1]);
		iter++;
		return reinterpret_cast<cell&>(iter);
	}

	// native MapIterator:map_next(MapIterator:iter);
	static cell AMX_NATIVE_CALL map_next(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;
		auto &iter = reinterpret_cast<std::unordered_map<dyn_object, dyn_object>::iterator&>(params[1]);
		iter++;
		return reinterpret_cast<cell&>(iter);
	}

	// native iter_get(ListIterator:iter, offset=0);
	static cell AMX_NATIVE_CALL iter_get(AMX *amx, cell *params)
	{
		return value_at<2>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr(ListIterator:iter, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL iter_get_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_get<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_var(ListIterator:iter);
	static cell AMX_NATIVE_CALL iter_get_var(AMX *amx, cell *params)
	{
		return value_at<>::iter_get<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_safe(ListIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr_safe(ListIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set(ListIterator:iter, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_set<dyn_func>(amx, params);
	}

	// native bool:iter_set_arr(ListIterator:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_set<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_str(ListIterator:iter, const value[]);
	static cell AMX_NATIVE_CALL iter_set_str(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set<dyn_func_str>(amx, params);
	}

	// native bool:iter_set_var(ListIterator:iter, VariantTag:value);
	static cell AMX_NATIVE_CALL iter_set_var(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set<dyn_func_var>(amx, params);
	}

	// native bool:iter_set_cell(ListIterator:iter, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL iter_set_cell(AMX *amx, cell *params)
	{
		return ::iter_set_cell(amx, params);
	}

	// native bool:iter_set_cell_safe(ListIterator:iter, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_cell_safe(AMX *amx, cell *params)
	{
		return ::iter_set_cell<4>(amx, params);
	}

	// native iter_get_key(MapIterator:iter, offset=0);
	static cell AMX_NATIVE_CALL iter_get_key(AMX *amx, cell *params)
	{
		return value_at<2>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr(MapIterator:iter, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL iter_get_key_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_key_var(MapIterator:iter);
	static cell AMX_NATIVE_CALL iter_get_key_var(AMX *amx, cell *params)
	{
		return value_at<>::iter_get_key<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_key_safe(MapIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_key_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr_safe(MapIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_key_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native iter_get_value(MapIterator:iter, offset=0);
	static cell AMX_NATIVE_CALL iter_get_value(AMX *amx, cell *params)
	{
		return value_at<2>::iter_get_value<dyn_func>(amx, params);
	}

	// native iter_get_value_arr(MapIterator:iter, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL iter_get_value_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_get_value<dyn_func_arr>(amx, params);
	}

	// native Variant:iter_get_value_var(MapIterator:iter);
	static cell AMX_NATIVE_CALL iter_get_value_var(AMX *amx, cell *params)
	{
		return value_at<>::iter_get_value<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_value_safe(MapIterator:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_value_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_value<dyn_func>(amx, params);
	}

	// native iter_get_value_arr_safe(MapIterator:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_get_value_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_get_value<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_value(MapIterator:iter, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_value(AMX *amx, cell *params)
	{
		return value_at<2, 3>::iter_set_value<dyn_func>(amx, params);
	}

	// native bool:iter_set_value_arr(MapIterator:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_value_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::iter_set_value<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_value_str(MapIterator:iter, const value[]);
	static cell AMX_NATIVE_CALL iter_set_value_str(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set_value<dyn_func_str>(amx, params);
	}

	// native bool:iter_set_value_var(MapIterator:iter, VariantTag:value);
	static cell AMX_NATIVE_CALL iter_set_value_var(AMX *amx, cell *params)
	{
		return value_at<2>::iter_set_value<dyn_func_var>(amx, params);
	}

	// native bool:iter_set_value_cell(MapIterator:iter, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL iter_set_value_cell(AMX *amx, cell *params)
	{
		return ::iter_set_value_cell(amx, params);
	}

	// native bool:iter_set_value_cell_safe(MapIterator:iter, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL iter_set_value_cell_safe(AMX *amx, cell *params)
	{
		return ::iter_set_value_cell<4>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_iter),
	AMX_DECLARE_NATIVE(map_iter),
	AMX_DECLARE_NATIVE(list_next),
	AMX_DECLARE_NATIVE(map_next),
	AMX_DECLARE_NATIVE(list_iter_valid),
	AMX_DECLARE_NATIVE(map_iter_valid),
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
	AMX_DECLARE_NATIVE(iter_get_key),
	AMX_DECLARE_NATIVE(iter_get_key_arr),
	AMX_DECLARE_NATIVE(iter_get_key_var),
	AMX_DECLARE_NATIVE(iter_get_key_safe),
	AMX_DECLARE_NATIVE(iter_get_key_arr_safe),
	AMX_DECLARE_NATIVE(iter_get_value),
	AMX_DECLARE_NATIVE(iter_get_value_arr),
	AMX_DECLARE_NATIVE(iter_get_value_var),
	AMX_DECLARE_NATIVE(iter_get_value_safe),
	AMX_DECLARE_NATIVE(iter_get_value_arr_safe),
	AMX_DECLARE_NATIVE(iter_set_value),
	AMX_DECLARE_NATIVE(iter_set_value_arr),
	AMX_DECLARE_NATIVE(iter_set_value_str),
	AMX_DECLARE_NATIVE(iter_set_value_var),
	AMX_DECLARE_NATIVE(iter_set_value_cell),
	AMX_DECLARE_NATIVE(iter_set_value_cell_safe),
};

int RegisterIterNatives(AMX *amx)
{
#ifdef _DEBUG
	return 0;
#else
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
#endif
}
