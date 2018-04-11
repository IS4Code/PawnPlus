#include "../natives.h"
#include "../variants.h"

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native Variant:var_new(value, ...);
	template <typename value_ftype Factory>
	static cell AMX_NATIVE_CALL var_new(AMX *amx, cell *params)
	{
		return variants::create(Factory(amx, params[Indices]...));
	}

	// native var_get(VariantTag:var, ...);
	template <typename result_ftype Factory>
	static cell AMX_NATIVE_CALL var_get(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
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

	// native Variant:var_new_str(const value[]);
	static cell AMX_NATIVE_CALL var_new_str(AMX *amx, cell *params)
	{
		return value_at<1>::var_new<dyn_func_str>(amx, params);
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
		if(var == nullptr) return 0;
		variants::pool.move_to_global(var);
		return params[1];
	}

	// native Variant:var_to_local(VariantTag:var);
	static cell AMX_NATIVE_CALL var_to_local(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		variants::pool.move_to_local(var);
		return params[1];
	}

	// native bool:var_delete(VariantTag:var);
	static cell AMX_NATIVE_CALL var_delete(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		return variants::pool.free(var);
	}

	// native bool:var_is_valid(VariantTag:var);
	static cell AMX_NATIVE_CALL var_is_valid(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		return variants::pool.is_valid(var);
	}

	// native var_get(VariantTag:var, offset=0);
	static cell AMX_NATIVE_CALL var_get(AMX *amx, cell *params)
	{
		return value_at<2>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr(VariantTag:var, AnyTag:value[], size=sizeof(value));
	static cell AMX_NATIVE_CALL var_get_arr(AMX *amx, cell *params)
	{
		return value_at<2, 3>::var_get<dyn_func_arr>(amx, params);
	}

	// native bool:var_get_safe(VariantTag:var, &AnyTag:value, offset=0, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_get_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr_safe(VariantTag:var, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_get_arr_safe(AMX *amx, cell *params)
	{
		return value_at<2, 3, 4>::var_get<dyn_func_arr>(amx, params);
	}

	// native bool:var_set_cell(VariantTag:var, offset, AnyTag:value);
	static cell AMX_NATIVE_CALL var_set_cell(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		return var->set_cell(params[2], params[3]);
	}

	// native bool:var_set_cell_safe(VariantTag:var, offset, AnyTag:value, tag_id=tagof(value));
	static cell AMX_NATIVE_CALL var_set_cell_safe(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		if(!var->check_tag(amx, params[4])) return 0;
		return var->set_cell(params[2], params[3]);
	}

	// native var_tagof(VariantTag:var);
	static cell AMX_NATIVE_CALL var_tagof(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		return var->get_tag(amx);
	}

	// native var_sizeof(VariantTag:var);
	static cell AMX_NATIVE_CALL var_sizeof(AMX *amx, cell *params)
	{
		auto var = reinterpret_cast<dyn_object*>(params[1]);
		if(var == nullptr) return 0;
		return var->get_size();
	}

	// native bool:var_equal(VariantTag:var1, VariantTag:var2);
	static cell AMX_NATIVE_CALL var_equal(AMX *amx, cell *params)
	{
		auto var1 = reinterpret_cast<dyn_object*>(params[1]);
		auto var2 = reinterpret_cast<dyn_object*>(params[2]);
		if(var1 == nullptr || var1->empty()) return var2 == nullptr || var2->empty();
		if(var2 == nullptr) return 0;
		return *var1 == *var2;
	}

	// native Variant:var_op(VariantTag:var1, VariantTag:var2);
	template <dyn_object (dyn_object::*op)(const dyn_object&) const>
	static cell AMX_NATIVE_CALL var_op(AMX *amx, cell *params)
	{
		auto var1 = reinterpret_cast<dyn_object*>(params[1]);
		if(var1 == nullptr) return 0;
		auto var2 = reinterpret_cast<dyn_object*>(params[2]);
		if(var2 == nullptr) return 0;
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
		return var_op<&dyn_object::operator%>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(var_new),
	AMX_DECLARE_NATIVE(var_new_arr),
	AMX_DECLARE_NATIVE(var_new_str),
	AMX_DECLARE_NATIVE(var_new_var),
	AMX_DECLARE_NATIVE(var_to_global),
	AMX_DECLARE_NATIVE(var_to_local),
	AMX_DECLARE_NATIVE(var_delete),
	AMX_DECLARE_NATIVE(var_is_valid),

	AMX_DECLARE_NATIVE(var_get),
	AMX_DECLARE_NATIVE(var_get_arr),
	AMX_DECLARE_NATIVE(var_get_safe),
	AMX_DECLARE_NATIVE(var_get_arr_safe),

	AMX_DECLARE_NATIVE(var_set_cell),
	AMX_DECLARE_NATIVE(var_set_cell_safe),

	AMX_DECLARE_NATIVE(var_tagof),
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
