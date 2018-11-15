#include "natives.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include <vector>

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native Handle:handle_new(value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL handle_new(AMX *amx, cell *params)
	{
		return handle_pool.get_id(handle_pool.add(handle_t(Factory(amx, params[Indices]...))));
	}

	// native handle_get(HandleTag:handle, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL handle_get(AMX *amx, cell *params)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) return 0;
		return Factory(amx, handle->get(), params[Indices]...);
	}
};

namespace Natives
{
	// native Handle:handle_new(AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_new, 2)
	{
		return value_at<1, 2>::handle_new<dyn_func>(amx, params);
	}

	// native Handle:handle_new_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_new_arr, 3)
	{
		return value_at<1, 2, 3>::handle_new<dyn_func_arr>(amx, params);
	}

	// native Handle:handle_new_var(ConstVariantTag:value);
	AMX_DEFINE_NATIVE(handle_new_var, 1)
	{
		return value_at<1>::handle_new<dyn_func_var>(amx, params);
	}

	// native Handle:handle_acquire(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_acquire, 1)
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(params[1], handle)) return 0;
		if(!handle_pool.acquire_ref(*handle)) return 0;
		return params[1];
	}

	// native Handle:handle_release(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_release, 1)
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(params[1], handle)) return 0;
		if(!handle_pool.release_ref(*handle)) return 0;
		return params[1];
	}

	// native bool:handle_delete(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_delete, 1)
	{
		return handle_pool.remove_by_id(params[1]);
	}

	// native bool:handle_valid(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_valid, 1)
	{
		handle_t *handle;
		return handle_pool.get_by_id(params[1], handle);
	}

	// native handle_get(HandleTag:handle, offset=0);
	AMX_DEFINE_NATIVE(handle_get, 2)
	{
		return value_at<2>::handle_get<dyn_func>(amx, params);
	}

	// native handle_get_arr(HandleTag:handle, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(handle_get_arr, 3)
	{
		return value_at<2, 3>::handle_get<dyn_func_arr>(amx, params);
	}

	// native Variant:handle_get_var(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_get_var, 1)
	{
		return value_at<>::handle_get<dyn_func_var>(amx, params);
	}

	// native bool:handle_get_safe(HandleTag:handle, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_get_safe, 4)
	{
		return value_at<2, 3, 4>::handle_get<dyn_func>(amx, params);
	}

	// native handle_get_arr_safe(ConstVariantTag:var, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_get_arr_safe, 4)
	{
		return value_at<2, 3, 4>::handle_get<dyn_func_arr>(amx, params);
	}

	// native handle_tagof(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_tagof, 1)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) return 0;
		return handle->get().get_tag(amx);
	}

	// native handle_sizeof(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_sizeof, 1)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) return 0;
		return handle->get().get_size();
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(handle_new),
	AMX_DECLARE_NATIVE(handle_new_arr),
	AMX_DECLARE_NATIVE(handle_new_var),
	AMX_DECLARE_NATIVE(handle_acquire),
	AMX_DECLARE_NATIVE(handle_release),
	AMX_DECLARE_NATIVE(handle_delete),
	AMX_DECLARE_NATIVE(handle_valid),

	AMX_DECLARE_NATIVE(handle_get),
	AMX_DECLARE_NATIVE(handle_get_arr),
	AMX_DECLARE_NATIVE(handle_get_var),
	AMX_DECLARE_NATIVE(handle_get_safe),
	AMX_DECLARE_NATIVE(handle_get_arr_safe),

	AMX_DECLARE_NATIVE(handle_tagof),
	AMX_DECLARE_NATIVE(handle_sizeof),
};

int RegisterHandleNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
