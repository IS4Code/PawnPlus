#include "natives.h"
#include "modules/variants.h"

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native Variant:var_new(value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL var_new(AMX *amx, cell *params)
	{
		return variants::create(Factory(amx, params[Indices]...));
	}

	// native var_get(VariantTag:var, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL var_get(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		return Factory(amx, *var, params[Indices]...);
	}
};

namespace Natives
{
	// native Variant:var_new(AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_new(AMX *amx, cell *params)
	{
		return value_at<1, 2>::var_new<dyn_func>(amx, params);
	}

	// native Variant:var_new_arr(const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_new_arr(AMX *amx, cell *params)
	{
		return value_at<1, 2, 3>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_arr_2d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_new_arr_2d(AMX *amx, cell *params)
	{
		return value_at<1, 2, 3, 4>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_arr_3d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[]), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_new_arr_3d(AMX *amx, cell *params)
	{
		return value_at<1, 2, 3, 4, 5>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_buf(size, tag_id=0);
	static cell AMX_NATIVE_CALL var_new_buf(AMX *amx, cell *params)
	{
		return variants::create(amx, nullptr, params[1], params[2]);
	}

	// native Variant:var_new_str(const value[]);
	static cell AMX_NATIVE_CALL var_new_str(AMX *amx, cell *params)
	{
		return value_at<1>::var_new<dyn_func_str>(amx, params);
	}

	// native Variant:var_new_str_s(StringTag:value);
	static cell AMX_NATIVE_CALL var_new_str_s(AMX *amx, cell *params)
	{
		return value_at<1>::var_new<dyn_func_str_s>(amx, params);
	}

	// native Variant:var_new_var(VariantTag:value);
	static cell AMX_NATIVE_CALL var_new_var(AMX *amx, cell *params)
	{
		return value_at<1>::var_new<dyn_func_var>(amx, params);
	}

	// native GlobalVariant:var_to_global(VariantTag:var);
	static cell AMX_NATIVE_CALL var_to_global(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		variants::pool.move_to_global(var);
		return params[1];
	}

	// native Variant:var_to_local(VariantTag:var);
	static cell AMX_NATIVE_CALL var_to_local(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		variants::pool.move_to_local(var);
		return params[1];
	}

	// native bool:var_delete(VariantTag:var);
	static cell AMX_NATIVE_CALL var_delete(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		return variants::pool.remove(var);
	}

	// native bool:var_delete_deep(VariantTag:var);
	static cell AMX_NATIVE_CALL var_delete_deep(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		var->free();
		return variants::pool.remove(var);
	}

	// native bool:var_valid(VariantTag:var);
	static cell AMX_NATIVE_CALL var_valid(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		return variants::pool.contains(var);
	}

	// native Variant:var_clone(VariantTag:var);
	static cell AMX_NATIVE_CALL var_clone(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		return reinterpret_cast<cell>(variants::pool.clone(var));
	}

	// native var_get(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_get(AMX *amx, cell *params)
	{
		return value_at<2, 3>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr(VariantTag:var, AnyTag:value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_get_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4, 5>::var_get<dyn_func_arr>(amx, params);
	}

	// native String:var_get_str_s(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_get_str_s(AMX *amx, cell *params)
	{
		return value_at<2, 3>::var_get<dyn_func_str_s>(amx, params);
	}

	// native bool:var_get_safe(VariantTag:var, &AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_get_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4, 5>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr_safe(VariantTag:var, AnyTag:value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_get_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4, 5, 6>::var_get<dyn_func_arr>(amx, params);
	}

	// native var_get_str_safe(VariantTag:var, value[], const offsets[]={cellmin}, size=sizeof(value), offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_get_str_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4, 5>::var_get<dyn_func_str>(amx, params);
	}

	// native String:var_get_str_safe_s(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_get_str_safe_s(AMX *amx, cell *params)
	{
		return value_at<2, 3, 0>::var_get<dyn_func_str_s>(amx, params);
	}

	// native bool:var_set_cell(VariantTag:var, AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_set_cell(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		if(!var->is_array()) return 0;
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[3], offsets_size);
		return var->set_cell(offsets_addr, offsets_size, params[2]);
	}

	// native bool:var_set_cell_safe(VariantTag:var, AnyTag:value, const offsets[]={cellmin}, offsets_size=sizeof(offsets), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_set_cell_safe(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		if(!var->is_array()) return 0;
		if(!var->tag_assignable(amx, params[5])) return 0;
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[3], offsets_size);
		return var->set_cell(offsets_addr, offsets_size, params[2]);
	}

	// native var_tagof(VariantTag:var);
	static cell AMX_NATIVE_CALL var_tagof(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		return var->get_tag(amx);
	}

	// native tag_uid:var_tag_uid(VariantTag:var);
	static cell AMX_NATIVE_CALL var_tag_uid(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		return var->get_tag()->uid;
	}

	// native var_sizeof(VariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	static cell AMX_NATIVE_CALL var_sizeof(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var)) return 0;
		cell offsets_size = params[3];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		return var->get_size(offsets_addr, offsets_size);
	}

	// native bool:var_equal(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_equal(AMX *amx, cell *params)
	{
		auto var1 = reinterpret_cast<dyn_object*>(params[1]);
		auto var2 = reinterpret_cast<dyn_object*>(params[2]);
		if((var1 != nullptr && !variants::pool.contains(var1)) || (var2 != nullptr && !variants::pool.contains(var2))) return 0;
		if(var1 == nullptr || var1->empty()) return var2 == nullptr || var2->empty();
		if(var2 == nullptr) return 0;
		return *var1 == *var2;
	}

	// native Variant:var_op(VariantTag:var1, VariantTag:var2);
	template <dyn_object (dyn_object::*op)(const dyn_object&) const>
	static cell AMX_NATIVE_CALL var_op(AMX *amx, cell *params)
	{
		auto var1 = reinterpret_cast<dyn_object*>(params[1]);
		if(!variants::pool.contains(var1)) return 0;
		auto var2 = reinterpret_cast<dyn_object*>(params[2]);
		if(!variants::pool.contains(var2)) return 0;
		auto var = (var1->*op)(*var2);
		if(var.empty()) return 0;
		return variants::create(std::move(var));
	}

	// native Variant:var_add(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_add(AMX *amx, cell *params)
	{
		return var_op<&dyn_object::operator+>(amx, params);
	}

	// native Variant:var_add(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_sub(AMX *amx, cell *params)
	{
		return var_op<&dyn_object::operator- >(amx, params);
	}

	// native Variant:var_add(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_mul(AMX *amx, cell *params)
	{
		return var_op<&dyn_object::operator*>(amx, params);
	}

	// native Variant:var_add(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_div(AMX *amx, cell *params)
	{
		return var_op<&dyn_object::operator/>(amx, params);
	}

	// native Variant:var_add(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_mod(AMX *amx, cell *params)
	{
		return var_op<&dyn_object::operator% >(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(var_new),
	AMX_DECLARE_NATIVE(var_new_arr),
	AMX_DECLARE_NATIVE(var_new_arr_2d),
	AMX_DECLARE_NATIVE(var_new_arr_3d),
	AMX_DECLARE_NATIVE(var_new_buf),
	AMX_DECLARE_NATIVE(var_new_str),
	AMX_DECLARE_NATIVE(var_new_str_s),
	AMX_DECLARE_NATIVE(var_new_var),
	AMX_DECLARE_NATIVE(var_to_global),
	AMX_DECLARE_NATIVE(var_to_local),
	AMX_DECLARE_NATIVE(var_delete),
	AMX_DECLARE_NATIVE(var_delete_deep),
	AMX_DECLARE_NATIVE(var_valid),
	AMX_DECLARE_NATIVE(var_clone),

	AMX_DECLARE_NATIVE(var_get),
	AMX_DECLARE_NATIVE(var_get_arr),
	AMX_DECLARE_NATIVE(var_get_str_s),
	AMX_DECLARE_NATIVE(var_get_safe),
	AMX_DECLARE_NATIVE(var_get_arr_safe),
	AMX_DECLARE_NATIVE(var_get_str_safe),
	AMX_DECLARE_NATIVE(var_get_str_safe_s),

	AMX_DECLARE_NATIVE(var_set_cell),
	AMX_DECLARE_NATIVE(var_set_cell_safe),

	AMX_DECLARE_NATIVE(var_tagof),
	AMX_DECLARE_NATIVE(var_tag_uid),
	AMX_DECLARE_NATIVE(var_sizeof),
	AMX_DECLARE_NATIVE(var_equal),
	AMX_DECLARE_NATIVE(var_add),
	AMX_DECLARE_NATIVE(var_sub),
	AMX_DECLARE_NATIVE(var_mul),
	AMX_DECLARE_NATIVE(var_div),
	AMX_DECLARE_NATIVE(var_mod),
};

int RegisterVariantNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
