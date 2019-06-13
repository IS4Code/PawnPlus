#ifndef EXPRESSIONS_H_INCLUDED
#define EXPRESSIONS_H_INCLUDED

#include "objects/dyn_object.h"
#include "objects/object_pool.h"
#include "amxinfo.h"
#include "modules/strings.h"
#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"

#include <vector>
#include <memory>
#include <functional>

typedef std::shared_ptr<const class expression> expression_ptr;

class expression
{
public:
	typedef std::vector<std::reference_wrapper<const dyn_object>> args_type;
	typedef std::vector<dyn_object> call_args_type;

	virtual dyn_object execute(AMX *amx, const args_type &args) const = 0;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const;
	virtual tag_ptr get_tag(const args_type &args) const;
	virtual cell get_size(const args_type &args) const;
	virtual cell get_rank(const args_type &args) const;
	virtual void to_string(strings::cell_string &str) const = 0;

	virtual expression_ptr clone() const = 0;
	virtual ~expression() = default;
	int &operator[](size_t index) const;
};

extern object_pool<expression> expression_pool;

class expression_base : public expression, public object_pool<expression>::ref_container_virtual
{
public:
	virtual expression *get() override;
	virtual const expression *get() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual expression_ptr clone() const override;
};

class weak_expression : public expression_base
{
	std::weak_ptr<const expression> ptr;

public:
	weak_expression(std::weak_ptr<const expression> &&ptr) : ptr(std::move(ptr))
	{

	}

	weak_expression(const expression_ptr &ptr) : ptr(ptr)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual expression_ptr clone() const override;
};

class arg_expression : public expression_base
{
	size_t index;

public:
	arg_expression(size_t index) : index(index)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual expression_ptr clone() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual expression_ptr clone() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
};

class bind_expression : public expression_base, public unary_expression
{
	expression_ptr operand;
	std::vector<dyn_object> base_args;

protected:
	args_type combine_args(const args_type &args) const;

public:
	bind_expression(expression_ptr &&ptr, std::vector<dyn_object> &&base_args) : operand(std::move(ptr)), base_args(std::move(base_args))
	{

	}

	bind_expression(const expression_ptr &ptr, const std::vector<dyn_object> &base_args) : operand(ptr), base_args(base_args)
	{

	}

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual tag_ptr get_tag(const args_type &args) const override;
	virtual cell get_size(const args_type &args) const override;
	virtual cell get_rank(const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual expression_ptr clone() const override;
};

namespace impl
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override
	{
		return (operand->execute(amx, args).*Func)();
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.append(strings::convert(impl::unary_object_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const override
	{
		return operand;
	}

	virtual expression_ptr clone() const override
	{
		return std::make_shared<unary_object_expression<Func>>(*this);
	}
};

namespace impl
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
			return "+";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator*>
	{
	public:
		constexpr const char *operator()() const
		{
			return "-";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator/>
	{
	public:
		constexpr const char *operator()() const
		{
			return "*";
		}
	};

	template <>
	class binary_object_operator_name<&dyn_object::operator% >
	{
	public:
		constexpr const char *operator()() const
		{
			return "/";
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override
	{
		return (left->execute(amx, args).*Func)(right->execute(amx, args));
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(impl::binary_object_operator_name<Func>()()));
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

	virtual expression_ptr clone() const override
	{
		return std::make_shared<binary_object_expression<Func>>(*this);
	}
};

template <class Type, cell Tag>
class simple_expression : public expression_base
{
public:
	virtual Type execute_inner(AMX *amx, const args_type &args) const = 0;

	virtual dyn_object execute(AMX *amx, const args_type &args) const override
	{
		return dyn_object(execute_inner(amx, args), tags::find_tag(Tag));
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

namespace impl
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

	virtual bool execute_inner(AMX *amx, const args_type &args) const override
	{
		return (operand->execute(amx, args).*Func)();
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.append(strings::convert(impl::unary_logic_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const override
	{
		return operand;
	}

	virtual expression_ptr clone() const override
	{
		return std::make_shared<unary_logic_expression<Func>>(*this);
	}
};

namespace impl
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

	virtual bool execute_inner(AMX *amx, const args_type &args) const override
	{
		return (left->execute(amx, args).*Func)(right->execute(amx, args));
	}

	virtual void to_string(strings::cell_string &str) const override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(impl::binary_logic_operator_name<Func>()()));
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

	virtual expression_ptr clone() const override
	{
		return std::make_shared<binary_logic_expression<Func>>(*this);
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

	virtual bool execute_inner(AMX *amx, const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual expression_ptr clone() const override;
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

	virtual bool execute_inner(AMX *amx, const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_left() const override;
	virtual const expression_ptr &get_right() const override;
	virtual expression_ptr clone() const override;
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

	virtual dyn_object execute(AMX *amx, const args_type &args) const override;
	virtual dyn_object call(AMX *amx, const args_type &args, const call_args_type &call_args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const;
	virtual const expression_ptr &get_left() const;
	virtual const expression_ptr &get_right() const;
	virtual expression_ptr clone() const override;
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

	virtual cell execute_inner(AMX *amx, const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
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

	virtual cell execute_inner(AMX *amx, const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
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

	virtual cell execute_inner(AMX *amx, const args_type &args) const override;
	virtual void to_string(strings::cell_string &str) const override;
	virtual const expression_ptr &get_operand() const override;
	virtual expression_ptr clone() const override;
};

#endif
