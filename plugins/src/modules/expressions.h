#ifndef EXPRESSIONS_H_INCLUDED
#define EXPRESSIONS_H_INCLUDED

#include "errors.h"
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

	struct exec_info
	{
		AMX *amx;
		map_t *env;
		bool env_readonly;

		exec_info() : amx(nullptr), env(nullptr), env_readonly(true)
		{

		}

		exec_info(AMX *amx) : amx(amx), env(nullptr), env_readonly(true)
		{

		}

		exec_info(map_t *env, bool env_readonly) : amx(nullptr), env(env), env_readonly(env_readonly)
		{

		}

		exec_info(AMX *amx, map_t *env, bool env_readonly) : amx(amx), env(env), env_readonly(env_readonly)
		{

		}

		exec_info(const exec_info &obj, map_t *env, bool env_readonly) : amx(obj.amx), env(env), env_readonly(env_readonly)
		{

		}

		exec_info(const exec_info &obj, bool env_readonly) : amx(obj.amx), env(obj.env), env_readonly(env_readonly)
		{

		}

		exec_info(const exec_info &obj, AMX *amx) : amx(amx), env(obj.env), env_readonly(obj.env_readonly)
		{

		}
	};

	virtual dyn_object execute(const args_type &args, const exec_info &info) const = 0;
	virtual void execute_discard(const args_type &args, const exec_info &info) const;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const;
	bool execute_bool(const args_type &args, const exec_info &info) const;
	expression_ptr execute_expression(const args_type &args, const exec_info &info) const;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const;
	virtual tag_ptr get_tag(const args_type &args) const noexcept;
	virtual cell get_size(const args_type &args) const noexcept;
	virtual cell get_rank(const args_type &args) const noexcept;
	virtual cell get_count(const args_type &args) const noexcept;
	virtual void to_string(strings::cell_string &str) const noexcept = 0;

	virtual ~expression() = default;
	int &operator[](size_t index) const;

protected:
	void checkstack() const;
};

extern object_pool<expression> expression_pool;

class expression_base : public expression, public object_pool<expression>::ref_container_virtual
{
public:
	virtual expression *get() override;
	virtual const expression *get() const override;
	virtual decltype(expression_pool)::object_ptr clone() const = 0;
};

class object_expression
{
public:
	virtual const dyn_object &get_value() const = 0;
};

class constant_expression : public expression_base, public object_expression
{
	dyn_object value;

public:
	constant_expression(dyn_object &&value) : value(std::move(value))
	{

	}

	constant_expression(const dyn_object &value) : value(value)
	{

	}

	template <class... Args>
	constant_expression(Args &&...args) : value(std::forward<Args>(args)...)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

	virtual const dyn_object &get_value() const override
	{
		return value;
	}
};

class handle_expression : public expression_base, public object_expression
{
	std::shared_ptr<handle_t> handle;

public:
	handle_expression(std::shared_ptr<handle_t> &&handle) : handle(std::move(handle))
	{

	}

	handle_expression(const std::shared_ptr<handle_t> &handle) : handle(handle)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

	virtual const dyn_object &get_value() const override
	{
		return handle->get();
	}
};

class proxy_expression : public expression_base
{
public:
	virtual dyn_object execute(const args_type &args, const exec_info &info) const override = 0;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override = 0;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override = 0;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override = 0;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override = 0;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override = 0;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override = 0;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override = 0;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override = 0;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override = 0;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override = 0;
	virtual cell get_size(const args_type &args) const noexcept override = 0;
	virtual cell get_rank(const args_type &args) const noexcept override = 0;
	virtual cell get_count(const args_type &args) const noexcept override = 0;
	virtual void to_string(strings::cell_string &str) const noexcept override = 0;
	virtual decltype(expression_pool)::object_ptr clone() const override = 0;
};

class weak_expression : public proxy_expression
{
public:
	std::weak_ptr<const expression> ptr;

	weak_expression(std::weak_ptr<const expression> &&ptr) : ptr(std::move(ptr))
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

protected:
	expression_ptr lock() const;
};

class arg_expression : public expression_base
{
	size_t index;

public:
	arg_expression(size_t index) : index(index)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

protected:
	const dyn_object &arg(const args_type &args) const;
};

class sequence_expression : public expression_base
{
public:
	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const = 0;
	virtual cell get_count(const args_type &args) const noexcept override = 0;
};

class empty_expression : public sequence_expression
{
	std::vector<expression_ptr> captures;

public:
	static expression_ptr instance;

	empty_expression()
	{

	}

	empty_expression(std::vector<expression_ptr> &&captures) : captures(std::move(captures))
	{

	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class void_expression : public sequence_expression
{
	expression_ptr expr;

public:
	void_expression(expression_ptr &&expr) : expr(std::move(expr))
	{

	}

	void_expression(const expression_ptr &expr) : expr(expr)
	{

	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class arg_pack_expression : public sequence_expression
{
	size_t begin;
	size_t end;

public:
	arg_pack_expression(size_t begin) : begin(begin), end(-1)
	{

	}

	arg_pack_expression(size_t begin, size_t end) : begin(begin), end(end)
	{

	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class unary_expression
{
public:
	virtual const expression_ptr &get_operand() const noexcept = 0;
};

class binary_expression
{
public:
	virtual const expression_ptr &get_left() const noexcept = 0;
	virtual const expression_ptr &get_right() const noexcept = 0;
};

class range_expression : public sequence_expression, public binary_expression
{
	expression_ptr begin;
	expression_ptr end;

public:
	range_expression(expression_ptr &&begin, expression_ptr &&end) : begin(std::move(begin)), end(std::move(end))
	{

	}

	range_expression(const expression_ptr &begin, const expression_ptr &end) : begin(begin), end(end)
	{

	}

	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class env_expression : public expression_base
{
public:
	env_expression()
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class info_set_expression : public proxy_expression, public unary_expression
{
protected:
	expression_ptr expr;

public:
	info_set_expression(expression_ptr &&expr) : expr(std::move(expr))
	{

	}

	info_set_expression(const expression_ptr &expr) : expr(expr)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;

protected:
	virtual exec_info get_info(const exec_info &info) const = 0;
};

class env_set_expression : public info_set_expression
{
	std::weak_ptr<map_t> new_env;
	bool readonly;

public:
	env_set_expression(expression_ptr &&expr, std::weak_ptr<map_t> &&env, bool readonly) : info_set_expression(std::move(expr)), new_env(std::move(env)), readonly(readonly)
	{

	}

	env_set_expression(const expression_ptr &expr, const std::weak_ptr<map_t> &env, bool readonly) : info_set_expression(expr), new_env(env), readonly(readonly)
	{

	}

	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

protected:
	virtual exec_info get_info(const exec_info &info) const override;
};

class amx_set_expression : public info_set_expression
{
	amx::handle target_amx;

public:
	amx_set_expression(expression_ptr &&expr, const amx::object &amx) : info_set_expression(std::move(expr)), target_amx(amx)
	{

	}

	amx_set_expression(const expression_ptr &expr, const amx::object &amx) : info_set_expression(expr), target_amx(amx)
	{

	}

	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;

protected:
	virtual exec_info get_info(const exec_info &info) const override;
};

class comma_expression : public proxy_expression, public binary_expression
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class try_expression : public proxy_expression, public binary_expression
{
	expression_ptr main;
	expression_ptr fallback;

	dyn_object get_error_obj(const errors::native_error &err) const;
	dyn_object get_error_obj(const errors::amx_error &err) const;
	args_type create_args(const dyn_object &err_obj, const args_type &args) const;

public:
	try_expression(expression_ptr &&main, expression_ptr &&fallback) : main(std::move(main)), fallback(std::move(fallback))
	{

	}

	try_expression(const expression_ptr &main, const expression_ptr &fallback) : main(main), fallback(fallback)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class bind_expression : public proxy_expression, public unary_expression
{
	expression_ptr operand;
	std::vector<expression_ptr> base_args;

protected:
	args_type combine_args(const args_type &args) const;
	args_type combine_args(const args_type &args, const exec_info &info, call_args_type &storage) const;

public:
	bind_expression(expression_ptr &&ptr, std::vector<expression_ptr> &&base_args) : operand(std::move(ptr)), base_args(std::move(base_args))
	{

	}

	bind_expression(const expression_ptr &ptr, const std::vector<expression_ptr> &base_args) : operand(ptr), base_args(base_args)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class public_expression : public expression_base
{
	mutable int index;
	std::string name;

	void load(AMX *amx) const;

public:
	public_expression(std::string &&name, int index) : name(std::move(name)), index(index)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class pubvar_expression : public expression_base
{
	mutable int index;
	std::string name;
	mutable cell amx_addr;

	void load(AMX *amx) const;

public:
	pubvar_expression(std::string &&name, int index, cell amx_addr) : name(std::move(name)), index(index), amx_addr(amx_addr)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class intrinsic_expression : public expression_base
{
	void *func;
	cell options;
	std::string name;

public:
	intrinsic_expression(void *func, cell options, std::string &&name) : func(func), options(options), name(std::move(name))
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override
	{
		return (operand->execute(args, info).*Func)();
	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override
	{
		operand->execute_discard(args, info);
	}

	virtual cell get_count(const args_type &args) const noexcept override
	{
		return 1;
	}

	virtual void to_string(strings::cell_string &str) const noexcept override
	{
		str.append(strings::convert(expr_impl::unary_object_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const noexcept override
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override
	{
		dyn_object left_op = left->execute(args, info);
		return (left_op.*Func)(right->execute(args, info));
	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override
	{
		left->execute_discard(args, info);
		right->execute_discard(args, info);
	}

	virtual cell get_count(const args_type &args) const noexcept override
	{
		return 1;
	}

	virtual void to_string(strings::cell_string &str) const noexcept override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(expr_impl::binary_object_operator_name<Func>()()));
		right->to_string(str);
		str.push_back(')');
	}

	virtual const expression_ptr &get_left() const noexcept override
	{
		return left;
	}

	virtual const expression_ptr &get_right() const noexcept override
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
	virtual Type execute_inner(const args_type &args, const exec_info &info) const = 0;

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override
	{
		return dyn_object(execute_inner(args, info), tags::find_tag(Tag));
	}

	virtual void execute_discard(const args_type &args, const exec_info &info) const override
	{
		execute_inner(args, info);
	}

	virtual tag_ptr get_tag(const args_type &args) const noexcept override
	{
		return tags::find_tag(Tag);
	}

	virtual cell get_size(const args_type &args) const noexcept override
	{
		return 1;
	}

	virtual cell get_rank(const args_type &args) const noexcept override
	{
		return 0;
	}

	virtual cell get_count(const args_type &args) const noexcept override
	{
		return 1;
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

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class dequote_expression : public proxy_expression, public unary_expression
{
	expression_ptr operand;

	expression *get_expr(const args_type &args, const exec_info &info) const;

public:
	dequote_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	dequote_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

template <bool Value>
class const_bool_expression : public bool_expression
{
public:
	const_bool_expression()
	{

	}

	virtual bool execute_inner(const args_type &args, const exec_info &info) const override
	{
		return Value;
	}

	virtual void to_string(strings::cell_string &str) const noexcept override
	{
		str.append(strings::convert(Value ? "true" : "false"));
	}

	virtual decltype(expression_pool)::object_ptr clone() const override
	{
		return expression_pool.emplace_derived<const_bool_expression<Value>>(*this);
	}
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

	virtual bool execute_inner(const args_type &args, const exec_info &info) const override
	{
		return (operand->execute(args, info).*Func)();
	}

	virtual void to_string(strings::cell_string &str) const noexcept override
	{
		str.append(strings::convert(expr_impl::unary_logic_operator_name<Func>()()));
		operand->to_string(str);
	}

	virtual const expression_ptr &get_operand() const noexcept override
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

	virtual bool execute_inner(const args_type &args, const exec_info &info) const override
	{
		return (left->execute(args, info).*Func)(right->execute(args, info));
	}

	virtual void to_string(strings::cell_string &str) const noexcept override
	{
		str.push_back('(');
		left->to_string(str);
		str.append(strings::convert(expr_impl::binary_logic_operator_name<Func>()()));
		right->to_string(str);
		str.push_back(')');
	}

	virtual const expression_ptr &get_left() const noexcept override
	{
		return left;
	}

	virtual const expression_ptr &get_right() const noexcept override
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

	virtual bool execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
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

	virtual bool execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept override;
	virtual const expression_ptr &get_right() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class conditional_expression : public proxy_expression, public unary_expression, public binary_expression
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual dyn_object call(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_discard(const args_type &args, const exec_info &info, const call_args_type &call_args) const override;
	virtual void call_multi(const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual std::tuple<cell*, size_t, tag_ptr> address(const args_type &args, const exec_info &info, const call_args_type &indices) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept;
	virtual const expression_ptr &get_left() const noexcept;
	virtual const expression_ptr &get_right() const noexcept;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class select_expression : public sequence_expression, public binary_expression
{
	expression_ptr list;
	expression_ptr func;

public:
	select_expression(expression_ptr &&list, expression_ptr &&func) : list(std::move(list)), func(std::move(func))
	{

	}

	select_expression(const expression_ptr &list, const expression_ptr &func) : list(list), func(func)
	{

	}

	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept;
	virtual const expression_ptr &get_right() const noexcept;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class where_expression : public sequence_expression, public binary_expression
{
	expression_ptr list;
	expression_ptr func;

public:
	where_expression(expression_ptr &&list, expression_ptr &&func) : list(std::move(list)), func(std::move(func))
	{

	}

	where_expression(const expression_ptr &list, const expression_ptr &func) : list(list), func(func)
	{

	}

	virtual void execute_multi(const args_type &args, const exec_info &info, call_args_type &output) const override;
	virtual void execute_discard(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_left() const noexcept;
	virtual const expression_ptr &get_right() const noexcept;
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

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class nameof_expression : public expression_base, public unary_expression
{
	expression_ptr operand;

public:
	nameof_expression(expression_ptr &&operand) : operand(std::move(operand))
	{

	}

	nameof_expression(const expression_ptr &operand) : operand(operand)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual tag_ptr get_tag(const args_type &args) const noexcept override;
	virtual cell get_size(const args_type &args) const noexcept override;
	virtual cell get_rank(const args_type &args) const noexcept override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class extract_expression : public expression_base, public unary_expression
{
	expression_ptr var;

public:
	extract_expression(expression_ptr &&var) : var(std::move(var))
	{

	}

	extract_expression(const expression_ptr &var) : var(var)
	{

	}

	virtual dyn_object execute(const args_type &args, const exec_info &info) const override;
	virtual dyn_object assign(const args_type &args, const exec_info &info, dyn_object &&value) const override;
	virtual dyn_object index_assign(const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const override;
	virtual cell get_count(const args_type &args) const noexcept override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
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

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

class string_expression : public simple_expression<cell, tags::tag_string>, public unary_expression
{
	expression_ptr value;

public:
	string_expression(expression_ptr &&value) : value(std::move(value))
	{

	}

	string_expression(const expression_ptr &value) : value(value)
	{

	}

	virtual cell execute_inner(const args_type &args, const exec_info &info) const override;
	virtual void to_string(strings::cell_string &str) const noexcept override;
	virtual const expression_ptr &get_operand() const noexcept override;
	virtual decltype(expression_pool)::object_ptr clone() const override;
};

#endif
