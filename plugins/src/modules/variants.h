#ifndef VARIANTS_H_INCLUDED
#define VARIANTS_H_INCLUDED

#include "objects/object_pool.h"
#include "objects/dyn_object.h"
#include "errors.h"
#include "sdk/amx/amx.h"

namespace variants
{
	extern object_pool<dyn_object> pool;

	inline cell create(dyn_object &&obj)
	{
		if(obj.is_null()) return 0;
		return pool.get_id(pool.add(std::move(obj)));
	}

	inline cell create(const dyn_object &obj)
	{
		if(obj.is_null()) return 0;
		return pool.get_id(pool.emplace(obj));
	}

	template <class... Args>
	inline cell emplace(Args&&... args)
	{
		return pool.get_id(pool.emplace(std::forward<Args>(args)...));
	}
	
	inline dyn_object get(cell ptr)
	{
		dyn_object *obj;
		if(pool.get_by_id(ptr, obj))
		{
			return *obj;
		}
		if(ptr != 0)
		{
			amx_LogicError(errors::pointer_invalid, "variant", ptr);
		}
		return dyn_object();
	}
}

inline dyn_object dyn_func(AMX *amx, cell value, cell tag_id)
{
	return dyn_object(amx, value, tag_id);
}

inline dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell tag_id)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return dyn_object(amx, addr, size, tag_id);
}

inline dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell size2, cell tag_id)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return dyn_object(amx, addr, size, size2, tag_id);
}

inline dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell size2, cell size3, cell tag_id)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return dyn_object(amx, addr, size, size2, size3, tag_id);
}

inline dyn_object dyn_func_str(AMX *amx, cell amx_addr)
{
	cell *addr = amx_GetAddrSafe(amx, amx_addr);
	return dyn_object(addr);
}

dyn_object dyn_func_str_s(AMX *amx, cell str);

inline dyn_object dyn_func_var(AMX *amx, cell ptr)
{
	return variants::get(ptr);
}

cell *get_offsets(AMX *amx, cell offsets, cell &offsets_size);

cell dyn_func(AMX *amx, const dyn_object &obj, cell offset);
cell dyn_func(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size);
cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offset, cell tag_id);
cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offsets, cell offsets_size, cell tag_id);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size, cell tag_id);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size, cell tag_id);
cell dyn_func_str(AMX *amx, const dyn_object &obj, cell amx_addr, cell size);
cell dyn_func_str(AMX *amx, const dyn_object &obj, cell amx_addr, cell offsets, cell size, cell offsets_size);
cell dyn_func_str_s(AMX *amx, const dyn_object &obj);
cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size);
cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell unused);
cell dyn_func_str_s(AMX *amx, const dyn_object &obj, cell offsets, cell offsets_size, cell unused);

inline cell dyn_func_var(AMX *amx, const dyn_object &obj)
{
	return variants::create(obj);
}

template <size_t... Indices>
class dyn_factory
{
#ifdef _WIN32
	//VS hacks
	template <class Type = dyn_object(&)(AMX*, decltype(static_cast<cell>(Indices))...)>
	using ftype_template = Type;
	static typename ftype_template<> fobj;
public:
	using type = decltype(fobj);
#else
public:
	using type = dyn_object(&)(AMX*, decltype(static_cast<cell>(Indices))...);
#endif
};

template <>
struct dyn_factory<>
{
	using type = dyn_object(&)(AMX*);
};

template <size_t... Indices>
class dyn_result
{
#ifdef _WIN32
	//VS hacks
	template <class Type = cell(&)(AMX*, const dyn_object &obj, decltype(static_cast<cell>(Indices))...)>
	using ftype_template = Type;
	static typename ftype_template<> fobj;
public:
	using type = decltype(fobj);
#else
public:
	using type = cell(&)(AMX*, const dyn_object &obj, decltype(static_cast<cell>(Indices))...);
#endif
};

template <>
struct dyn_result<>
{
	using type = cell(&)(AMX*, const dyn_object &obj);
};

#endif
