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
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		return Factory(amx, *var, params[Indices]...);
	}
};

namespace Natives
{
	// native Variant:var_new(AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_new, 2, variant)
	{
		return value_at<1, 2>::var_new<dyn_func>(amx, params);
	}

	// native Variant:var_new_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_new_arr, 3, variant)
	{
		return value_at<1, 2, 3>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_arr_2d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_new_arr_2d, 4, variant)
	{
		return value_at<1, 2, 3, 4>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_arr_3d(const AnyTag:value[][], size=sizeof(value), size2=sizeof(value[]), size3=sizeof(value[]), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_new_arr_3d, 5, variant)
	{
		return value_at<1, 2, 3, 4, 5>::var_new<dyn_func_arr>(amx, params);
	}

	// native Variant:var_new_buf(size, TagTag:tag_id=0);
	AMX_DEFINE_NATIVE_TAG(var_new_buf, 1, variant)
	{
		return variants::emplace(amx, nullptr, params[1], optparam(2, 0));
	}

	// native Variant:var_new_str(const value[]);
	AMX_DEFINE_NATIVE_TAG(var_new_str, 1, variant)
	{
		return value_at<1>::var_new<dyn_func_str>(amx, params);
	}

	// native Variant:var_new_str_s(ConstStringTag:value);
	AMX_DEFINE_NATIVE_TAG(var_new_str_s, 1, variant)
	{
		return value_at<1>::var_new<dyn_func_str_s>(amx, params);
	}

	// native Variant:var_new_var(ConstVariantTag:value);
	AMX_DEFINE_NATIVE_TAG(var_new_var, 1, variant)
	{
		return value_at<1>::var_new<dyn_func_var>(amx, params);
	}

	// native AmxVariant:var_addr(VariantTag:str);
	AMX_DEFINE_NATIVE_TAG(var_addr, 1, cell)
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return variants::pool.get_null_address(amx);
		}
		if((*var)->is_cell())
		{
			amx_LogicError(errors::operation_not_supported, "variant");
		}
		return variants::pool.get_address(amx, *var);
	}

	// native ConstAmxVariant:var_addr_const(ConstVariantTag:str);
	AMX_DEFINE_NATIVE_TAG(var_addr_const, 1, cell)
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return variants::pool.get_null_address(amx);
		}
		return variants::pool.get_address(amx, *var);
	}

	// native AmxVariantBuffer:var_buf_addr(VariantTag:str);
	AMX_DEFINE_NATIVE_TAG(var_buf_addr, 1, cell)
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if((*var)->is_cell())
		{
			amx_LogicError(errors::operation_not_supported, "variant");
		}
		return variants::pool.get_inner_address(amx, *var);
	}

	// native Variant:var_acquire(VariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_acquire, 1, variant)
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!variants::pool.acquire_ref(*var)) amx_LogicError(errors::cannot_acquire, "variant", params[1]);
		return params[1];
	}

	// native Variant:var_release(VariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_release, 1, variant)
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!variants::pool.release_ref(*var)) amx_LogicError(errors::cannot_release, "variant", params[1]);
		return params[1];
	}

	// native var_delete(VariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_delete, 1, cell)
	{
		return variants::pool.remove_by_id(params[1]);
	}

	// native bool:var_valid(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_valid, 1, bool)
	{
		dyn_object *var;
		return variants::pool.get_by_id(params[1], var);
	}

	// native Variant:var_clone(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_clone, 1, variant)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0;
		}
		return variants::pool.get_id(variants::pool.add(var->clone()));
	}

	// native var_get(ConstVariantTag:var, offset=0);
	AMX_DEFINE_NATIVE(var_get, 2)
	{
		return value_at<2>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr(ConstVariantTag:var, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_arr, 3, cell)
	{
		return value_at<2, 3>::var_get<dyn_func_arr>(amx, params);
	}

	// native String:var_get_str_s(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_get_str_s, 1, string)
	{
		return value_at<>::var_get<dyn_func_str_s>(amx, params);
	}

	// native bool:var_get_safe(ConstVariantTag:var, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_safe, 4, bool)
	{
		return value_at<2, 3, 4>::var_get<dyn_func>(amx, params);
	}

	// native var_get_arr_safe(ConstVariantTag:var, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_arr_safe, 4, cell)
	{
		return value_at<2, 3, 4>::var_get<dyn_func_arr>(amx, params);
	}

	// native var_get_str_safe(ConstVariantTag:var, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_str_safe, 3, cell)
	{
		return value_at<2, 3>::var_get<dyn_func_str>(amx, params);
	}

	// native String:var_get_str_safe_s(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_get_str_safe_s, 1, string)
	{
		return value_at<0>::var_get<dyn_func_str_s>(amx, params);
	}

	// native var_set_cell(VariantTag:var, offset, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(var_set_cell, 3, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		var->set_cell(params[2], params[3]);
		return 1;
	}

	// native bool:var_set_cell_safe(VariantTag:var, offset, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_set_cell_safe, 4, bool)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!var->tag_assignable(amx, params[4])) return 0;
		var->set_cell(params[2], params[3]);
		return 1;
	}

	// native var_set_cells(VariantTag:var, offset, AnyTag:values[], size=sizeof(values));
	AMX_DEFINE_NATIVE_TAG(var_set_cells, 4, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		return var->set_cells(params[2], addr, params[4]);
	}

	// native var_set_cells_safe(VariantTag:var, offset, AnyTag:values[], size=sizeof(values), TagTag:tag_id=tagof(values));
	AMX_DEFINE_NATIVE_TAG(var_set_cells_safe, 5, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!var->tag_assignable(amx, params[5])) return 0;
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		return var->set_cells(params[2], addr, params[4]);
	}

	// native var_get_md(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(var_get_md, 3)
	{
		return value_at<2, 3>::var_get<dyn_func>(amx, params);
	}

	// native var_get_md_arr(ConstVariantTag:var, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_get_md_arr, 5, cell)
	{
		return value_at<3, 2, 4, 5>::var_get<dyn_func_arr>(amx, params);
	}

	// native String:var_get_md_str_s(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_get_md_str_s, 3, string)
	{
		return value_at<2, 3>::var_get<dyn_func_str_s>(amx, params);
	}

	// native bool:var_get_md_safe(ConstVariantTag:var, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_md_safe, 5, bool)
	{
		return value_at<3, 2, 4, 5>::var_get<dyn_func>(amx, params);
	}

	// native var_get_md_arr_safe(ConstVariantTag:var, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_get_md_arr_safe, 6, cell)
	{
		return value_at<3, 2, 4, 5, 6>::var_get<dyn_func_arr>(amx, params);
	}

	// native var_get_md_str_safe(ConstVariantTag:var, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_get_md_str_safe, 5, cell)
	{
		return value_at<3, 2, 4, 5>::var_get<dyn_func_str>(amx, params);
	}

	// native String:var_get_md_str_safe_s(ConstVariantTag:var, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_get_md_str_safe_s, 3, string)
	{
		return value_at<2, 3, 0>::var_get<dyn_func_str_s>(amx, params);
	}

	// native var_set_cell_md(VariantTag:var, const offsets[], AnyTag:value, offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_set_cell_md, 4, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		var->set_cell(offsets_addr, offsets_size, params[3]);
		return 1;
	}

	// native bool:var_set_cell_md_safe(VariantTag:var, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(var_set_cell_md_safe, 5, bool)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!var->tag_assignable(amx, params[5])) return 0;
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		var->set_cell(offsets_addr, offsets_size, params[3]);
		return 1;
	}

	// native var_set_cells_md(VariantTag:var, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values));
	AMX_DEFINE_NATIVE_TAG(var_set_cells_md, 5, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		return var->set_cells(offsets_addr, offsets_size, addr, params[5]);
	}

	// native var_set_cells_md_safe(VariantTag:var, const offsets[], AnyTag:values[], offsets_size=sizeof(offsets), size=sizeof(values), TagTag:tag_id=tagof(values));
	AMX_DEFINE_NATIVE_TAG(var_set_cells_md_safe, 6, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!var->tag_assignable(amx, params[6])) return 0;
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		return var->set_cells(offsets_addr, offsets_size, addr, params[5]);
	}

	// native var_tagof(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_tagof, 1, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0x80000000;
		}
		return var->get_tag(amx);
	}

	// native tag_uid:var_tag_uid(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_tag_uid, 1, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return tags::tag_cell;
		}
		return var->get_tag()->uid;
	}

	// native var_sizeof(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_sizeof, 1, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0;
		}
		return var->get_size();
	}

	// native var_sizeof_md(ConstVariantTag:var, const offsets[]={cellmin}, offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE_TAG(var_sizeof_md, 3, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		cell offsets_size = params[3];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		return var->get_size(offsets_addr, offsets_size);
	}

	// native var_rank(ConstVariantTag:var)
	AMX_DEFINE_NATIVE_TAG(var_rank, 1, cell)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		return var == nullptr ? -1 : var->get_rank();
	}
	
	// native Variant:var_bin_op(ConstVariantTag:var1, ConstVariantTag:var2);
	template <dyn_object (dyn_object::*op)(const dyn_object&) const>
	static cell AMX_NATIVE_CALL var_bin_op(AMX *amx, cell *params)
	{
		dyn_object *var1;
		if(!variants::pool.get_by_id(params[1], var1) && var1 != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		dyn_object *var2;
		if(!variants::pool.get_by_id(params[2], var2) && var2 != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[2]);
		if(var1 == nullptr || var2 == nullptr)
		{
			return 0;
		}
		auto var = (var1->*op)(*var2);
		return variants::create(std::move(var));
	}

	// native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_add, 2, variant)
	{
		return var_bin_op<&dyn_object::operator+>(amx, params);
	}

	// native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_sub, 2, variant)
	{
		return var_bin_op<&dyn_object::operator- >(amx, params);
	}

	// native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_mul, 2, variant)
	{
		return var_bin_op<&dyn_object::operator*>(amx, params);
	}

	// native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_div, 2, variant)
	{
		return var_bin_op<&dyn_object::operator/>(amx, params);
	}

	// native Variant:var_add(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_mod, 2, variant)
	{
		return var_bin_op<&dyn_object::operator% >(amx, params);
	}
	
	// native Variant:var_un_op(ConstVariantTag:var);
	template <dyn_object (dyn_object::*op)() const>
	static cell AMX_NATIVE_CALL var_un_op(AMX *amx, cell *params)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0;
		}
		auto result = (var->*op)();
		return variants::create(std::move(result));
	}

	// native Variant:var_neg(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_neg, 1, variant)
	{
		return var_un_op<&dyn_object::operator- >(amx, params);
	}

	// native Variant:var_neg(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_inc, 1, variant)
	{
		return var_un_op<&dyn_object::inc>(amx, params);
	}

	// native Variant:var_neg(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_dec, 1, variant)
	{
		return var_un_op<&dyn_object::dec>(amx, params);
	}

	// native bool:var_log_op(ConstVariantTag:var1, ConstVariantTag:var2);
	template <bool(dyn_object::*op)(const dyn_object&) const>
	static cell AMX_NATIVE_CALL var_log_op(AMX *amx, cell *params)
	{
		dyn_object *var1, *var2;
		if((!variants::pool.get_by_id(params[1], var1) && var1 != nullptr)) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if((!variants::pool.get_by_id(params[2], var2) && var2 != nullptr)) amx_LogicError(errors::pointer_invalid, "variant", params[2]);
		bool init1 = false, init2 = false;
		if(var1 == nullptr)
		{
			var1 = new dyn_object();
			init1 = true;
		}
		if(var2 == nullptr)
		{
			var2 = new dyn_object();
			init2 = false;
		}
		bool result = (var1->*op)(*var2);
		if(init1)
		{
			delete var1;
		}
		if(init2)
		{
			delete var2;
		}
		return result;
	}

	// native bool:var_eq(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_eq, 2, bool)
	{
		return var_log_op<&dyn_object::operator==>(amx, params);
	}

	// native bool:var_neq(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_neq, 2, bool)
	{
		return var_log_op<&dyn_object::operator!=>(amx, params);
	}

	// native bool:var_lt(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_lt, 2, bool)
	{
		return var_log_op<&dyn_object::operator<>(amx, params);
	}

	// native bool:var_gt(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_gt, 2, bool)
	{
		return var_log_op<(&dyn_object::operator>)>(amx, params);
	}

	// native bool:var_lte(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_lte, 2, bool)
	{
		return var_log_op<&dyn_object::operator<=>(amx, params);
	}

	// native bool:var_gte(ConstVariantTag:var1, ConstVariantTag:var2);
	AMX_DEFINE_NATIVE_TAG(var_gte, 2, bool)
	{
		return var_log_op<&dyn_object::operator>=>(amx, params);
	}

	// native bool:var_not(ConstVariantTag:var);
	AMX_DEFINE_NATIVE_TAG(var_not, 1, bool)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return true;
		}
		return !(*var);
	}

	// native Variant:var_call_op(VariantTag:var, tag_op:tag_op, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(var_call_op, 2, variant)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0;
		}
		size_t numargs = (params[0] / sizeof(cell)) - 2;
		cell *args = new cell[numargs];
		for(size_t i = 0; i < numargs; i++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[3 + i]);
			args[i] = *addr;
		}
		auto result = var->call_op(static_cast<op_type>(params[2]), args, numargs, true);
		delete[] args;
		return variants::create(std::move(result));
	}

	// native Variant:var_call_op_raw(VariantTag:var, tag_op:tag_op, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(var_call_op_raw, 2, variant)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(var == nullptr)
		{
			return 0;
		}
		size_t numargs = (params[0] / sizeof(cell)) - 2;
		cell *args = new cell[numargs];
		for(size_t i = 0; i < numargs; i++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[3 + i]);
			args[i] = *addr;
		}
		auto result = var->call_op(static_cast<op_type>(params[2]), args, numargs, false);
		delete[] args;
		return variants::create(std::move(result));
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
	AMX_DECLARE_NATIVE(var_addr),
	AMX_DECLARE_NATIVE(var_addr_const),
	AMX_DECLARE_NATIVE(var_buf_addr),
	AMX_DECLARE_NATIVE(var_acquire),
	AMX_DECLARE_NATIVE(var_release),
	AMX_DECLARE_NATIVE(var_delete),
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
	AMX_DECLARE_NATIVE(var_set_cells),
	AMX_DECLARE_NATIVE(var_set_cells_safe),

	AMX_DECLARE_NATIVE(var_get_md),
	AMX_DECLARE_NATIVE(var_get_md_arr),
	AMX_DECLARE_NATIVE(var_get_md_str_s),
	AMX_DECLARE_NATIVE(var_get_md_safe),
	AMX_DECLARE_NATIVE(var_get_md_arr_safe),
	AMX_DECLARE_NATIVE(var_get_md_str_safe),
	AMX_DECLARE_NATIVE(var_get_md_str_safe_s),

	AMX_DECLARE_NATIVE(var_set_cell_md),
	AMX_DECLARE_NATIVE(var_set_cell_md_safe),
	AMX_DECLARE_NATIVE(var_set_cells_md),
	AMX_DECLARE_NATIVE(var_set_cells_md_safe),

	AMX_DECLARE_NATIVE(var_tagof),
	AMX_DECLARE_NATIVE(var_tag_uid),
	AMX_DECLARE_NATIVE(var_sizeof),
	AMX_DECLARE_NATIVE(var_sizeof_md),
	AMX_DECLARE_NATIVE(var_rank),
	AMX_DECLARE_NATIVE(var_add),
	AMX_DECLARE_NATIVE(var_sub),
	AMX_DECLARE_NATIVE(var_mul),
	AMX_DECLARE_NATIVE(var_div),
	AMX_DECLARE_NATIVE(var_mod),
	AMX_DECLARE_NATIVE(var_neg),
	AMX_DECLARE_NATIVE(var_inc),
	AMX_DECLARE_NATIVE(var_dec),
	AMX_DECLARE_NATIVE(var_eq),
	AMX_DECLARE_NATIVE(var_neq),
	AMX_DECLARE_NATIVE(var_lt),
	AMX_DECLARE_NATIVE(var_gt),
	AMX_DECLARE_NATIVE(var_lte),
	AMX_DECLARE_NATIVE(var_gte),
	AMX_DECLARE_NATIVE(var_not),
	AMX_DECLARE_NATIVE(var_call_op),
	AMX_DECLARE_NATIVE(var_call_op_raw),
};

int RegisterVariantNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
