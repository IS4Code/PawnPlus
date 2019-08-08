#include "natives.h"
#include "modules/strings.h"
#include "modules/expressions.h"
#include "modules/parser.h"
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
		expression_ptr ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		amx::guard guard(amx);
		return Factory(amx, ptr->execute({}, expression::exec_info(amx, nullptr, true)), params[Indices]...);
	}

	// native expr_set(Expression:expr, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL expr_set(AMX *amx, cell *params)
	{
		expression_ptr ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		amx::guard guard(amx);
		ptr->assign({}, expression::exec_info(amx, nullptr, true), Factory(amx, params[Indices]...));
		return 1;
	}
};

namespace Natives
{
	// native Expression:expr_acquire(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_acquire, 1, expression)
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		if(!expression_pool.acquire_ref(*ptr)) amx_LogicError(errors::cannot_acquire, "expression", params[1]);
		return params[1];
	}

	// native Expression:expr_release(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_release, 1, expression)
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		if(!expression_pool.release_ref(*ptr)) amx_LogicError(errors::cannot_release, "expression", params[1]);
		return params[1];
	}

	// native expr_delete(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_delete, 1, cell)
	{
		if(!expression_pool.remove_by_id(params[1])) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		return 1;
	}

	// native bool:expr_valid(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_valid, 1, bool)
	{
		expression *ptr;
		return expression_pool.get_by_id(params[1], ptr);
	}

	// native expr_type(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_type, 1, cell)
	{
		expression *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		return reinterpret_cast<cell>(&typeid(*ptr));
	}

	// native expr_type_str(Expression:expr, type[], size=sizeof(type));
	AMX_DEFINE_NATIVE_TAG(expr_type_str, 3, cell)
	{
		expression *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		auto str = typeid(*ptr).name();
		amx_SetString(addr, str, false, false, params[3]);
		return std::strlen(str);
	}

	// native String:expr_type_str_s(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_type_str_s, 1, string)
	{
		expression *ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		return strings::create(typeid(*ptr).name());
	}

	template <class Iter>
	struct parse_base
	{
		cell operator()(Iter begin, Iter end, AMX *amx, cell options)
		{
			return expression_pool.get_id(expression_parser<Iter>(static_cast<parser_options>(options)).parse(amx, begin, end));
		}
	};

	// native Expression:expr_parse(const string[], parser_options:options=parser_all);
	AMX_DEFINE_NATIVE_TAG(expr_parse, 1, expression)
	{
		cell *str = amx_GetAddrSafe(amx, params[1]);
		return strings::select_iterator<parse_base>(str, amx, optparam(2, -1));
	}

	// native Expression:expr_parse_s(ConstString:string, parser_options:options=parser_all);
	AMX_DEFINE_NATIVE_TAG(expr_parse_s, 1, expression)
	{
		strings::cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		return strings::select_iterator<parse_base>(str, amx, optparam(2, -1));
	}

	// native Expression:expr_literal(const name[]);
	AMX_DEFINE_NATIVE_TAG(expr_literal, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "name");
		auto it = parser_symbols().find(name);
		if(it == parser_symbols().end())
		{
			amx_LogicError(errors::var_not_found, "literal", name);
		}
		auto &expr = dynamic_cast<const expression_base*>(it->second.get())->clone();
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_intrinsic(const name[], parser_options:options=parser_all);
	AMX_DEFINE_NATIVE_TAG(expr_intrinsic, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "name");
		std::string strname(name);
		auto it = parser_instrinsics().find(strname);
		if(it == parser_instrinsics().end())
		{
			amx_LogicError(errors::func_not_found, "intrinsic", name);
		}
		auto &expr = expression_pool.emplace_derived<intrinsic_expression>(reinterpret_cast<void*>(it->second), optparam(2, -1), std::move(strname));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_empty(Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_empty, 0, expression)
	{
		cell numargs = params[0] / sizeof(cell);
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[1 + i]);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		auto &expr = expression_pool.emplace_derived<empty_expression>(std::move(args));
		return expression_pool.get_id(expr);
	}

	template <class Expr, class... Args>
	static cell AMX_NATIVE_CALL expr_unary(AMX *amx, cell *params, Args &&...args)
	{
		expression_ptr ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		auto &expr = expression_pool.emplace_derived<Expr>(std::move(ptr), std::forward<Args>(args)...);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_void(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_void, 1, expression)
	{
		return expr_unary<void_expression>(amx, params);
	}

	// native Expression:expr_weak(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_weak, 1, expression)
	{
		return expr_unary<weak_expression>(amx, params);
	}

	// native Expression:expr_weak_set(Expression:weak, Expression:target);
	AMX_DEFINE_NATIVE_TAG(expr_weak_set, 2, expression)
	{
		std::shared_ptr<expression> ptr1;
		if(!expression_pool.get_by_id(params[1], ptr1)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		expression_ptr ptr2;
		if(!expression_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		auto weak = dynamic_cast<weak_expression*>(ptr1.get());
		if(!weak)
		{
			amx_LogicError(errors::invalid_expression, "must be a weak expression");
		}
		weak->ptr = ptr2;
		return params[1];
	}

	// native Expression:expr_const(AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_const, 2, expression)
	{
		return value_at<1, 2>::expr_const<dyn_func>(amx, params);
	}

	// native Expression:expr_const_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_const_arr, 3, expression)
	{
		return value_at<1, 2, 3>::expr_const<dyn_func_arr>(amx, params);
	}

	// native Expression:expr_const_str(const value[]);
	AMX_DEFINE_NATIVE_TAG(expr_const_str, 1, expression)
	{
		return value_at<1>::expr_const<dyn_func_str>(amx, params);
	}

	// native Expression:expr_const_var(VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(expr_const_var, 1, expression)
	{
		return value_at<1>::expr_const<dyn_func_var>(amx, params);
	}

	// native Expression:expr_true();
	AMX_DEFINE_NATIVE_TAG(expr_true, 0, expression)
	{
		auto &expr = expression_pool.emplace_derived<const_bool_expression<true>>();
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_false();
	AMX_DEFINE_NATIVE_TAG(expr_false, 0, expression)
	{
		auto &expr = expression_pool.emplace_derived<const_bool_expression<false>>();
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_handle(HandleTag:handle);
	AMX_DEFINE_NATIVE_TAG(expr_handle, 1, expression)
	{
		std::shared_ptr<handle_t> handle;
		if(!handle_pool.get_by_id(params[1], handle)) amx_LogicError(errors::pointer_invalid, "handle", params[1]);
		auto &expr = expression_pool.emplace_derived<handle_expression>(handle);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_arr(Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_arr, 0, expression)
	{
		cell numargs = params[0] / sizeof(cell);
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[1 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		auto &expr = expression_pool.emplace_derived<array_expression>(std::move(args));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_arg(index);
	AMX_DEFINE_NATIVE_TAG(expr_arg, 1, expression)
	{
		cell index = params[1];
		if(index < 0) amx_LogicError(errors::out_of_range, "index");
		auto &expr = expression_pool.emplace_derived<arg_expression>(index);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_arg_pack(begin, end=-1);
	AMX_DEFINE_NATIVE_TAG(expr_arg_pack, 1, expression)
	{
		cell begin = params[1];
		if(begin < 0) amx_LogicError(errors::out_of_range, "begin");
		cell end = optparam(2, -1);
		if(end < -1 || (end != -1 && end < begin)) amx_LogicError(errors::out_of_range, "end");
		auto &expr = expression_pool.emplace_derived<arg_pack_expression>(begin, end);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_bind(Expression:expr, Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_bind, 1, expression)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<bind_expression>(amx, params, std::move(args));
	}

	template <class Expr, class... Args>
	static cell AMX_NATIVE_CALL expr_binary(AMX *amx, cell *params, Args &&...args)
	{
		expression_ptr ptr1;
		if(!expression_pool.get_by_id(params[1], ptr1)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		expression_ptr ptr2;
		if(!expression_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		auto &expr = expression_pool.emplace_derived<Expr>(std::move(ptr1), std::move(ptr2), std::forward<Args>(args)...);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_nested(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_nested, 1, expression)
	{
		return expr_unary<nested_expression>(amx, params);
	}

	// native Expression:expr_env();
	AMX_DEFINE_NATIVE_TAG(expr_env, 0, expression)
	{
		auto &expr = expression_pool.emplace_derived<env_expression>();
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_set_env(Expression:expr, Map:env=INVALID_MAP, bool:env_readonly=false);
	AMX_DEFINE_NATIVE_TAG(expr_set_env, 1, expression)
	{
		std::shared_ptr<map_t> map;
		if(optparam(2, 0) && !map_pool.get_by_id(params[2], map)) amx_LogicError(errors::pointer_invalid, "map", params[2]);
		return expr_unary<env_set_expression>(amx, params, std::move(map), optparam(3, 0));
	}

	// native Expression:expr_global(const name[]);
	AMX_DEFINE_NATIVE_TAG(expr_global, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "name");
		auto &expr = expression_pool.emplace_derived<global_expression>(name);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_set_amx(Expression:expr, Amx:amx);
	AMX_DEFINE_NATIVE_TAG(expr_set_amx, 2, expression)
	{
		if(!params[2])
		{
			return expr_unary<amx_set_expression>(amx, params, amx::object());
		}
		AMX *set_amx = reinterpret_cast<AMX*>(params[2]);
		if(!amx::valid(set_amx))
		{
			amx_LogicError(errors::pointer_invalid, "AMX", params[2]);
		}
		return expr_unary<amx_set_expression>(amx, params, amx::load_lock(set_amx));
	}

	// native Expression:expr_comma(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_comma, 2, expression)
	{
		return expr_binary<comma_expression>(amx, params);
	}

	// native Expression:expr_assign(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_assign, 2, expression)
	{
		return expr_binary<assign_expression>(amx, params);
	}

	// native Expression:expr_try(Expression:main, Expression:fallback);
	AMX_DEFINE_NATIVE_TAG(expr_try, 2, expression)
	{
		return expr_binary<try_expression>(amx, params);
	}

	// native Expression:expr_symbol(Symbol:symbol);
	AMX_DEFINE_NATIVE_TAG(expr_symbol, 1, expression)
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
		ucell cip = amx->cip;
		if(sym->vclass == 1 && (sym->codestart > cip || cip >= sym->codeend))
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
		}
		auto &expr = expression_pool.emplace_derived<symbol_expression>(obj, dbg, sym);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_native(const function[]);
	AMX_DEFINE_NATIVE_TAG(expr_native, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);
		auto &expr = expression_pool.emplace_derived<native_expression>(func, name);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_public(const function[]);
	AMX_DEFINE_NATIVE_TAG(expr_public, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		int index;
		if(amx_FindPublicSafe(amx, name, &index) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public", name);
		}
		auto &expr = expression_pool.emplace_derived<public_expression>(name, index);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_pubvar(const name[]);
	AMX_DEFINE_NATIVE_TAG(expr_pubvar, 1, expression)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "name");
		cell amx_addr;
		if(amx_FindPubVar(amx, name, &amx_addr) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::var_not_found, "public", name);
		}
		auto &expr = expression_pool.emplace_derived<pubvar_expression>(name, last_pubvar_index, amx_addr);
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_call(Expression:func, Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_call, 1, expression)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<call_expression>(amx, params, std::move(args));
	}

	// native Expression:expr_index(Expression:arr, Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_index, 1, expression)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> args;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			args.emplace_back(std::move(ptr));
		}
		return expr_unary<index_expression>(amx, params, std::move(args));
	}

	// native Expression:expr_quote(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_quote, 1, expression)
	{
		return expr_unary<quote_expression>(amx, params);
	}

	// native Expression:expr_dequote(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_dequote, 1, expression)
	{
		return expr_unary<dequote_expression>(amx, params);
	}

	// native Expression:expr_add(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_add, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator+>>(amx, params);
	}

	// native Expression:expr_sub(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_sub, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator- >>(amx, params);
	}

	// native Expression:expr_mul(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_mul, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator*>>(amx, params);
	}

	// native Expression:expr_div(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_div, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator/>>(amx, params);
	}

	// native Expression:expr_mod(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_mod, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator% >>(amx, params);
	}

	// native Expression:expr_bit_and(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_bit_and, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator&>>(amx, params);
	}

	// native Expression:expr_bit_or(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_bit_or, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator|>>(amx, params);
	}

	// native Expression:expr_bit_xor(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_bit_xor, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator^>>(amx, params);
	}

	// native Expression:expr_rs(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_rs, 2, expression)
	{
		return expr_binary<binary_object_expression<(&dyn_object::operator>>)>>(amx, params);
	}

	// native Expression:expr_ls(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_ls, 2, expression)
	{
		return expr_binary<binary_object_expression<&dyn_object::operator<<>>(amx, params);
	}

	// native Expression:expr_neg(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_neg, 1, expression)
	{
		return expr_unary<unary_object_expression<&dyn_object::operator- >>(amx, params);
	}

	// native Expression:expr_inc(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_inc, 1, expression)
	{
		return expr_unary<unary_object_expression<&dyn_object::inc>>(amx, params);
	}

	// native Expression:expr_dec(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_dec, 1, expression)
	{
		return expr_unary<unary_object_expression<&dyn_object::dec>>(amx, params);
	}

	// native Expression:expr_bit_not(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_bit_not, 1, expression)
	{
		return expr_unary<unary_object_expression<&dyn_object::operator~>>(amx, params);
	}

	// native Expression:expr_eq(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_eq, 2, expression)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator==>>(amx, params);
	}

	// native Expression:expr_neq(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_neq, 2, expression)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator!=>>(amx, params);
	}

	// native Expression:expr_lt(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_lt, 2, expression)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator<>>(amx, params);
	}

	// native Expression:expr_gt(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_gt, 2, expression)
	{
		return expr_binary<binary_logic_expression<(&dyn_object::operator>)>>(amx, params);
	}

	// native Expression:expr_lte(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_lte, 2, expression)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator<=>>(amx, params);
	}

	// native Expression:expr_gte(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_gte, 2, expression)
	{
		return expr_binary<binary_logic_expression<&dyn_object::operator>=>>(amx, params);
	}

	// native Expression:expr_not(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_not, 1, expression)
	{
		return expr_unary<unary_logic_expression<&dyn_object::operator!>>(amx, params);
	}

	// native Expression:expr_and(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_and, 2, expression)
	{
		return expr_binary<logic_and_expression>(amx, params);
	}

	// native Expression:expr_or(Expression:left, Expression:right);
	AMX_DEFINE_NATIVE_TAG(expr_or, 2, expression)
	{
		return expr_binary<logic_or_expression>(amx, params);
	}

	// native Expression:expr_cond(Expression:cond, Expression:on_true, Expression:on_false);
	AMX_DEFINE_NATIVE_TAG(expr_cond, 3, expression)
	{
		expression_ptr ptr1;
		if(!expression_pool.get_by_id(params[1], ptr1)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		expression_ptr ptr2;
		if(!expression_pool.get_by_id(params[2], ptr2)) amx_LogicError(errors::pointer_invalid, "expression", params[2]);
		expression_ptr ptr3;
		if(!expression_pool.get_by_id(params[3], ptr3)) amx_LogicError(errors::pointer_invalid, "expression", params[3]);
		auto &expr = expression_pool.emplace_derived<conditional_expression>(std::move(ptr1), std::move(ptr2), std::move(ptr3));
		return expression_pool.get_id(expr);
	}

	// native Expression:expr_range(Expression:begin, Expression:end);
	AMX_DEFINE_NATIVE_TAG(expr_range, 2, expression)
	{
		return expr_binary<range_expression>(amx, params);
	}

	// native Expression:expr_select(Expression:list, Expression:func);
	AMX_DEFINE_NATIVE_TAG(expr_select, 2, expression)
	{
		return expr_binary<select_expression>(amx, params);
	}

	// native Expression:expr_where(Expression:list, Expression:code);
	AMX_DEFINE_NATIVE_TAG(expr_where, 2, expression)
	{
		return expr_binary<where_expression>(amx, params);
	}

	// native Expression:expr_cast(Expression:expr, TagTag:tag_id);
	AMX_DEFINE_NATIVE_TAG(expr_cast, 1, expression)
	{
		return expr_unary<cast_expression>(amx, params, tags::find_tag(amx, params[2]));
	}

	// native Expression:expr_tagof(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_tagof, 1, expression)
	{
		return expr_unary<tagof_expression>(amx, params);
	}

	// native Expression:expr_sizeof(Expression:expr, Expression:...);
	AMX_DEFINE_NATIVE_TAG(expr_sizeof, 1, expression)
	{
		cell numargs = (params[0] / sizeof(cell)) - 1;
		std::vector<expression_ptr> indices;
		for(cell i = 0; i < numargs; i++)
		{
			cell amx_addr = params[2 + i];
			cell *addr = amx_GetAddrSafe(amx, amx_addr);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			indices.emplace_back(std::move(ptr));
		}
		return expr_unary<sizeof_expression>(amx, params, std::move(indices));
	}

	// native Expression:expr_rankof(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_rankof, 1, expression)
	{
		return expr_unary<rankof_expression>(amx, params);
	}

	// native Expression:expr_addressof(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_addressof, 1, expression)
	{
		return expr_unary<addressof_expression>(amx, params);
	}

	// native Expression:expr_nameof(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_nameof, 1, expression)
	{
		return expr_unary<nameof_expression>(amx, params);
	}

	// native Expression:expr_extract(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_extract, 1, expression)
	{
		return expr_unary<extract_expression>(amx, params);
	}

	// native Expression:expr_variant(Expression:value);
	AMX_DEFINE_NATIVE_TAG(expr_variant, 1, expression)
	{
		return expr_unary<variant_expression>(amx, params);
	}

	// native Expression:expr_string(Expression:value);
	AMX_DEFINE_NATIVE_TAG(expr_string, 1, expression)
	{
		return expr_unary<string_expression>(amx, params);
	}

	// native expr_get(Expression:expr, offset=0);
	AMX_DEFINE_NATIVE(expr_get, 2)
	{
		return value_at<2>::expr_get<dyn_func>(amx, params);
	}

	// native expr_get_arr(Expression:expr, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(expr_get_arr, 3, cell)
	{
		return value_at<2, 3>::expr_get<dyn_func_arr>(amx, params);
	}

	// native String:expr_get_str_s(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_get_str_s, 1, string)
	{
		return value_at<>::expr_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:expr_get_var(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_get_var, 1, variant)
	{
		return value_at<>::expr_get<dyn_func_var>(amx, params);
	}

	// native bool:expr_get_safe(Expression:expr, &AnyTag:value, offset=0, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_get_safe, 4, bool)
	{
		return value_at<2, 3, 4>::expr_get<dyn_func>(amx, params);
	}

	// native expr_get_arr_safe(Expression:expr, AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_get_arr_safe, 4, cell)
	{
		return value_at<2, 3, 4>::expr_get<dyn_func_arr>(amx, params);
	}

	// native expr_get_str_safe(Expression:expr, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE_TAG(expr_get_str_safe, 3, cell)
	{
		return value_at<2, 3>::expr_get<dyn_func_str>(amx, params);
	}

	// native String:expr_get_str_safe_s(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_get_str_safe_s, 1, string)
	{
		return value_at<0>::expr_get<dyn_func_str_s>(amx, params);
	}

	// native expr_set(Expression:expr, AnyTag:value, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_set, 3, cell)
	{
		return value_at<2, 3>::expr_set<dyn_func>(amx, params);
	}

	// native expr_set_arr(Expression:expr, const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(expr_set_arr, 4, cell)
	{
		return value_at<2, 3, 4>::expr_set<dyn_func_arr>(amx, params);
	}

	// native expr_set_str(Expression:expr, const value[]);
	AMX_DEFINE_NATIVE_TAG(expr_set_str, 2, cell)
	{
		return value_at<2>::expr_set<dyn_func_str>(amx, params);
	}

	// native expr_set_var(Expression:expr, VariantTag:value);
	AMX_DEFINE_NATIVE_TAG(expr_set_var, 2, cell)
	{
		return value_at<2>::expr_set<dyn_func_var>(amx, params);
	}

	// native expr_exec(Expression:expr);
	AMX_DEFINE_NATIVE_TAG(expr_exec, 1, cell)
	{
		expression_ptr ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		amx::guard guard(amx);
		ptr->execute_discard({}, expression::exec_info(amx, nullptr, true));
		return 1;
	}

	// native expr_exec_multi(Expression:expr, List:output);
	AMX_DEFINE_NATIVE_TAG(expr_exec_multi, 2, cell)
	{
		expression_ptr ptr;
		if(!expression_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "expression", params[1]);
		std::shared_ptr<list_t> list;
		if(!list_pool.get_by_id(params[2], list)) amx_LogicError(errors::pointer_invalid, "list", params[2]);
		amx::guard guard(amx);
		size_t count = list->size();
		ptr->execute_multi({}, expression::exec_info(amx, nullptr, true), list->get_data());
		return list->size() - count;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(expr_acquire),
	AMX_DECLARE_NATIVE(expr_release),
	AMX_DECLARE_NATIVE(expr_delete),
	AMX_DECLARE_NATIVE(expr_valid),
	AMX_DECLARE_NATIVE(expr_type),
	AMX_DECLARE_NATIVE(expr_type_str),
	AMX_DECLARE_NATIVE(expr_type_str_s),

	AMX_DECLARE_NATIVE(expr_empty),
	AMX_DECLARE_NATIVE(expr_void),

	AMX_DECLARE_NATIVE(expr_weak),
	AMX_DECLARE_NATIVE(expr_weak_set),

	AMX_DECLARE_NATIVE(expr_const),
	AMX_DECLARE_NATIVE(expr_const_arr),
	AMX_DECLARE_NATIVE(expr_const_str),
	AMX_DECLARE_NATIVE(expr_const_var),
	AMX_DECLARE_NATIVE(expr_true),
	AMX_DECLARE_NATIVE(expr_false),
	AMX_DECLARE_NATIVE(expr_handle),

	AMX_DECLARE_NATIVE(expr_arr),

	AMX_DECLARE_NATIVE(expr_arg),
	AMX_DECLARE_NATIVE(expr_arg_pack),
	AMX_DECLARE_NATIVE(expr_bind),
	AMX_DECLARE_NATIVE(expr_nested),
	AMX_DECLARE_NATIVE(expr_env),
	AMX_DECLARE_NATIVE(expr_set_env),
	AMX_DECLARE_NATIVE(expr_global),
	AMX_DECLARE_NATIVE(expr_set_amx),

	AMX_DECLARE_NATIVE(expr_comma),
	AMX_DECLARE_NATIVE(expr_assign),

	AMX_DECLARE_NATIVE(expr_try),

	AMX_DECLARE_NATIVE(expr_symbol),
	AMX_DECLARE_NATIVE(expr_native),
	AMX_DECLARE_NATIVE(expr_public),
	AMX_DECLARE_NATIVE(expr_pubvar),
	AMX_DECLARE_NATIVE(expr_call),
	AMX_DECLARE_NATIVE(expr_index),

	AMX_DECLARE_NATIVE(expr_quote),
	AMX_DECLARE_NATIVE(expr_dequote),

	AMX_DECLARE_NATIVE(expr_add),
	AMX_DECLARE_NATIVE(expr_sub),
	AMX_DECLARE_NATIVE(expr_mul),
	AMX_DECLARE_NATIVE(expr_div),
	AMX_DECLARE_NATIVE(expr_mod),

	AMX_DECLARE_NATIVE(expr_bit_and),
	AMX_DECLARE_NATIVE(expr_bit_or),
	AMX_DECLARE_NATIVE(expr_bit_xor),
	AMX_DECLARE_NATIVE(expr_rs),
	AMX_DECLARE_NATIVE(expr_ls),

	AMX_DECLARE_NATIVE(expr_neg),
	AMX_DECLARE_NATIVE(expr_inc),
	AMX_DECLARE_NATIVE(expr_dec),

	AMX_DECLARE_NATIVE(expr_bit_not),

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

	AMX_DECLARE_NATIVE(expr_range),
	AMX_DECLARE_NATIVE(expr_select),
	AMX_DECLARE_NATIVE(expr_where),

	AMX_DECLARE_NATIVE(expr_cast),
	AMX_DECLARE_NATIVE(expr_tagof),
	AMX_DECLARE_NATIVE(expr_sizeof),
	AMX_DECLARE_NATIVE(expr_rankof),
	AMX_DECLARE_NATIVE(expr_addressof),
	AMX_DECLARE_NATIVE(expr_nameof),

	AMX_DECLARE_NATIVE(expr_extract),
	AMX_DECLARE_NATIVE(expr_variant),
	AMX_DECLARE_NATIVE(expr_string),

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

	AMX_DECLARE_NATIVE(expr_exec),
	AMX_DECLARE_NATIVE(expr_exec_multi),

	AMX_DECLARE_NATIVE(expr_parse),
	AMX_DECLARE_NATIVE(expr_parse_s),
	AMX_DECLARE_NATIVE(expr_literal),
	AMX_DECLARE_NATIVE(expr_intrinsic),
};

int RegisterExprNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
