#ifndef EXPRESSIONS_H_INCLUDED
#define EXPRESSIONS_H_INCLUDED

#include "objects/dyn_object.h"
#include "objects/object_pool.h"
#include "amxinfo.h"
#include "modules/strings.h"
#include "modules/containers.h"
#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"

#include <vector>
#include <memory>
#include <functional>
#include <tuple>

typedef std::shared_ptr<const class expression> expression_ptr;

class expression
{
public:
	typedef std::vector<std::reference_wrapper<const dyn_object>> args_type;
	typedef std::vector<dyn_object> call_args_type;
	typedef map_t env_type;

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const = 0;
	bool execute_bool(AMX *amx, const args_type &args, env_type &env) const;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const;
	virtual tag_ptr get_tag(const args_type &args) const;
	virtual cell get_size(const args_type &args) const;
	virtual cell get_rank(const args_type &args) const;
	virtual void to_string(strings::cell_string &str) const = 0;

	virtual ~expression() = default;
	int &operator[](size_t index) const;
};

extern object_pool<expression> expression_pool;

class expression_base : public expression, public object_pool<expression>::ref_container_virtual
{
public:
	virtual expression *get() override;
	virtual const expression *get() const override;
	virtual decltype(expression_pool)::object_ptr clone() const = 0;
};

class constant_expression : public expression_base
{
	dyn_object value;

public:
	constant_expression(dyn_object &&value) : value(std::move(value))
	{

	}

	constant_expression(const dyn_object &value) : value(value)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

	const dyn_object &get_value() const
	{
		return value;
	}
};

class weak_expression : public expression_base
{
public:
	std::weak_ptr<const expression> ptr;

	weak_expression(std::weak_ptr<const expression> &&ptr) : ptr(std::move(ptr))
	{

	}

	weak_expression(const expression_ptr &ptr) : ptr(ptr)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class arg_expression : public expression_base
{
	size_t index;

public:
	arg_expression(size_t index) : index(index)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class unary_expression
{
public:
	virtual const expression_ptr &get_operand() const = 0;
};

class binary_expression
{
public:
	virtual const expression_ptr &get_left() const = 0;
	virtual const expression_ptr &get_right() const = 0;
};

class nested_expression : public expression_base, public unary_expression
{
	expression_ptr expr;

public:
	nested_expression(expression_ptr &&expr) : expr(std::move(expr))
	{

	}

	nested_expression(const expression_ptr &expr) : expr(expr)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class env_set_expression : public expression_base, public unary_expression
{
	expression_ptr expr;
	std::shared_ptr<env_type> new_env;

public:
	env_set_expression(expression_ptr &&expr, std::shared_ptr<env_type> &&env) : expr(std::move(expr)), new_env(std::move(env))
	{

	}

	env_set_expression(const expression_ptr &expr, const std::shared_ptr<env_type> &env) : expr(expr), new_env(env)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class comma_expression : public expression_base, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	comma_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	comma_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class assign_expression : public expression_base, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	assign_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	assign_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class try_expression : public expression_base, public binary_expression
{
	expression_ptr main;
	expression_ptr fallback;

public:
	try_expression(expression_ptr &&main, expression_ptr &&fallback) : main(std::move(main)), fallback(std::move(fallback))
	{

	}

	try_expression(const expression_ptr &main, const expression_ptr &fallback) : main(main), fallback(fallback)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class call_expression : public expression_base, public unary_expression
{
	expression_ptr func;
	std::vector<expression_ptr> args;

public:
	call_expression(expression_ptr &&func, std::vector<expression_ptr> &&args) : func(std::move(func)), args(std::move(args))
	{

	}

	call_expression(const expression_ptr &func, const std::vector<expression_ptr> &args) : func(func), args(args)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class index_expression : public expression_base, public unary_expression
{
	expression_ptr arr;
	std::vector<expression_ptr> indices;

public:
	index_expression(expression_ptr &&arr, std::vector<expression_ptr> &&indices) : arr(std::move(arr)), indices(std::move(indices))
	{

	}

	index_expression(const expression_ptr &arr, const std::vector<expression_ptr> &indices) : arr(arr), indices(indices)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class bind_expression : public expression_base, public unary_expression
{
	expression_ptr operand;
	std::vector<expression_ptr> base_args;

protected:
	args_type combine_args(const args_type &args) const;
	args_type combine_args(AMX *amx, const args_type &args, env_type &env, call_args_type &storage) const;

public:
	bind_expression(expression_ptr &&ptr, std::vector<expression_ptr> &&base_args) : operand(std::move(ptr)), base_args(std::move(base_args))
	{

	}

	bind_expression(const expression_ptr &ptr, const std::vector<expression_ptr> &base_args) : operand(ptr), base_args(base_args)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class cast_expression : public expression_base, public unary_expression
{
	expression_ptr operand;
	tag_ptr new_tag;

public:
	cast_expression(expression_ptr &&ptr, tag_ptr new_tag) : operand(std::move(ptr)), new_tag(new_tag)
	{

	}

	cast_expression(const expression_ptr &ptr, tag_ptr new_tag) : operand(ptr), new_tag(new_tag)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class array_expression : public expression_base
{
	std::vector<expression_ptr> args;

public:
	array_expression(std::vector<expression_ptr> &&args) : args(std::move(args))
	{

	}

	array_expression(const std::vector<expression_ptr> &args) : args(args)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class global_expression : public expression_base
{
	strings::cell_string name;
	dyn_object key;

public:
	global_expression(std::string &&name) : name(strings::convert(name)), key(this->name.data(), this->name.size() + 1, tags::find_tag(tags::tag_char))
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class symbol_expression : public expression_base
{
	amx::handle target_amx;
	AMX_DBG *debug;
	AMX_DBG_SYMBOL *symbol;
	cell target_frm;

public:
	symbol_expression(const amx::object &amx, AMX_DBG *debug, AMX_DBG_SYMBOL *symbol) : target_amx(amx), debug(debug), symbol(symbol), target_frm(static_cast<AMX*>(*amx)->frm)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class native_expression : public expression_base
{
	AMX_NATIVE native;
	std::string name;

public:
	native_expression(AMX_NATIVE native, std::string &&name) : native(native), name(std::move(name))
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class local_native_expression : public native_expression
{
	amx::handle target_amx;

public:
	local_native_expression(AMX *amx, AMX_NATIVE native, std::string &&name) : target_amx(amx::load(amx)), native_expression(native, std::move(name))
	{

	}

	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

namespace expr_impl
{
	template <dyn_object(dyn_object::*Func)() const>
	class unary_object_operator_name;

	template <>
	class unary_object_operator_name<&dyn_object::operator- >
	{
	public:
		constexpr const char *operator()() const
		{
			return "-";
		}
	};

	template <>
	class unary_object_operator_name<&dyn_object::inc>
	{
	public:
		constexpr const char *operator()() const
		{
			return "++";
		}
	};

	template <>
	class unary_object_operator_name<&dyn_object::dec>
	{
	public:
		constexpr const char *operator()() const
		{
			return "--";
		}
	};

	template <>
	class unary_object_operator_name<&dyn_object::operator+>
	{
	public:
		constexpr const char *operator()() const
		{
			return "+";
		}
	};

	template <>
	class unary_object_operator_name<&dyn_object::operator~>
	{
	public:
		constexpr const char *operator()() const
		{
			return "~";
		}
	};
}

template <dyn_object(dyn_object::*Func)() const>
class unary_object_expression : public expression_base, public unary_expression
{
	expression_ptr operand;

public:
	unary_object_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	unary_object_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override
	{
		return (operand->execute(amx, args, env).*Func)();
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.append(strings::convert(expr_impl::unary_object_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const override
	{
		return operand;
	}

	virtual decltype(expression_pool)::object_ptr clone() const override
	{
		return expression_pool.emplace_derived<unary_object_expression<Func>>(*this);
	}
};

namespace expr_impl
{
	template <dyn_object(dyn_object::*Func)(const dyn_object&) const>
	class binary_object_operator_name;

	template <>
	class binary_object_operator_name<&dyn_object::operator+>
	{
	public:
		constexpr const char *operator()() const
		{
			return "+";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator- >
	{
	public:
		constexpr const char *operator()() const
		{
			return "-";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator*>
	{
	public:
		constexpr const char *operator()() const
		{
			return "*";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator/>
	{
	public:
		constexpr const char *operator()() const
		{
			return "/";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator% >
	{
	public:
		constexpr const char *operator()() const
		{
			return "%";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator&>
	{
	public:
		constexpr const char *operator()() const
		{
			return "&";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator|>
	{
	public:
		constexpr const char *operator()() const
		{
			return "|";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator^>
	{
	public:
		constexpr const char *operator()() const
		{
			return "^";
		}
	};

	template <>
	class binary_object_operator_name<(&dyn_object::operator>>)>
	{
	public:
		constexpr const char *operator()() const
		{
			return ">>";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator<<>
	{
	public:
		constexpr const char *operator()() const
		{
			return "<<";
		}
	};
}

template <dyn_object(dyn_object::*Func)(const dyn_object&) const>
class binary_object_expression : public expression_base, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	binary_object_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	binary_object_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override
	{
		return (left->execute(amx, args, env).*Func)(right->execute(amx, args, env));
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(expr_impl::binary_object_operator_name<Func>()()));
		right->to_string(str);
		str.push_back(')');
	}

	virtual const expression_ptr &get_left() const override
	{
		return left;
	}

	virtual const expression_ptr &get_right() const override
	{
		return right;
	}

	virtual decltype(expression_pool)::object_ptr clone() const override
	{
		return expression_pool.emplace_derived<binary_object_expression<Func>>(*this);
	}
};

template <class Type, cell Tag>
class simple_expression : public expression_base
{
public:
	virtual Type execute_inner(AMX *amx, const args_type &args, env_type &env) const = 0;

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override
	{
		return dyn_object(execute_inner(amx, args, env), tags::find_tag(Tag));
	}

	virtual tag_ptr get_tag(const args_type &args) const override
	{
		return tags::find_tag(Tag);
	}

	virtual cell get_size(const args_type &args) const override
	{
		return 1;
	}

	virtual cell get_rank(const args_type &args) const override
	{
		return 0;
	}
};

typedef simple_expression<cell, tags::tag_cell> cell_expression;
typedef simple_expression<bool, tags::tag_bool> bool_expression;

class quote_expression : public simple_expression<cell, tags::tag_expression>, public unary_expression
{
	expression_ptr operand;

public:
	quote_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	quote_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual cell execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class dequote_expression : public expression_base, public unary_expression
{
	expression_ptr operand;

	expression *get_expr(AMX *amx, const args_type &args, env_type &env) const;

public:
	dequote_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	dequote_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

namespace expr_impl
{
	template <bool(dyn_object::*Func)() const>
	class unary_logic_operator_name;

	template <>
	class unary_logic_operator_name<&dyn_object::operator!>
	{
	public:
		constexpr const char *operator()() const
		{
			return "!";
		}
	};
}

template <bool(dyn_object::*Func)() const>
class unary_logic_expression : public bool_expression, public unary_expression
{
	expression_ptr operand;

public:
	unary_logic_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	unary_logic_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual bool execute_inner(AMX *amx, const args_type &args, env_type &env) const override
	{
		return (operand->execute(amx, args, env).*Func)();
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.append(strings::convert(expr_impl::unary_logic_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const override
	{
		return operand;
	}

	virtual decltype(expression_pool)::object_ptr clone() const override
	{
		return expression_pool.emplace_derived<unary_logic_expression<Func>>(*this);
	}
};

namespace expr_impl
{
	template <bool(dyn_object::*Func)(const dyn_object&) const>
	class binary_logic_operator_name;

	template <>
	class binary_logic_operator_name<&dyn_object::operator==>
	{
	public:
		constexpr const char *operator()() const
		{
			return "==";
		}
	};

	template <>
	class binary_logic_operator_name<&dyn_object::operator!=>
	{
	public:
		constexpr const char *operator()() const
		{
			return "!=";
		}
	};

	template <>
	class binary_logic_operator_name<(&dyn_object::operator>)>
	{
	public:
		constexpr const char *operator()() const
		{
			return ">";
		}
	};

	template <>
	class binary_logic_operator_name<&dyn_object::operator<>
	{
	public:
		constexpr const char *operator()() const
		{
			return "<";
		}
	};

	template <>
	class binary_logic_operator_name<&dyn_object::operator>=>
	{
	public:
		constexpr const char *operator()() const
		{
			return ">=";
		}
	};

	template <>
	class binary_logic_operator_name<&dyn_object::operator<=>
	{
	public:
		constexpr const char *operator()() const
		{
			return "<=";
		}
	};
}

template <bool(dyn_object::*Func)(const dyn_object&) const>
class binary_logic_expression : public bool_expression, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	binary_logic_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	binary_logic_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual bool execute_inner(AMX *amx, const args_type &args, env_type &env) const override
	{
		return (left->execute(amx, args, env).*Func)(right->execute(amx, args, env));
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(expr_impl::binary_logic_operator_name<Func>()()));
		right->to_string(str);
		str.push_back(')');
	}

	virtual const expression_ptr &get_left() const override
	{
		return left;
	}

	virtual const expression_ptr &get_right() const override
	{
		return right;
	}

	virtual decltype(expression_pool)::object_ptr clone() const override
	{
		return expression_pool.emplace_derived<binary_logic_expression<Func>>(*this);
	}
};

class logic_and_expression : public bool_expression, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	logic_and_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	logic_and_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual bool execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class logic_or_expression : public bool_expression, public binary_expression
{
	expression_ptr left;
	expression_ptr right;

public:
	logic_or_expression(expression_ptr &&left, expression_ptr &&right) : left(std::move(left)), right(std::move(right))
	{

	}

	logic_or_expression(const expression_ptr &left, const expression_ptr &right) : left(left), right(right)
	{

	}

	virtual bool execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class conditional_expression : public expression_base, public unary_expression, public binary_expression
{
	expression_ptr cond;
	expression_ptr on_true;
	expression_ptr on_false;

public:
	conditional_expression(expression_ptr &&cond, expression_ptr &&on_true, expression_ptr &&on_false) : cond(std::move(cond)), on_true(std::move(on_true)), on_false(std::move(on_false))
	{

	}

	conditional_expression(const expression_ptr &cond, const expression_ptr &on_true, const expression_ptr &on_false) : cond(cond), on_true(on_true), on_false(on_false)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const override;
	virtual dyn_object assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const override;
	virtual dyn_object index(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const;
	virtual const expression_ptr &get_left() const;
	virtual const expression_ptr &get_right() const;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class tagof_expression : public cell_expression, public unary_expression
{
	expression_ptr operand;

public:
	tagof_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	tagof_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual cell execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class sizeof_expression : public cell_expression, public unary_expression
{
	expression_ptr operand;
	std::vector<expression_ptr> indices;

public:
	sizeof_expression(expression_ptr &&operand, std::vector<expression_ptr> &&indices) : operand(std::move(operand)), indices(std::move(indices))
	{

	}

	sizeof_expression(expression_ptr &&operand, const std::vector<expression_ptr> &indices) : operand(std::move(operand)), indices(indices)
	{

	}

	sizeof_expression(const expression_ptr &operand, std::vector<expression_ptr> &&indices) : operand(operand), indices(std::move(indices))
	{

	}

	sizeof_expression(const expression_ptr &operand, const std::vector<expression_ptr> &indices) : operand(operand), indices(indices)
	{

	}

	virtual cell execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class rankof_expression : public cell_expression, public unary_expression
{
	expression_ptr operand;

public:
	rankof_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	rankof_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual cell execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class addressof_expression : public expression_base, public unary_expression
{
	expression_ptr operand;

public:
	addressof_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	addressof_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class variant_value_expression : public expression_base, public unary_expression
{
	expression_ptr var;

public:
	variant_value_expression(expression_ptr &&var) : var(std::move(var))
	{

	}

	variant_value_expression(const expression_ptr &var) : var(var)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class variant_expression : public simple_expression<cell, tags::tag_variant>, public unary_expression
{
	expression_ptr value;

public:
	variant_expression(expression_ptr &&value) : value(std::move(value))
	{

	}

	variant_expression(const expression_ptr &value) : value(value)
	{

	}

	virtual cell execute_inner(AMX *amx, const args_type &args, env_type &env) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

#endif
