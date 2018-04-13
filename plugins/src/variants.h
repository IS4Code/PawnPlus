#ifndef VARIANTS_H_INCLUDED
#define VARIANTS_H_INCLUDED

#include "utils/object_pool.h"
#include "utils/dyn_object.h"
#include "sdk/amx/amx.h"

namespace variants
{
	extern object_pool<dyn_object> pool;

	cell create(dyn_object &&obj);
	dyn_object get(cell ptr);

	template <class... Args>
	cell create(Args&&... args)
	{
		return create(dyn_object(std::forward<Args>(args)...));
	}
}

dyn_object dyn_func(AMX *amx, cell value, cell tag_id);
dyn_object dyn_func_arr(AMX *amx, cell amx_addr, cell size, cell tag_id);
dyn_object dyn_func_str(AMX *amx, cell amx_addr);
dyn_object dyn_func_var(AMX *amx, cell ptr);

cell dyn_func(AMX *amx, const dyn_object &obj, cell offset);
cell dyn_func(AMX *amx, const dyn_object &obj, cell result, cell offset, cell tag_id);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size);
cell dyn_func_arr(AMX *amx, const dyn_object &obj, cell amx_addr, cell size, cell tag_id);
cell dyn_func_var(AMX *amx, const dyn_object &obj);

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
