#include "natives.h"
#include "modules/expressions.h"
#include "modules/variants.h"

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native Expression:expr_const(value, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL expr_const(AMX *amx, cell *params)
	{
		auto &expr = expression_pool.emplace_derived<constant_expression>(Factory(amx, params[Indices]...));
		return expression_pool.get_id(expr);
	}

	// native expr_get(Expression:expr, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL expr_get(AMX *amx, cell *params)
	{
		expression *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		return Factory(amx, ptr->execute(amx, {}), params[Indices]...);
	}

	// native expr_set(Expression:expr, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL expr_set(AMX *amx, cell *params)
	{
		expression *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		ptr->assign(amx, {}, Factory(amx, params[Indices]...));
		return 1;
	}
};

namespace Natives
{

	// native Expression:expr_acquire(Expression:expr);
	AMX_DEFINE_NATIVE(expr_acquire, 1)
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		if(!expression_pool.acquire_ref(*ptr)) amx_LogicError(errors::cannot_acquire, "expression", params[1]);
		return params[1];
	}

	// native Expression:expr_release(Expression:expr);
	AMX_DEFINE_NATIVE(expr_release, 1)
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		if(!expression_pool.release_ref(*ptr)) amx_LogicError(errors::cannot_release, "expression", params[1]);
		return params[1];
	}

	// native expr_delete(Expression:expr);
	AMX_DEFINE_NATIVE(expr_delete, 1)
	{
		if(!expression_pool.remove_by_id(params[1])) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		return 1;
	}

	// native bool:expr_valid(Expression:expr);
	AMX_DEFINE_NATIVE(expr_valid, 1)
	{
		expression *ptr;
		return expression_pool.get_by_id(params[1], ptr);
	}

	// native Expression:expr_weak(Expression:expr);
	AMX_DEFINE_NATIVE(expr_weak, 1)
	{
		std::shared_ptr<expression> ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		auto &expr = expression_pool.emplace_derived<weak_expression>(std::move(ptr));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_const(AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_const, 2)
	{
		return value_at<1, 2>::expr_const<dyn_func>(amx, params);
	}

	// native Expression:expr_const_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_const_arr, 3)
	{
		return value_at<1, 2, 3>::expr_const<dyn_func_arr>(amx, params);
	}

	// native Expression:expr_const_str(const value[]);
	AMX_DEFINE_NATIVE(expr_const_str, 1)
	{
		return value_at<1>::expr_const<dyn_func_str>(amx, params);
	}

	// native Expression:expr_const_var(VariantTag:value);
	AMX_DEFINE_NATIVE(expr_const_var, 1)
	{
		return value_at<1>::expr_const<dyn_func_var>(amx, params);
	}

	// native Expression:expr_arr(Expression:...);
	AMX_DEFINE_NATIVE(expr_arr, 0)
	{
		cell numargs = params[0] / sizeof(cell);
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[1 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			std::shared_ptr<expression> ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		auto &expr = expression_pool.emplace_derived<array_expression>(std::move(args));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_arg(index);
	AMX_DEFINE_NATIVE(expr_arg, 1)
	{
		cell index = params[1];
		if(index < 0) amx_LogicError(errors::out_of_range, "argument index");
		auto &expr = expression_pool.emplace_derived<arg_expression>(index);
		return expression_pool.get_id(expr);
	}

	template <class Expr, class... Args>
	static cell AMX_NATIVE_CALL expr_unary(AMX *amx, cell *params, Args &&...args)
	{
		std::shared_ptr<expression> ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		auto &expr = expression_pool.emplace_derived<Expr>(std::move(ptr), std::forward<Args>(args)...);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_bind(Expression:expr, Expression:...);
	AMX_DEFINE_NATIVE(expr_bind, 1)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			std::shared_ptr<expression> ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<bind_expression>(amx, params, std::move(args));
	}

	template <class Expr, class... Args>
	static cell AMX_NATIVE_CALL expr_binary(AMX *amx, cell *params, Args &&...args)
	{
		std::shared_ptr<expression> ptr1;
		if(!expression_pool.get_by_id(params[1], ptr1)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		std::shared_ptr<expression> ptr2;
		if(!expression_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		auto &expr = expression_pool.emplace_derived<Expr>(std::move(ptr1), std::move(ptr2), std::forward<Args>(args)...);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_comma(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_comma, 2)
	{
		return expr_binary<comma_expression>(amx, params);
	}

	// native Expression:expr_assign(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_assign, 2)
	{
		return expr_binary<assign_expression>(amx, params);
	}

	// native Expression:expr_try(Expression:main, Expression:fallback);
	AMX_DEFINE_NATIVE(expr_try, 2)
	{
		return expr_binary<try_expression>(amx, params);
	}

	// native Expression:expr_symbol(Symbol:symbol);
	AMX_DEFINE_NATIVE(expr_symbol, 1)
	{
		const auto &obj = amx::load_lock(amx);
		if(!obj->dbg)
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->dbg.get();

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		auto &expr = expression_pool.emplace_derived<symbol_expression>(obj, dbg, sym);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_native(const function[], bool:local=true);
	AMX_DEFINE_NATIVE(expr_native, 1)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);
		auto &expr = optparam(2, 1) ? expression_pool.emplace_derived<local_native_expression>(amx, func, name) : expression_pool.emplace_derived<native_expression>(func, name);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_call(Expression:func, Expression:...);
	AMX_DEFINE_NATIVE(expr_call, 1)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			std::shared_ptr<expression> ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<call_expression>(amx, params, std::move(args));
	}

	// native Expression:expr_index(Expression:arr, Expression:...);
	AMX_DEFINE_NATIVE(expr_index, 1)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			std::shared_ptr<expression> ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<index_expression>(amx, params, std::move(args));
	}

	// native Expression:expr_add(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_add, 2)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator+>>(amx, params);
	}

	// native Expression:expr_sub(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_sub, 2)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator- >>(amx, params);
	}

	// native Expression:expr_mul(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_mul, 2)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator*>>(amx, params);
	}

	// native Expression:expr_div(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_div, 2)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator/>>(amx, params);
	}

	// native Expression:expr_mod(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_mod, 2)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator% >>(amx, params);
	}

	// native Expression:expr_neg(Expression:expr);
	AMX_DEFINE_NATIVE(expr_neg, 1)
	{
		return expr_unary<unary_object_expression<&dyn_object::operator- >>(amx, params);
	}

	// native Expression:expr_inc(Expression:expr);
	AMX_DEFINE_NATIVE(expr_inc, 1)
	{
		return expr_unary<unary_object_expression<&dyn_object::inc>>(amx, params);
	}

	// native Expression:expr_dec(Expression:expr);
	AMX_DEFINE_NATIVE(expr_dec, 1)
	{
		return expr_unary<unary_object_expression<&dyn_object::dec>>(amx, params);
	}

	// native Expression:expr_eq(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_eq, 2)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator==>>(amx, params);
	}

	// native Expression:expr_neq(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_neq, 2)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator!=>>(amx, params);
	}

	// native Expression:expr_lt(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_lt, 2)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator<>>(amx, params);
	}

	// native Expression:expr_gt(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_gt, 2)
	{
		return expr_binary<binary_logic_expression<(&dyn_object::operator>)>>(amx, params);
	}

	// native Expression:expr_lte(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_lte, 2)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator<=>>(amx, params);
	}

	// native Expression:expr_gte(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_gte, 2)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator>=>>(amx, params);
	}

	// native Expression:expr_not(Expression:expr);
	AMX_DEFINE_NATIVE(expr_not, 1)
	{
		return expr_unary<unary_logic_expression<&dyn_object::operator!>>(amx, params);
	}

	// native Expression:expr_and(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_and, 2)
	{
		return expr_binary<logic_and_expression>(amx, params);
	}

	// native Expression:expr_or(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE(expr_or, 2)
	{
		return expr_binary<logic_or_expression>(amx, params);
	}

	// native Expression:expr_cond(Expression:cond, Expression:on_true, Expression:on_false);
	AMX_DEFINE_NATIVE(expr_cond, 3)
	{
		std::shared_ptr<expression> ptr1;
		if(!expression_pool.get_by_id(params[1], ptr1)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		std::shared_ptr<expression> ptr2;
		if(!expression_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		std::shared_ptr<expression> ptr3;
		if(!expression_pool.get_by_id(params[3], ptr3)) amx_LogicError(errors::pointer_invalid, "expression", params[3]);
		auto &expr = expression_pool.emplace_derived<conditional_expression>(std::move(ptr1), std::move(ptr2), std::move(ptr3));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_cast(Expression:expr, TagTag:tag_id);
	AMX_DEFINE_NATIVE(expr_cast, 1)
	{
		return expr_unary<cast_expression>(amx, params, tags::find_tag(amx, params[2]));
	}

	// native Expression:expr_tagof(Expression:expr);
	AMX_DEFINE_NATIVE(expr_tagof, 1)
	{
		return expr_unary<tagof_expression>(amx, params);
	}

	// native Expression:expr_sizeof(Expression:expr, Expression:...);
	AMX_DEFINE_NATIVE(expr_sizeof, 1)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> indices;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			std::shared_ptr<expression> ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			indices.emplace_back(std::move(ptr));
		}
		return expr_unary<sizeof_expression>(amx, params, std::move(indices));
	}

	// native Expression:expr_rankof(Expression:expr);
	AMX_DEFINE_NATIVE(expr_rankof, 1)
	{
		return expr_unary<rankof_expression>(amx, params);
	}

	// native Expression:expr_value(Expression:var);
	AMX_DEFINE_NATIVE(expr_value, 1)
	{
		return expr_unary<extract_expression>(amx, params);
	}

	// native expr_get(Expression:expr, offset=0);
	AMX_DEFINE_NATIVE(expr_get, 2)
	{
		return value_at<2>::expr_get<dyn_func>(amx, params);
	}

	// native expr_get_arr(Expression:expr, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(expr_get_arr, 3)
	{
		return value_at<2, 3>::expr_get<dyn_func_arr>(amx, params);
	}

	// native String:expr_get_str_s(Expression:expr);
	AMX_DEFINE_NATIVE(expr_get_str_s, 1)
	{
		return value_at<>::expr_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:expr_get_var(Expression:expr);
	AMX_DEFINE_NATIVE(expr_get_var, 1)
	{
		return value_at<>::expr_get<dyn_func_var>(amx, params);
	}

	// native bool:expr_get_safe(Expression:expr, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_get_safe, 4)
	{
		return value_at<2, 3, 4>::expr_get<dyn_func>(amx, params);
	}

	// native expr_get_arr_safe(Expression:expr, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_get_arr_safe, 4)
	{
		return value_at<2, 3, 4>::expr_get<dyn_func_arr>(amx, params);
	}

	// native expr_get_str_safe(Expression:expr, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(expr_get_str_safe, 3)
	{
		return value_at<2, 3>::expr_get<dyn_func_str>(amx, params);
	}

	// native String:expr_get_str_safe_s(Expression:expr);
	AMX_DEFINE_NATIVE(expr_get_str_safe_s, 1)
	{
		return value_at<0>::expr_get<dyn_func_str_s>(amx, params);
	}

	// native expr_set(Expression:expr, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_set, 3)
	{
		return value_at<2, 3>::expr_set<dyn_func>(amx, params);
	}

	// native expr_set_arr(Expression:expr, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE(expr_set_arr, 4)
	{
		return value_at<2, 3, 4>::expr_set<dyn_func_arr>(amx, params);
	}

	// native expr_set_str(Expression:expr, const value[]);
	AMX_DEFINE_NATIVE(expr_set_str, 2)
	{
		return value_at<2>::expr_set<dyn_func_str>(amx, params);
	}

	// native expr_set_var(Expression:expr, VariantTag:value);
	AMX_DEFINE_NATIVE(expr_set_var, 2)
	{
		return value_at<2>::expr_set<dyn_func_var>(amx, params);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(expr_acquire),
	AMX_DECLARE_NATIVE(expr_release),
	AMX_DECLARE_NATIVE(expr_delete),
	AMX_DECLARE_NATIVE(expr_valid),

	AMX_DECLARE_NATIVE(expr_weak),

	AMX_DECLARE_NATIVE(expr_const),
	AMX_DECLARE_NATIVE(expr_const_arr),
	AMX_DECLARE_NATIVE(expr_const_str),
	AMX_DECLARE_NATIVE(expr_const_var),

	AMX_DECLARE_NATIVE(expr_arr),

	AMX_DECLARE_NATIVE(expr_arg),
	AMX_DECLARE_NATIVE(expr_bind),

	AMX_DECLARE_NATIVE(expr_comma),
	AMX_DECLARE_NATIVE(expr_assign),

	AMX_DECLARE_NATIVE(expr_try),

	AMX_DECLARE_NATIVE(expr_symbol),
	AMX_DECLARE_NATIVE(expr_native),
	AMX_DECLARE_NATIVE(expr_call),
	AMX_DECLARE_NATIVE(expr_index),

	AMX_DECLARE_NATIVE(expr_add),
	AMX_DECLARE_NATIVE(expr_sub),
	AMX_DECLARE_NATIVE(expr_mul),
	AMX_DECLARE_NATIVE(expr_div),
	AMX_DECLARE_NATIVE(expr_mod),

	AMX_DECLARE_NATIVE(expr_neg),
	AMX_DECLARE_NATIVE(expr_inc),
	AMX_DECLARE_NATIVE(expr_dec),

	AMX_DECLARE_NATIVE(expr_eq),
	AMX_DECLARE_NATIVE(expr_neq),
	AMX_DECLARE_NATIVE(expr_lt),
	AMX_DECLARE_NATIVE(expr_gt),
	AMX_DECLARE_NATIVE(expr_lte),
	AMX_DECLARE_NATIVE(expr_gte),
	AMX_DECLARE_NATIVE(expr_not),

	AMX_DECLARE_NATIVE(expr_and),
	AMX_DECLARE_NATIVE(expr_or),

	AMX_DECLARE_NATIVE(expr_cond),

	AMX_DECLARE_NATIVE(expr_cast),
	AMX_DECLARE_NATIVE(expr_tagof),
	AMX_DECLARE_NATIVE(expr_sizeof),
	AMX_DECLARE_NATIVE(expr_rankof),

	AMX_DECLARE_NATIVE(expr_value),

	AMX_DECLARE_NATIVE(expr_get),
	AMX_DECLARE_NATIVE(expr_get_arr),
	AMX_DECLARE_NATIVE(expr_get_str_s),
	AMX_DECLARE_NATIVE(expr_get_var),
	AMX_DECLARE_NATIVE(expr_get_safe),
	AMX_DECLARE_NATIVE(expr_get_arr_safe),
	AMX_DECLARE_NATIVE(expr_get_str_safe),
	AMX_DECLARE_NATIVE(expr_get_str_safe_s),

	AMX_DECLARE_NATIVE(expr_set),
	AMX_DECLARE_NATIVE(expr_set_arr),
	AMX_DECLARE_NATIVE(expr_set_str),
	AMX_DECLARE_NATIVE(expr_set_var),
};

int RegisterExprNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
