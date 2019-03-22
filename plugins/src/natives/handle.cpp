#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include <vector>

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native Handle:handle_new(value, bool:weak=false, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL handle_new(AMX *amx, cell *params)
	{
		return handle_pool.get_id(handle_pool.emplace(Factory(amx, params[Indices]...), optparam(2, 0)));
	}

	// native Handle:handle_alias(HandleTag:handle, value);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL handle_alias(AMX *amx, cell *params)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return handle_pool.get_id(handle_pool.emplace(*handle, Factory(amx, params[Indices]...), true));
	}

	// native handle_get(HandleTag:handle, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL handle_get(AMX *amx, cell *params)
	{
		handle_t *handle;
		if(params[1] == 0)
		{
			return Factory(amx, dyn_object(), params[Indices]...);
		}
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return Factory(amx, handle->get(), params[Indices]...);
	}
};

namespace Natives
{
	// native Handle:handle_new(AnyTag:value, bool:weak=false, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_new, 3)
	{
		return value_at<1, 3>::handle_new<dyn_func>(amx, params);
	}

	// native Handle:handle_new_arr(const AnyTag:value[], bool:weak=false, size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_new_arr, 4)
	{
		return value_at<1, 3, 4>::handle_new<dyn_func_arr>(amx, params);
	}

	// native Handle:handle_new_var(ConstVariantTag:value, bool:weak=false);
	AMX_DEFINE_NATIVE(handle_new_var, 2)
	{
		return value_at<1>::handle_new<dyn_func_var>(amx, params);
	}

	// native Handle:handle_alias(HandleTag:handle, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_alias, 3)
	{
		return value_at<2, 3>::handle_alias<dyn_func>(amx, params);
	}

	// native Handle:handle_alias_arr(HandleTag:handle, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(handle_alias_arr, 4)
	{
		return value_at<2, 3, 4>::handle_alias<dyn_func_arr>(amx, params);
	}

	// native Handle:handle_alias_var(HandleTag:handle, ConstVariantTag:value);
	AMX_DEFINE_NATIVE(handle_alias_var, 2)
	{
		return value_at<2>::handle_alias<dyn_func_var>(amx, params);
	}

	// native Handle:handle_acquire(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_acquire, 1)
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		if(!handle_pool.acquire_ref(*handle)) amx_LogicError(errors::cannot_acquire, "handle", params[1]);
		return params[1];
	}

	// native Handle:handle_release(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_release, 1)
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		if(!handle_pool.release_ref(*handle)) amx_LogicError(errors::cannot_acquire, "handle", params[1]);
		return params[1];
	}

	// native handle_delete(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_delete, 1)
	{
		if(!handle_pool.remove_by_id(params[1])) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return 1;
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
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return handle->get().get_tag(amx);
	}

	// native handle_sizeof(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_sizeof, 1)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return handle->get().get_size();
	}

	// native bool:handle_linked(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_linked, 1)
	{
		handle_t *handle;
		if(params[1] == 0) return false;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return handle->linked();
	}

	// native bool::handle_alive(HandleTag:handle);
	AMX_DEFINE_NATIVE(handle_alive, 1)
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		return handle->alive();
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(handle_new),
	AMX_DECLARE_NATIVE(handle_new_arr),
	AMX_DECLARE_NATIVE(handle_new_var),
	AMX_DECLARE_NATIVE(handle_alias),
	AMX_DECLARE_NATIVE(handle_alias_arr),
	AMX_DECLARE_NATIVE(handle_alias_var),
	AMX_DECLARE_NATIVE(handle_acquire),
	AMX_DECLARE_NATIVE(handle_release),
	AMX_DECLARE_NATIVE(handle_delete),
	AMX_DECLARE_NATIVE(handle_valid),
	AMX_DECLARE_NATIVE(handle_linked),
	AMX_DECLARE_NATIVE(handle_alive),

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
