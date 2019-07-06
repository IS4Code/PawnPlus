#include "expressions.h"
#include "tags.h"
#include "errors.h"
#include "modules/variants.h"
#include "utils/systools.h"

#include <string>
#include <stdarg.h>
#include <cstring>

object_pool<expression> expression_pool;

void amx_ExpressionError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	std::string message(vsnprintf(NULL, 0, format, args), '\0');
	vsprintf(&message[0], format, args);
	va_end(args);
	amx_LogicError(errors::invalid_expression, message.c_str());
}

void expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	execute(amx, args, info);
}

void expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	output.push_back(execute(amx, args, info));
}

bool expression::execute_bool(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto bool_exp = dynamic_cast<const bool_expression*>(this);
	if(bool_exp)
	{
		return bool_exp->execute_inner(amx, args, info);
	}
	return !!execute(amx, args, info);
}

expression_ptr expression::execute_expression(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto result = execute(amx, args, info);
	std::shared_ptr<expression> ptr;
	if(!(result.tag_assignable(tags::find_tag(tags::tag_expression))) || !result.is_cell() || !expression_pool.get_by_id(result.get_cell(0), ptr))
	{
		return {};
	}
	return ptr;
}

dyn_object expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	checkstack();
	auto ptr = execute_expression(amx, args, info);
	if(!ptr)
	{
		amx_ExpressionError("attempt to call a non-function expression");
	}
	args_type new_args;
	new_args.reserve(call_args.size());
	for(const auto &arg : call_args)
	{
		new_args.push_back(std::cref(arg));
	}
	return ptr->execute(amx, new_args, info);
}

void expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	checkstack();
	auto ptr = execute_expression(amx, args, info);
	if(!ptr)
	{
		amx_ExpressionError("attempt to call a non-function expression");
	}
	args_type new_args;
	new_args.reserve(call_args.size());
	for(const auto &arg : call_args)
	{
		new_args.push_back(std::cref(arg));
	}
	ptr->execute_discard(amx, new_args, info);
}

void expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	checkstack();
	auto ptr = execute_expression(amx, args, info);
	if(!ptr)
	{
		amx_ExpressionError("attempt to call a non-function expression");
	}
	args_type new_args;
	new_args.reserve(call_args.size());
	for(const auto &arg : call_args)
	{
		new_args.push_back(std::cref(arg));
	}
	ptr->execute_multi(amx, new_args, info, output);
}

dyn_object expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	amx_ExpressionError("attempt to assign to a non-lvalue expression");
	return {};
}

dyn_object expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	auto value = execute(amx, args, info);
	if(value.is_cell())
	{
		std::shared_ptr<expression> ptr;
		tag_ptr tag = tags::find_tag(tags::tag_expression);
		if(!(value.tag_assignable(tag)) || !expression_pool.get_by_id(value.get_cell(0), ptr))
		{
			amx_ExpressionError("attempt to index a non-array expression");
		}
		std::vector<expression_ptr> args;
		for(const auto &arg : indices)
		{
			args.push_back(std::make_shared<constant_expression>(arg));
		}
		return dyn_object(expression_pool.get_id(expression_pool.emplace_derived<bind_expression>(std::move(ptr), std::move(args))), tag);
	}
	std::vector<cell> cell_indices;
	for(const auto &index : indices)
	{
		if(!(index.tag_assignable(tags::find_tag(tags::tag_cell))))
		{
			amx_ExpressionError("index tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), index.get_tag()->format_name());
		}
		if(index.get_rank() != 0)
		{
			amx_ExpressionError("index operation requires a single cell value (value of rank %d provided)", index.get_rank());
		}
		cell_indices.push_back(index.get_cell(0));
	}
	return dyn_object(value.get_cell(cell_indices.data(), cell_indices.size()), value.get_tag());
}

dyn_object expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("indexed assignment operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	auto addr = address(amx, args, info, indices);
	if(std::get<1>(addr) == 0)
	{
		amx_ExpressionError("index out of bounds");
	}
	if(!(value.tag_assignable(std::get<2>(addr))))
	{
		amx_ExpressionError("assigned value tag mismatch (%s: required, %s: provided)", std::get<2>(addr)->format_name(), value.get_tag()->format_name());
	}
	return dyn_object(std::get<0>(addr)[0] = value.get_cell(0), std::get<2>(addr));
}

std::tuple<cell*, size_t, tag_ptr> expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	std::vector<cell> cell_indices;
	for(const auto &index : indices)
	{
		if(!(index.tag_assignable(tags::find_tag(tags::tag_cell))))
		{
			amx_ExpressionError("index tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), index.get_tag()->format_name());
		}
		if(index.get_rank() != 0)
		{
			amx_ExpressionError("index operation requires a single cell value (value of rank %d provided)", index.get_rank());
		}
		cell_indices.push_back(index.get_cell(0));
	}
	auto value = execute(amx, args, info);
	cell *arr, size;
	arr = value.get_array(cell_indices.data(), cell_indices.size(), size);
	cell amx_addr, *addr;
	amx_Allot(amx, size ? size : 1, &amx_addr, &addr);
	std::memcpy(addr, arr, size * sizeof(cell));
	return std::tuple<cell*, size_t, tag_ptr>(addr, size, value.get_tag());
}

constexpr const tag_ptr invalid_tag = nullptr;
constexpr const cell invalid_size = -1;
constexpr const cell invalid_rank = -2;

tag_ptr expression::get_tag(const args_type &args) const noexcept
{
	return invalid_tag;
}

cell expression::get_size(const args_type &args) const noexcept
{
	return invalid_size;
}

cell expression::get_rank(const args_type &args) const noexcept
{
	return invalid_rank;
}

int &expression::operator[](size_t index) const
{
	static int unused;
	return unused;
}

void expression::checkstack() const
{
	if(stackspace() < 32768)
	{
		amx_ExpressionError("recursion too deep");
	}
}

expression *expression_base::get()
{
	return this;
}

const expression *expression_base::get() const
{
	return this;
}

dyn_object constant_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return value;
}

tag_ptr constant_expression::get_tag(const args_type &args) const noexcept
{
	return value.get_tag();
}

cell constant_expression::get_size(const args_type &args) const noexcept
{
	return value.get_size();
}

cell constant_expression::get_rank(const args_type &args) const noexcept
{
	return value.get_rank();
}

void constant_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(value.to_string());
}

decltype(expression_pool)::object_ptr constant_expression::clone() const
{
	return expression_pool.emplace_derived<constant_expression>(*this);
}

expression_ptr weak_expression::lock() const
{
	if(auto lock = ptr.lock())
	{
		return lock;
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

dyn_object weak_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return lock()->execute(amx, args, info);
}

void weak_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	lock()->execute_discard(amx, args, info);
}

void weak_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	lock()->execute_multi(amx, args, info, output);
}

dyn_object weak_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	return lock()->call(amx, args, info, call_args);
}

void weak_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	lock()->call_discard(amx, args, info, call_args);
}

void weak_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	lock()->call_multi(amx, args, info, call_args, output);
}

dyn_object weak_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	return lock()->assign(amx, args, info, std::move(value));
}

dyn_object weak_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return lock()->index(amx, args, info, indices);
}

dyn_object weak_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	return lock()->index_assign(amx, args, info, indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> weak_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return lock()->address(amx, args, info, indices);
}

tag_ptr weak_expression::get_tag(const args_type &args) const noexcept
{
	if(auto lock = ptr.lock())
	{
		return lock->get_tag(args);
	}
	return invalid_tag;
}

cell weak_expression::get_size(const args_type &args) const noexcept
{
	if(auto lock = ptr.lock())
	{
		return lock->get_size(args);
	}
	return invalid_size;
}

cell weak_expression::get_rank(const args_type &args) const noexcept
{
	if(auto lock = ptr.lock())
	{
		return lock->get_rank(args);
	}
	return invalid_rank;
}

void weak_expression::to_string(strings::cell_string &str) const noexcept
{
	if(auto lock = ptr.lock())
	{
		lock->to_string(str);
	}else{
		str.append(strings::convert("dead weak expression"));
	}
}

decltype(expression_pool)::object_ptr weak_expression::clone() const
{
	return expression_pool.emplace_derived<weak_expression>(*this);
}

const dyn_object &arg_expression::arg(const args_type &args) const
{
	if(index >= args.size())
	{
		amx_ExpressionError("expression argument #%d was not provided", index);
	}
	return args[index].get();
}

dyn_object arg_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return arg(args);
}

tag_ptr arg_expression::get_tag(const args_type &args) const noexcept
{
	if(index >= args.size())
	{
		return invalid_tag;
	}
	return args[index].get().get_tag();
}

cell arg_expression::get_size(const args_type &args) const noexcept
{
	if(index >= args.size())
	{
		return invalid_size;
	}
	return args[index].get().get_size();
}

cell arg_expression::get_rank(const args_type &args) const noexcept
{
	if(index >= args.size())
	{
		return invalid_rank;
	}
	return args[index].get().get_rank();
}

void arg_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("$arg"));
	str.append(strings::convert(std::to_string(index)));
}

decltype(expression_pool)::object_ptr arg_expression::clone() const
{
	return expression_pool.emplace_derived<arg_expression>(*this);
}

dyn_object sequence_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type output;
	execute_multi(amx, args, info, output);
	if(output.size() == 0)
	{
		amx_ExpressionError("expression has no value");
	}
	return output.back();
}

void sequence_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type output;
	execute_multi(amx, args, info, output);
}

expression_ptr empty_expression::instance = std::make_shared<empty_expression>();

void empty_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{

}

void empty_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{

}

void empty_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("()"));
}

decltype(expression_pool)::object_ptr empty_expression::clone() const
{
	return expression_pool.emplace_derived<empty_expression>(*this);
}

void arg_pack_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	if(end == -1)
	{
		if(begin > args.size())
		{
			amx_ExpressionError("expression argument #%d was not provided", begin - 1);
		}
	}else{
		if(end >= args.size())
		{
			amx_ExpressionError("expression argument #%d was not provided", args.size());
		}
	}
}

void arg_pack_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	if(end == -1)
	{
		if(begin > args.size())
		{
			amx_ExpressionError("expression argument #%d was not provided", begin - 1);
		}else for(size_t i = begin; i < args.size(); i++)
		{
			output.push_back(args[i]);
		}
	}else{
		if(end >= args.size())
		{
			amx_ExpressionError("expression argument #%d was not provided", args.size());
		}else for(size_t i = begin; i <= end; i++)
		{
			output.push_back(args[i]);
		}
	}
}

tag_ptr arg_pack_expression::get_tag(const args_type &args) const noexcept
{
	if(end == -1)
	{
		if(begin >= args.size())
		{
			return invalid_tag;
		}else{
			return args.back().get().get_tag();
		}
	}else{
		if(end >= args.size())
		{
			return invalid_tag;
		}else{
			return args[end].get().get_tag();
		}
	}
}

cell arg_pack_expression::get_size(const args_type &args) const noexcept
{
	if(end == -1)
	{
		if(begin >= args.size())
		{
			return invalid_size;
		}else{
			return args.back().get().get_size();
		}
	}else{
		if(end >= args.size())
		{
			return invalid_size;
		}else{
			return args[end].get().get_size();
		}
	}
}

cell arg_pack_expression::get_rank(const args_type &args) const noexcept
{
	if(end == -1)
	{
		if(begin >= args.size())
		{
			return invalid_rank;
		}else{
			return args.back().get().get_rank();
		}
	}else{
		if(end >= args.size())
		{
			return invalid_rank;
		}else{
			return args[end].get().get_rank();
		}
	}
}

void arg_pack_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("$args"));
	if(begin != 0 || end != -1)
	{
		str.append(strings::convert(std::to_string(begin)));
		if(end != -1)
		{
			str.push_back('_');
			str.append(strings::convert(std::to_string(end)));
		}
	}
}

decltype(expression_pool)::object_ptr arg_pack_expression::clone() const
{
	return expression_pool.emplace_derived<arg_pack_expression>(*this);
}

void range_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	auto a = begin->execute(amx, args, info);
	auto b = end->execute(amx, args, info);
	auto tag = tags::find_tag(tags::tag_cell);
	if(!a.tag_assignable(tag) || !a.is_cell())
	{
		amx_ExpressionError("range bound tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), a.get_tag()->format_name());
	}
	if(!b.tag_assignable(tag) || !b.is_cell())
	{
		amx_ExpressionError("range bound tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), b.get_tag()->format_name());
	}
	cell ca = a.get_cell(0);
	cell cb = b.get_cell(0);
	if(ca <= cb)
	{
		while(ca < cb)
		{
			output.emplace_back(ca, tag);
			ca++;
		}
		output.emplace_back(cb, tag);
	}else{
		while(ca > cb)
		{
			output.emplace_back(ca, tag);
			ca--;
		}
		output.emplace_back(cb, tag);
	}
}

tag_ptr range_expression::get_tag(const args_type &args) const noexcept
{
	return tags::find_tag(tags::tag_cell);
}

cell range_expression::get_size(const args_type &args) const noexcept
{
	return 1;
}

cell range_expression::get_rank(const args_type &args) const noexcept
{
	return 0;
}

void range_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	begin->to_string(str);
	str.append(strings::convert(".."));
	end->to_string(str);
	str.push_back(')');
}

const expression_ptr &range_expression::get_left() const noexcept
{
	return begin;
}

const expression_ptr &range_expression::get_right() const noexcept
{
	return end;
}

decltype(expression_pool)::object_ptr range_expression::clone() const
{
	return expression_pool.emplace_derived<range_expression>(*this);
}

dyn_object nested_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return expr->execute(amx, args, info);
}

tag_ptr nested_expression::get_tag(const args_type &args) const noexcept
{
	return expr->get_tag(args);
}

cell nested_expression::get_size(const args_type &args) const noexcept
{
	return expr->get_size(args);
}

cell nested_expression::get_rank(const args_type &args) const noexcept
{
	return expr->get_rank(args);
}

void nested_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('+');
	expr->to_string(str);
}

const expression_ptr &nested_expression::get_operand() const noexcept
{
	return expr;
}

decltype(expression_pool)::object_ptr nested_expression::clone() const
{
	return expression_pool.emplace_derived<nested_expression>(*this);
}

dyn_object comma_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type output;
	left->execute_multi(amx, args, info, output);
	if(output.size() == 0)
	{
		return right->execute(amx, args, info);
	}
	right->execute_multi(amx, args, info, output);
	return output.back();
}

void comma_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	left->execute_discard(amx, args, info);
	right->execute_discard(amx, args, info);
}

void comma_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	left->execute_multi(amx, args, info, output);
	right->execute_multi(amx, args, info, output);
}

dyn_object comma_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	left->execute_discard(amx, args, info);
	return right->call(amx, args, info, call_args);
}

void comma_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	left->execute_discard(amx, args, info);
	right->call_discard(amx, args, info, call_args);
}

void comma_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	left->execute_discard(amx, args, info);
	right->call_multi(amx, args, info, call_args, output);
}

dyn_object comma_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	left->execute_discard(amx, args, info);
	return right->assign(amx, args, info, std::move(value));
}

dyn_object comma_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	left->execute_discard(amx, args, info);
	return right->index(amx, args, info, indices);
}

dyn_object comma_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	left->execute_discard(amx, args, info);
	return right->index_assign(amx, args, info, indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> comma_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	left->execute_discard(amx, args, info);
	return right->address(amx, args, info, indices);
}

tag_ptr comma_expression::get_tag(const args_type &args) const noexcept
{
	auto tag = left->get_tag(args);
	if(tag == right->get_tag(args))
	{
		return tag;
	}
	return invalid_tag;
}

cell comma_expression::get_size(const args_type &args) const noexcept
{
	auto size = left->get_size(args);
	if(size == right->get_size(args))
	{
		return size;
	}
	return invalid_size;
}

cell comma_expression::get_rank(const args_type &args) const noexcept
{
	auto rank = left->get_rank(args);
	if(rank == right->get_rank(args))
	{
		return rank;
	}
	return invalid_rank;
}

void comma_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	left->to_string(str);
	str.push_back(',');
	str.push_back(' ');
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &comma_expression::get_left() const noexcept
{
	return left;
}

const expression_ptr &comma_expression::get_right() const noexcept
{
	return right;
}

decltype(expression_pool)::object_ptr comma_expression::clone() const
{
	return expression_pool.emplace_derived<comma_expression>(*this);
}

dyn_object env_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	amx_ExpressionError("attempt to obtain the value of the environment");
	return {};
}

dyn_object env_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	if(indices.size() != 1)
	{
		amx_ExpressionError("exactly one index must be specified to access the environment (%d given)", indices.size());
	}
	auto it = info.env.find(indices[0]);
	if(it == info.env.end())
	{
		amx_ExpressionError("the element is not present in the environment");
	}
	return it->second;
}

dyn_object env_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	if(indices.size() != 1)
	{
		amx_ExpressionError("exactly one index must be specified to access the environment (%d given)", indices.size());
	}
	if(info.env_readonly)
	{
		amx_ExpressionError("the environment is read-only");
	}
	return info.env[indices[0]] = std::move(value);
}

void env_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("$env"));
}

decltype(expression_pool)::object_ptr env_expression::clone() const
{
	return expression_pool.emplace_derived<env_expression>(*this);
}

expression::exec_info env_set_expression::get_info(const exec_info &info) const
{
	if(new_env)
	{
		return exec_info(info, *new_env, readonly);
	}
	if(readonly)
	{
		return exec_info(info, readonly);
	}
	return info;
}

dyn_object env_set_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return expr->execute(amx, args, get_info(info));
}

dyn_object env_set_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	return expr->call(amx, args, get_info(info), call_args);
}

void env_set_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	expr->call_discard(amx, args, get_info(info), call_args);
}

void env_set_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	expr->call_multi(amx, args, get_info(info), call_args, output);
}

dyn_object env_set_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	return expr->assign(amx, args, get_info(info), std::move(value));
}

dyn_object env_set_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return expr->index(amx, args, get_info(info), indices);
}

dyn_object env_set_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	return expr->index_assign(amx, args, get_info(info), indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> env_set_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return expr->address(amx, args, get_info(info), indices);
}

tag_ptr env_set_expression::get_tag(const args_type &args) const noexcept
{
	return expr->get_tag(args);
}

cell env_set_expression::get_size(const args_type &args) const noexcept
{
	return expr->get_size(args);
}

cell env_set_expression::get_rank(const args_type &args) const noexcept
{
	return expr->get_rank(args);
}

void env_set_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('[');
	expr->to_string(str);
	str.append(strings::convert("]{env}"));
}

const expression_ptr &env_set_expression::get_operand() const noexcept
{
	return expr;
}

decltype(expression_pool)::object_ptr env_set_expression::clone() const
{
	return expression_pool.emplace_derived<env_set_expression>(*this);
}

dyn_object assign_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return left->assign(amx, args, info, right->execute(amx, args, info));
}

tag_ptr assign_expression::get_tag(const args_type &args) const noexcept
{
	return left->get_tag(args);
}

cell assign_expression::get_size(const args_type &args) const noexcept
{
	return left->get_size(args);
}

cell assign_expression::get_rank(const args_type &args) const noexcept
{
	return left->get_rank(args);
}

void assign_expression::to_string(strings::cell_string &str) const noexcept
{
	left->to_string(str);
	str.append(strings::convert(" = "));
	right->to_string(str);
}

const expression_ptr &assign_expression::get_left() const noexcept
{
	return left;
}

const expression_ptr &assign_expression::get_right() const noexcept
{
	return right;
}

decltype(expression_pool)::object_ptr assign_expression::clone() const
{
	return expression_pool.emplace_derived<assign_expression>(*this);
}

dyn_object try_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	try{
		return main->execute(amx, args, info);
	}catch(const errors::native_error&)
	{
		return fallback->execute(amx, args, info);
	}
}

void try_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	try{
		main->execute_discard(amx, args, info);
	}catch(const errors::native_error&)
	{
		fallback->execute_discard(amx, args, info);
	}
}

void try_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	size_t size = output.size();
	try{
		main->execute_multi(amx, args, info, output);
	}catch(const errors::native_error&)
	{
		output.resize(size);
		fallback->execute_multi(amx, args, info, output);
	}
}

dyn_object try_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	try{
		return main->call(amx, args, info, call_args);
	}catch(const errors::native_error&)
	{
		return fallback->call(amx, args, info, call_args);
	}
}

void try_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	try{
		main->call_discard(amx, args, info, call_args);
	}catch(const errors::native_error&)
	{
		fallback->call_discard(amx, args, info, call_args);
	}
}

void try_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	size_t size = output.size();
	try{
		main->call_multi(amx, args, info, call_args, output);
	}catch(const errors::native_error&)
	{
		output.resize(size);
		fallback->call_multi(amx, args, info, call_args, output);
	}
}

dyn_object try_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	try{
		return main->assign(amx, args, info, std::move(value));
	}catch(const errors::native_error&)
	{
		return fallback->assign(amx, args, info, std::move(value));
	}
}

dyn_object try_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	try{
		return main->index(amx, args, info, indices);
	}catch(const errors::native_error&)
	{
		return fallback->index(amx, args, info, indices);
	}
}

dyn_object try_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	try{
		return main->index_assign(amx, args, info, indices, std::move(value));
	}catch(const errors::native_error&)
	{
		return fallback->index_assign(amx, args, info, indices, std::move(value));
	}
}

std::tuple<cell*, size_t, tag_ptr> try_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	try{
		return main->address(amx, args, info, indices);
	}catch(const errors::native_error&)
	{
		return fallback->address(amx, args, info, indices);
	}
}

tag_ptr try_expression::get_tag(const args_type &args) const noexcept
{
	auto tag = main->get_tag(args);
	if(tag == fallback->get_tag(args))
	{
		return tag;
	}
	return invalid_tag;
}

cell try_expression::get_size(const args_type &args) const noexcept
{
	auto size = main->get_size(args);
	if(size == fallback->get_size(args))
	{
		return size;
	}
	return invalid_size;
}

cell try_expression::get_rank(const args_type &args) const noexcept
{
	auto rank = main->get_rank(args);
	if(rank == fallback->get_rank(args))
	{
		return rank;
	}
	return invalid_rank;
}

void try_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("try["));
	main->to_string(str);
	str.append(strings::convert("]catch["));
	fallback->to_string(str);
	str.push_back(']');
}

const expression_ptr &try_expression::get_left() const noexcept
{
	return main;
}

const expression_ptr &try_expression::get_right() const noexcept
{
	return fallback;
}

decltype(expression_pool)::object_ptr try_expression::clone() const
{
	return expression_pool.emplace_derived<try_expression>(*this);
}

dyn_object call_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	checkstack();
	call_args_type func_args;
	for(const auto &arg : this->args)
	{
		arg->execute_multi(amx, args, info, func_args);
	}
	return func->call(amx, args, info, func_args);
}

void call_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	checkstack();
	call_args_type func_args;
	for(const auto &arg : this->args)
	{
		arg->execute_multi(amx, args, info, func_args);
	}
	func->call_discard(amx, args, info, func_args);
}

void call_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	checkstack();
	call_args_type func_args;
	for(const auto &arg : this->args)
	{
		arg->execute_multi(amx, args, info, func_args);
	}
	func->call_multi(amx, args, info, func_args, output);
}

void call_expression::to_string(strings::cell_string &str) const noexcept
{
	func->to_string(str);
	str.push_back('(');
	bool first = true;
	for(const auto &arg : args)
	{
		if(first)
		{
			first = false;
		}else{
			str.push_back(',');
			str.push_back(' ');
		}
		arg->to_string(str);
	}
	str.push_back(')');
}

tag_ptr call_expression::get_tag(const args_type &args) const noexcept
{
	return func->get_tag(args);
}

cell call_expression::get_size(const args_type &args) const noexcept
{
	return func->get_size(args);
}

cell call_expression::get_rank(const args_type &args) const noexcept
{
	return func->get_rank(args);
}

const expression_ptr &call_expression::get_operand() const noexcept
{
	return func;
}

decltype(expression_pool)::object_ptr call_expression::clone() const
{
	return expression_pool.emplace_derived<call_expression>(*this);
}

dyn_object index_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type indices;
	for(const auto &index : this->indices)
	{
		index->execute_multi(amx, args, info, indices);
	}
	return arr->index(amx, args, info, indices);
}

dyn_object index_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	call_args_type indices;
	for(const auto &index : this->indices)
	{
		index->execute_multi(amx, args, info, indices);
	}
	return arr->index_assign(amx, args, info, indices, std::move(value));
}

dyn_object index_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	call_args_type new_indices;
	new_indices.reserve(this->indices.size() + indices.size());
	for(const auto &index : this->indices)
	{
		index->execute_multi(amx, args, info, new_indices);
	}
	for(const auto &index : indices)
	{
		new_indices.push_back(index);
	}
	return arr->index(amx, args, info, new_indices);
}

std::tuple<cell*, size_t, tag_ptr> index_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	call_args_type new_indices;
	new_indices.reserve(this->indices.size() + indices.size());
	for(const auto &index : this->indices)
	{
		index->execute_multi(amx, args, info, new_indices);
	}
	for(const auto &index : indices)
	{
		new_indices.push_back(index);
	}
	return arr->address(amx, args, info, new_indices);
}

void index_expression::to_string(strings::cell_string &str) const noexcept
{
	arr->to_string(str);
	for(const auto &index : indices)
	{
		str.push_back('[');
		index->to_string(str);
		str.push_back(']');
	}
}

tag_ptr index_expression::get_tag(const args_type &args) const noexcept
{
	return arr->get_tag(args);
}

cell index_expression::get_size(const args_type &args) const noexcept
{
	if(arr->get_size(args) != invalid_size)
	{
		return 1;
	}
	return invalid_size;
}

cell index_expression::get_rank(const args_type &args) const noexcept
{
	if(arr->get_rank(args) != invalid_rank)
	{
		return 1;
	}
	return invalid_rank;
}

const expression_ptr &index_expression::get_operand() const noexcept
{
	return arr;
}

decltype(expression_pool)::object_ptr index_expression::clone() const
{
	return expression_pool.emplace_derived<index_expression>(*this);
}

auto bind_expression::combine_args(const args_type &args) const -> args_type
{
	args_type new_args;
	new_args.reserve(base_args.size() + args.size());
	for(const auto &arg : base_args)
	{
		if(auto const_expr = dynamic_cast<const constant_expression*>(arg.get()))
		{
			new_args.push_back(std::cref(const_expr->get_value()));
		}else{
			return new_args;
		}
	}
	for(const auto &arg_ref : args)
	{
		new_args.push_back(arg_ref);
	}
	return new_args;
}

auto bind_expression::combine_args(AMX *amx, const args_type &args, const exec_info &info, call_args_type &storage) const -> args_type
{
	storage.reserve(base_args.size() + args.size());
	args_type new_args;
	new_args.reserve(base_args.size() + args.size());
	for(const auto &arg : base_args)
	{
		if(auto const_expr = dynamic_cast<const constant_expression*>(arg.get()))
		{
			new_args.push_back(std::cref(const_expr->get_value()));
		}else{
			size_t index = storage.size();
			arg->execute_multi(amx, args, info, storage);
			while(index < storage.size())
			{
				new_args.push_back(std::cref(storage[index++]));
			}
		}
	}
	for(const auto &arg_ref : args)
	{
		new_args.push_back(arg_ref);
	}
	return new_args;
}

void bind_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type storage;
	operand->execute_discard(amx, combine_args(amx, args, info, storage), info);
}

void bind_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	call_args_type storage;
	operand->execute_multi(amx, combine_args(amx, args, info, storage), info, output);
}

dyn_object bind_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	call_args_type storage;
	return operand->execute(amx, combine_args(amx, args, info, storage), info);
}

dyn_object bind_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	call_args_type storage;
	return operand->call(amx, combine_args(amx, args, info, storage), info, call_args);
}

void bind_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	call_args_type storage;
	operand->call_discard(amx, combine_args(amx, args, info, storage), info, call_args);
}

void bind_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	call_args_type storage;
	operand->call_multi(amx, combine_args(amx, args, info, storage), info, call_args, output);
}

dyn_object bind_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	call_args_type storage;
	return operand->assign(amx, combine_args(amx, args, info, storage), info, std::move(value));
}

dyn_object bind_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	call_args_type storage;
	return operand->index(amx, combine_args(amx, args, info, storage), info, indices);
}

dyn_object bind_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	call_args_type storage;
	return operand->index_assign(amx, combine_args(amx, args, info, storage), info, indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> bind_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	call_args_type storage;
	return operand->address(amx, combine_args(amx, args, info, storage), info, indices);
}

void bind_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('[');
	operand->to_string(str);
	str.push_back(']');
	str.push_back('(');
	bool first = true;
	for(const auto &arg : base_args)
	{
		if(first)
		{
			first = false;
		}else{
			str.push_back(',');
			str.push_back(' ');
		}
		arg->to_string(str);
	}
	str.push_back(')');
}

tag_ptr bind_expression::get_tag(const args_type &args) const noexcept
{
	return operand->get_tag(combine_args(args));
}

cell bind_expression::get_size(const args_type &args) const noexcept
{
	return operand->get_size(combine_args(args));
}

cell bind_expression::get_rank(const args_type &args) const noexcept
{
	return operand->get_rank(combine_args(args));
}

const expression_ptr &bind_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr bind_expression::clone() const
{
	return expression_pool.emplace_derived<bind_expression>(*this);
}

dyn_object cast_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return dyn_object(operand->execute(amx, args, info), new_tag);
}

void cast_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	operand->execute_discard(amx, args, info);
}

void cast_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	size_t size = output.size();
	operand->execute_multi(amx, args, info, output);
	while(size < output.size())
	{
		dyn_object val(output[size], new_tag);
		output[size] = std::move(val);
		size++;
	}
}

dyn_object cast_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	return dyn_object(operand->assign(amx, args, info, dyn_object(std::move(value), operand->get_tag(args))), new_tag);
}

dyn_object cast_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	return dyn_object(operand->index_assign(amx, args, info, indices, dyn_object(std::move(value), operand->get_tag(args))), new_tag);
}

void cast_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert(new_tag->format_name()));
	str.push_back(':');
	operand->to_string(str);
}

tag_ptr cast_expression::get_tag(const args_type &args) const noexcept
{
	return new_tag;
}

cell cast_expression::get_size(const args_type &args) const noexcept
{
	return operand->get_size(args);
}

cell cast_expression::get_rank(const args_type &args) const noexcept
{
	return operand->get_rank(args);
}

const expression_ptr &cast_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr cast_expression::clone() const
{
	return expression_pool.emplace_derived<cast_expression>(*this);
}

dyn_object array_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	std::vector<cell> data;
	tag_ptr tag = nullptr;
	for(const auto &arg : this->args)
	{
		call_args_type values;
		arg->execute_multi(amx, args, info, values);
		for(const auto &value : values)
		{
			if(tag == nullptr)
			{
				tag = value.get_tag();
			}else if(tag != value.get_tag())
			{
				amx_ExpressionError("array constructor argument tag mismatch (%s: required, %s: provided)", tag->format_name(), value.get_tag()->format_name());
			}
			if(value.get_rank() != 0)
			{
				amx_ExpressionError("only single cell arguments are supported in array construction (value of rank %d provided)", value.get_rank());
			}
			data.push_back(value.get_cell(0));
		}
	}
	return dyn_object(data.data(), data.size(), tag ? tag : tags::find_tag(tags::tag_cell));
}

tag_ptr array_expression::get_tag(const args_type &args) const noexcept
{
	tag_ptr tag = nullptr;
	for(const auto &arg : this->args)
	{
		auto arg_tag = arg->get_tag(args);
		if(arg_tag == invalid_tag)
		{
			return invalid_tag;
		}
		if(!tag)
		{
			tag = arg_tag;
		}else if(tag != arg_tag)
		{
			return invalid_tag;
		}
	}
	return tag ? tag : tags::find_tag(tags::tag_cell);
}

cell array_expression::get_size(const args_type &args) const noexcept
{
	return invalid_size;
}

cell array_expression::get_rank(const args_type &args) const noexcept
{
	return 1;
}

void array_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('{');
	bool first = true;
	for(const auto &arg : args)
	{
		if(first)
		{
			first = false;
		}else{
			str.push_back(',');
			str.push_back(' ');
		}
		arg->to_string(str);
	}
	str.push_back('}');
}

decltype(expression_pool)::object_ptr array_expression::clone() const
{
	return expression_pool.emplace_derived<array_expression>(*this);
}

dyn_object global_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto it = info.env.find(key);
	if(it == info.env.end())
	{
		amx_ExpressionError("symbol is not defined");
	}
	return it->second;
}

dyn_object global_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	if(info.env_readonly)
	{
		amx_ExpressionError("the environment is read-only");
	}
	return info.env[key] = std::move(value);
}

dyn_object global_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	auto it = info.env.find(key);
	if(it == info.env.end())
	{
		amx_ExpressionError("symbol is not defined");
	}
	if(info.env_readonly)
	{
		amx_ExpressionError("the environment is read-only");
	}
	auto &obj = it->second;
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("indexed assignment operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	if(!(value.tag_assignable(obj.get_tag())))
	{
		amx_ExpressionError("assigned value tag mismatch (%s: required, %s: provided)", obj.get_tag()->format_name(), value.get_tag()->format_name());
	}
	std::vector<cell> cell_indices;
	for(const auto &index : indices)
	{
		if(!(index.tag_assignable(tags::find_tag(tags::tag_cell))))
		{
			amx_ExpressionError("index tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), index.get_tag()->format_name());
		}
		if(index.get_rank() != 0)
		{
			amx_ExpressionError("index operation requires a single cell value (value of rank %d provided)", index.get_rank());
		}
		cell c = index.get_cell(0);
		if(c < 0)
		{
			amx_ExpressionError("index out of bounds");
		}
		cell_indices.push_back(c);
	}
	obj.set_cell(cell_indices.data(), cell_indices.size(), value.get_cell(0));
	return value;
}

void global_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(name);
}

decltype(expression_pool)::object_ptr global_expression::clone() const
{
	return expression_pool.emplace_derived<global_expression>(*this);
}

dyn_object symbol_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			amx_ExpressionError("attempt to obtain the value of function '%s'", symbol->name);
		}

		amx = *obj;
		auto data = amx_GetData(amx);
		cell *ptr;
		if(symbol->vclass == 0 || symbol->vclass == 2)
		{
			ptr = reinterpret_cast<cell*>(data + symbol->address);
		}else{
			cell frm = amx->frm;
			ucell cip = amx->cip;
			while(frm != 0)
			{
				if(frm == target_frm && symbol->codestart <= cip && cip < symbol->codeend)
				{
					break;
				}else if(frm > target_frm)
				{
					frm = 0;
					break;
				}
				cip = reinterpret_cast<cell*>(data + frm)[1];
				frm = reinterpret_cast<cell*>(data + frm)[0];
				if(cip == 0)
				{
					frm = 0;
				}
			}
			if(frm == 0)
			{
				amx_ExpressionError("referenced variable was unloaded");
			}
			ptr = reinterpret_cast<cell*>(data + frm + symbol->address);
		}

		if(symbol->ident == iREFERENCE || symbol->ident == iREFARRAY)
		{
			ptr = reinterpret_cast<cell*>(data + *ptr);
		}
		tag_ptr tag_ptr = get_tag(args);

		if(symbol->dim == 0)
		{
			return dyn_object(*ptr, tag_ptr);
		}else{
			const AMX_DBG_SYMDIM *dim;
			if(dbg_GetArrayDim(debug, symbol, &dim) == AMX_ERR_NONE)
			{
				switch(symbol->dim)
				{
					case 1:
						return dyn_object(ptr, dim[0].size, tag_ptr);
					case 2:
						return dyn_object(amx, ptr, dim[0].size, dim[1].size, tag_ptr);
					case 3:
						return dyn_object(amx, ptr, dim[0].size, dim[1].size, dim[2].size, tag_ptr);
					default:
						amx_ExpressionError("array rank %d not supported", symbol->dim);
				}
			}
		}
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

dyn_object symbol_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident != iFUNCTN)
		{
			return expression::call(amx, args, info, call_args);
		}

		amx = *obj;
		auto data = amx_GetData(amx);
		auto stk = reinterpret_cast<cell*>(data + amx->stk);

		cell argslen = call_args.size() * sizeof(cell);
		cell argsneeded = 0;
		for(uint16_t i = 0; i < debug->hdr->symbols; i++)
		{
			auto vsym = debug->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= symbol->codestart && vsym->codeend <= symbol->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}else if(addr >= 0 && !argsneeded)
				{
					const auto &arg = call_args[addr / sizeof(cell) - 1];
					cell tag = vsym->tag;
					tag_ptr test_tag = nullptr;

					if(tag == 0)
					{
						test_tag = tags::find_tag(tags::tag_cell);
					}else{
						const char *tagname;
						if(dbg_GetTagName(debug, tag, &tagname) == AMX_ERR_NONE)
						{
							test_tag = tags::find_existing_tag(tagname);
						}
					}
					bool isaddress = arg.tag_assignable(tags::find_tag(tags::tag_address));
					if(isaddress && vsym->ident != iREFERENCE && vsym->ident != iARRAY && vsym->ident != iREFARRAY)
					{
						amx_ExpressionError("address argument not expected");
					}
					if(isaddress)
					{
						if(test_tag)
						{
							test_tag = tags::find_tag((tags::find_tag(tags::tag_address)->name + "@" + test_tag->name).c_str());
						}
					}else{
						if(vsym->dim != arg.get_rank())
						{
							amx_ExpressionError("incorrect rank of argument (%d needed, %d given)", vsym->dim, arg.get_rank());
						}
					}
					if(test_tag && !arg.tag_assignable(test_tag))
					{
						amx_ExpressionError("argument tag mismatch (%s: required, %s: provided)", test_tag->format_name(), arg.get_tag()->format_name());
					}
					if(!isaddress && (vsym->ident == iREFERENCE || vsym->ident == iARRAY || vsym->ident == iREFARRAY) && arg.get_rank() <= 0)
					{
						amx_ExpressionError("a reference argument must be provided with an array value or an address");
					}
				}
			}
		}
		if(argsneeded)
		{
			amx_ExpressionError("too few arguments provided to a function (%d needed, %d given)", argsneeded / sizeof(cell), argslen / sizeof(cell));
		}

		cell reset_hea, *tmp;
		amx_Allot(amx, 0, &reset_hea, &tmp);

		cell num = call_args.size() * sizeof(cell);
		amx->stk -= num + 2 * sizeof(cell);
		for(cell i = call_args.size() - 1; i >= 0; i--)
		{
			cell val = call_args[i].store(amx);
			*--stk = val;
		}
		*--stk = num;
		*--stk = 0;
		amx->cip = symbol->address;
		amx->reset_hea = amx->hea;
		amx->reset_stk = amx->stk;
		cell ret;
		int old_error = amx->error;
		int err = amx_Exec(amx, &ret, AMX_EXEC_CONT);
		amx->error = old_error;
		amx_Release(amx, reset_hea);
		if(err == AMX_ERR_NONE)
		{
			return dyn_object(ret, get_tag(args));
		}
		amx_ExpressionError(errors::inner_error, "script", symbol->name, err, amx::StrError(err));
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

void symbol_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident != iFUNCTN)
		{
			expression::call_discard(amx, args, info, call_args);
			return;
		}
	}
	call(amx, args, info, call_args);
}

void symbol_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident != iFUNCTN)
		{
			expression::call_multi(amx, args, info, call_args, output);
			return;
		}
	}
	output.push_back(call(amx, args, info, call_args));
}

dyn_object symbol_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			return expression::assign(amx, args, info, std::move(value));
		}

		amx = *obj;
		auto data = amx_GetData(amx);
		cell *ptr;
		if(symbol->vclass == 0 || symbol->vclass == 2)
		{
			ptr = reinterpret_cast<cell*>(data + symbol->address);
		}else{
			cell frm = amx->frm;
			ucell cip = amx->cip;
			while(frm != 0)
			{
				if(frm == target_frm && symbol->codestart <= cip && cip < symbol->codeend)
				{
					break;
				}else if(frm > target_frm)
				{
					frm = 0;
					break;
				}
				cip = reinterpret_cast<cell*>(data + frm)[1];
				frm = reinterpret_cast<cell*>(data + frm)[0];
				if(cip == 0)
				{
					frm = 0;
				}
			}
			if(frm == 0)
			{
				amx_ExpressionError("referenced variable was unloaded");
			}
			ptr = reinterpret_cast<cell*>(data + frm + symbol->address);
		}

		if(symbol->ident == iREFERENCE || symbol->ident == iREFARRAY)
		{
			ptr = reinterpret_cast<cell*>(data + *ptr);
		}
		tag_ptr tag_ptr = get_tag(args);

		if(symbol->dim == value.get_rank())
		{
			if(symbol->dim == 0)
			{
				*ptr = value.get_cell(0);
				return std::move(value);
			}else if(symbol->dim == 1)
			{
				const AMX_DBG_SYMDIM *dim;
				if(dbg_GetArrayDim(debug, symbol, &dim) == AMX_ERR_NONE && dim[0].size == value.get_size())
				{
					value.get_array(ptr, dim[0].size);
					return std::move(value);
				}
			}
			amx_ExpressionError("array rank %d not supported", symbol->dim);
		}
		amx_ExpressionError("incorrect rank of value (%d needed, %d given)", symbol->dim, value.get_rank());
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

dyn_object symbol_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->dim == 0)
		{
			return expression::index(amx, args, info, indices);
		}
		auto addr = address(amx, args, info, indices);
		if(std::get<1>(addr) == 0)
		{
			amx_ExpressionError("index out of bounds");
		}
		return dyn_object(std::get<0>(addr)[0], std::get<2>(addr));
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

std::tuple<cell*, size_t, tag_ptr> symbol_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			return expression::address(amx, args, info, indices);
		}

		amx = *obj;
		auto data = amx_GetData(amx);
		cell *ptr;
		if(symbol->vclass == 0 || symbol->vclass == 2)
		{
			ptr = reinterpret_cast<cell*>(data + symbol->address);
		}else{
			cell frm = amx->frm;
			ucell cip = amx->cip;
			while(frm != 0)
			{
				if(frm == target_frm && symbol->codestart <= cip && cip < symbol->codeend)
				{
					break;
				}else if(frm > target_frm)
				{
					frm = 0;
					break;
				}
				cip = reinterpret_cast<cell*>(data + frm)[1];
				frm = reinterpret_cast<cell*>(data + frm)[0];
				if(cip == 0)
				{
					frm = 0;
				}
			}
			if(frm == 0)
			{
				amx_ExpressionError("referenced variable was unloaded");
			}
			ptr = reinterpret_cast<cell*>(data + frm + symbol->address);
		}

		if(symbol->ident == iREFERENCE || symbol->ident == iREFARRAY)
		{
			ptr = reinterpret_cast<cell*>(data + *ptr);
		}
		tag_ptr tag = get_tag(args);

		if(symbol->dim == indices.size())
		{
			std::vector<ucell> cell_indices;
			for(const auto &index : indices)
			{
				if(!(index.tag_assignable(tags::find_tag(tags::tag_cell))))
				{
					amx_ExpressionError("index tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), index.get_tag()->format_name());
				}
				if(index.get_rank() != 0)
				{
					amx_ExpressionError("index operation requires a single cell value (value of rank %d provided)", index.get_rank());
				}
				cell c = index.get_cell(0);
				if(c < 0)
				{
					amx_ExpressionError("index out of bounds");
				}
				cell_indices.push_back(c);
			}
			if(symbol->dim == 0)
			{
				return std::tuple<cell*, size_t, tag_ptr>(ptr, 1, tag);
			}else if(symbol->dim == 1)
			{
				const AMX_DBG_SYMDIM *dim;
				if(dbg_GetArrayDim(debug, symbol, &dim) == AMX_ERR_NONE)
				{
					if(cell_indices[0] > dim[0].size)
					{
						amx_ExpressionError("index out of bounds");
					}
					return std::tuple<cell*, size_t, tag_ptr>(ptr + cell_indices[0], dim[0].size - cell_indices[0], tag);
				}
			}
			amx_ExpressionError("array rank %d not supported", symbol->dim);
		}
		amx_ExpressionError("incorrect number of indices (%d needed, %d given)", symbol->dim, indices.size());
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

tag_ptr symbol_expression::get_tag(const args_type &args) const noexcept
{
	if(!target_amx.expired())
	{
		cell tag = symbol->tag;

		if(tag == 0)
		{
			return tags::find_tag(tags::tag_cell);
		}
		const char *tagname;
		if(dbg_GetTagName(debug, tag, &tagname) == AMX_ERR_NONE)
		{
			return tags::find_existing_tag(tagname);
		}
		return tags::find_tag(tags::tag_unknown);
	}
	return invalid_tag;
}

cell symbol_expression::get_size(const args_type &args) const noexcept
{
	if(!target_amx.expired())
	{
		const AMX_DBG_SYMDIM *dim;
		if(dbg_GetArrayDim(debug, symbol, &dim) == AMX_ERR_NONE)
		{
			return dim[0].size;
		}
		return 0;
	}
	return invalid_size;
}

cell symbol_expression::get_rank(const args_type &args) const noexcept
{
	if(!target_amx.expired())
	{
		return symbol->dim;
	}
	return invalid_rank;
}

void symbol_expression::to_string(strings::cell_string &str) const noexcept
{
	if(!target_amx.expired())
	{
		str.append(strings::convert(symbol->name));
		return;
	}
	str.append(strings::convert("unknown symbol"));
}

decltype(expression_pool)::object_ptr symbol_expression::clone() const
{
	return expression_pool.emplace_derived<symbol_expression>(*this);
}

dyn_object native_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	amx_ExpressionError("attempt to obtain the value of function '%s'", name.c_str());
	return {};
}

dyn_object native_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	auto data = amx_GetData(amx);
	auto stk = reinterpret_cast<cell*>(data + amx->stk);

	cell reset_hea, *tmp;
	amx_Allot(amx, 0, &reset_hea, &tmp);

	cell num = call_args.size() * sizeof(cell);
	amx->stk -= num + 1 * sizeof(cell);
	for(cell i = call_args.size() - 1; i >= 0; i--)
	{
		cell val = call_args[i].store(amx);
		*--stk = val;
	}
	*--stk = num;
	int old_error = amx->error;
	cell ret = native(amx, stk);
	amx->stk += num + 1 * sizeof(cell);
	int err = amx->error;
	amx->error = old_error;
	amx_Release(amx, reset_hea);
	if(err == AMX_ERR_NONE)
	{
		return dyn_object(ret, tags::find_tag(tags::tag_cell));
	}
	amx_ExpressionError(errors::inner_error, "native", name.c_str(), err, amx::StrError(err));
	return {};
}

void native_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	call(amx, args, info, call_args);
}

void native_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	output.push_back(call(amx, args, info, call_args));
}

tag_ptr native_expression::get_tag(const args_type &args) const noexcept
{
	return tags::find_tag(tags::tag_cell);
}

cell native_expression::get_size(const args_type &args) const noexcept
{
	return 1;
}

cell native_expression::get_rank(const args_type &args) const noexcept
{
	return 0;
}

void native_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert(name));
}

decltype(expression_pool)::object_ptr native_expression::clone() const
{
	return expression_pool.emplace_derived<native_expression>(*this);
}

dyn_object local_native_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		return native_expression::call(*obj, args, info, call_args);
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

void local_native_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		native_expression::call_discard(*obj, args, info, call_args);
		return;
	}
	amx_ExpressionError("target AMX was unloaded");
}

void local_native_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	if(auto obj = target_amx.lock())
	{
		native_expression::call_multi(*obj, args, info, call_args, output);
		return;
	}
	amx_ExpressionError("target AMX was unloaded");
}

decltype(expression_pool)::object_ptr local_native_expression::clone() const
{
	return expression_pool.emplace_derived<local_native_expression>(*this);
}

AMX *public_expression::load() const
{
	if(auto amx = target_amx.lock())
	{
		if(index != -1)
		{
			int len;
			amx_NameLength(*amx, &len);
			char *funcname = static_cast<char*>(alloca(len + 1));

			if(amx_GetPublic(*amx, index, funcname) == AMX_ERR_NONE && !std::strcmp(name.c_str(), funcname))
			{
				return *amx;
			}else if(amx_FindPublicSafe(*amx, name.c_str(), &index) == AMX_ERR_NONE)
			{
				return *amx;
			}
		}else if(amx_FindPublicSafe(*amx, name.c_str(), &index) == AMX_ERR_NONE)
		{
			return *amx;
		}
		index = -1;
		amx_ExpressionError(errors::func_not_found, "public", name.c_str());
	}
	amx_ExpressionError("target AMX was unloaded");
	return nullptr;
}

dyn_object public_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	amx_ExpressionError("attempt to obtain the value of function '%s'", name.c_str());
	return {};
}

dyn_object public_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	load();

	auto data = amx_GetData(amx);
	auto stk = reinterpret_cast<cell*>(data + amx->stk);

	cell reset_hea, *tmp;
	amx_Allot(amx, 0, &reset_hea, &tmp);

	for(cell i = call_args.size() - 1; i >= 0; i--)
	{
		cell val = call_args[i].store(amx);
		amx_Push(amx, val);
	}
	cell ret;
	int err = amx_Exec(amx, &ret, index);
	amx_Release(amx, reset_hea);
	if(err == AMX_ERR_NONE)
	{
		return dyn_object(ret, tags::find_tag(tags::tag_cell));
	}
	amx_ExpressionError(errors::inner_error, "public", name.c_str(), err, amx::StrError(err));
	return {};
}

void public_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	call(amx, args, info, call_args);
}

void public_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	output.push_back(call(amx, args, info, call_args));
}

tag_ptr public_expression::get_tag(const args_type &args) const noexcept
{
	return tags::find_tag(tags::tag_cell);
}

cell public_expression::get_size(const args_type &args) const noexcept
{
	return 1;
}

cell public_expression::get_rank(const args_type &args) const noexcept
{
	return 0;
}

void public_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert(name));
}

decltype(expression_pool)::object_ptr public_expression::clone() const
{
	return expression_pool.emplace_derived<public_expression>(*this);
}

cell quote_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	return expression_pool.get_id(static_cast<const expression_base*>(operand.get())->clone());
}

void quote_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('<');
	operand->to_string(str);
	str.push_back('>');
}

const expression_ptr &quote_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr quote_expression::clone() const
{
	return expression_pool.emplace_derived<quote_expression>(*this);
}

expression *dequote_expression::get_expr(AMX *amx, const args_type &args, const exec_info &info) const
{
	checkstack();
	auto value = operand->execute(amx, args, info);
	if(!(value.tag_assignable(tags::find_tag(tags::tag_expression))))
	{
		amx_ExpressionError("dequote argument tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_expression)->format_name(), value.get_tag()->format_name());
	}
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("dequote operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	cell c = value.get_cell(0);
	expression *expr;
	if(!expression_pool.get_by_id(c, expr))
	{
		amx_ExpressionError(errors::pointer_invalid, "expression", c);
	}
	return expr;
}

dyn_object dequote_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return get_expr(amx, args, info)->execute(amx, args, info);
}

void dequote_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	return get_expr(amx, args, info)->execute_discard(amx, args, info);
}

void dequote_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	return get_expr(amx, args, info)->execute_multi(amx, args, info, output);
}

dyn_object dequote_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	return get_expr(amx, args, info)->call(amx, args, info, call_args);
}

void dequote_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	get_expr(amx, args, info)->call_discard(amx, args, info, call_args);
}

void dequote_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	get_expr(amx, args, info)->call_multi(amx, args, info, call_args, output);
}

dyn_object dequote_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	return get_expr(amx, args, info)->assign(amx, args, info, std::move(value));
}

dyn_object dequote_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return get_expr(amx, args, info)->index(amx, args, info, indices);
}

dyn_object dequote_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	return get_expr(amx, args, info)->index_assign(amx, args, info, indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> dequote_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return get_expr(amx, args, info)->address(amx, args, info, indices);
}

void dequote_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('^');
	operand->to_string(str);
}

const expression_ptr &dequote_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr dequote_expression::clone() const
{
	return expression_pool.emplace_derived<dequote_expression>(*this);
}

bool logic_and_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	return left->execute_bool(amx, args, info) && right->execute_bool(amx, args, info);
}

void logic_and_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	left->to_string(str);
	str.append(strings::convert(" && "));
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &logic_and_expression::get_left() const noexcept
{
	return left;
}

const expression_ptr &logic_and_expression::get_right() const noexcept
{
	return right;
}

decltype(expression_pool)::object_ptr logic_and_expression::clone() const
{
	return expression_pool.emplace_derived<logic_and_expression>(*this);
}

bool logic_or_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	return left->execute_bool(amx, args, info) || right->execute_bool(amx, args, info);
}

void logic_or_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	left->to_string(str);
	str.append(strings::convert(" || "));
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &logic_or_expression::get_left() const noexcept
{
	return left;
}

const expression_ptr &logic_or_expression::get_right() const noexcept
{
	return right;
}

decltype(expression_pool)::object_ptr logic_or_expression::clone() const
{
	return expression_pool.emplace_derived<logic_or_expression>(*this);
}

dyn_object conditional_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->execute(amx, args, info);
}

void conditional_expression::execute_discard(AMX *amx, const args_type &args, const exec_info &info) const
{
	(cond->execute_bool(amx, args, info) ? on_true : on_false)->execute_discard(amx, args, info);
}

void conditional_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	(cond->execute_bool(amx, args, info) ? on_true : on_false)->execute_multi(amx, args, info, output);
}

dyn_object conditional_expression::call(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->call(amx, args, info, call_args);
}

void conditional_expression::call_discard(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args) const
{
	(cond->execute_bool(amx, args, info) ? on_true : on_false)->call_discard(amx, args, info, call_args);
}

void conditional_expression::call_multi(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &call_args, call_args_type &output) const
{
	(cond->execute_bool(amx, args, info) ? on_true : on_false)->call_multi(amx, args, info, call_args, output);
}

dyn_object conditional_expression::assign(AMX *amx, const args_type &args, const exec_info &info, dyn_object &&value) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->assign(amx, args, info, std::move(value));
}

dyn_object conditional_expression::index(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->index(amx, args, info, indices);
}

dyn_object conditional_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->index_assign(amx, args, info, indices, std::move(value));
}

std::tuple<cell*, size_t, tag_ptr> conditional_expression::address(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices) const
{
	return (cond->execute_bool(amx, args, info) ? on_true : on_false)->address(amx, args, info, indices);
}

tag_ptr conditional_expression::get_tag(const args_type &args) const noexcept
{
	auto tag = on_true->get_tag(args);
	if(tag == on_false->get_tag(args))
	{
		return tag;
	}
	return invalid_tag;
}

cell conditional_expression::get_size(const args_type &args) const noexcept
{
	auto size = on_true->get_size(args);
	if(size == on_false->get_size(args))
	{
		return size;
	}
	return invalid_size;
}

cell conditional_expression::get_rank(const args_type &args) const noexcept
{
	auto rank = on_true->get_rank(args);
	if(rank == on_false->get_rank(args))
	{
		return rank;
	}
	return invalid_rank;
}

const expression_ptr &conditional_expression::get_operand() const noexcept
{
	return cond;
}

const expression_ptr &conditional_expression::get_left() const noexcept
{
	return on_true;
}

const expression_ptr &conditional_expression::get_right() const noexcept
{
	return on_false;
}

void conditional_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	cond->to_string(str);
	str.append(strings::convert(" ? "));
	on_true->to_string(str);
	str.append(strings::convert(" : "));
	on_false->to_string(str);
	str.push_back(')');
}

decltype(expression_pool)::object_ptr conditional_expression::clone() const
{
	return expression_pool.emplace_derived<conditional_expression>(*this);
}

void select_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	call_args_type values;
	list->execute_multi(amx, args, info, values);
	args_type new_args;
	for(const auto &value : values)
	{
		if(new_args.size() == 0)
		{
			new_args.push_back(std::cref(value));
			new_args.insert(new_args.end(), args.begin(), args.end());
		}else{
			new_args[0] = std::cref(value);
		}
		func->execute_multi(amx, new_args, info, output);
	}
}

tag_ptr select_expression::get_tag(const args_type &args) const noexcept
{
	return func->get_tag(args_type());
}

cell select_expression::get_size(const args_type &args) const noexcept
{
	return func->get_size(args_type());
}

cell select_expression::get_rank(const args_type &args) const noexcept
{
	return func->get_rank(args_type());
}

const expression_ptr &select_expression::get_left() const noexcept
{
	return func;
}

const expression_ptr &select_expression::get_right() const noexcept
{
	return list;
}

void select_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	list->to_string(str);
	str.push_back(')');
	str.append(strings::convert("select["));
	func->to_string(str);
	str.push_back(']');
}

decltype(expression_pool)::object_ptr select_expression::clone() const
{
	return expression_pool.emplace_derived<select_expression>(*this);
}

void where_expression::execute_multi(AMX *amx, const args_type &args, const exec_info &info, call_args_type &output) const
{
	call_args_type values;
	list->execute_multi(amx, args, info, values);
	args_type new_args;
	for(auto &value : values)
	{
		if(new_args.size() == 0)
		{
			new_args.push_back(std::cref(value));
			new_args.insert(new_args.end(), args.begin(), args.end());
		}else{
			new_args[0] = std::cref(value);
		}
		if(func->execute_bool(amx, new_args, info))
		{
			output.push_back(std::move(value));
		}
	}
}

tag_ptr where_expression::get_tag(const args_type &args) const noexcept
{
	return list->get_tag(args);
}

cell where_expression::get_size(const args_type &args) const noexcept
{
	return list->get_size(args);
}

cell where_expression::get_rank(const args_type &args) const noexcept
{
	return list->get_rank(args);
}

const expression_ptr &where_expression::get_left() const noexcept
{
	return func;
}

const expression_ptr &where_expression::get_right() const noexcept
{
	return list;
}

void where_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('(');
	list->to_string(str);
	str.push_back(')');
	str.append(strings::convert("where["));
	func->to_string(str);
	str.push_back(']');
}

decltype(expression_pool)::object_ptr where_expression::clone() const
{
	return expression_pool.emplace_derived<where_expression>(*this);
}

cell tagof_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	tag_ptr static_tag = operand->get_tag(args);
	return (static_tag != invalid_tag ? static_tag : operand->execute(amx, args, info).get_tag())->get_id(amx);
}

void tagof_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("tagof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &tagof_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr tagof_expression::clone() const
{
	return expression_pool.emplace_derived<tagof_expression>(*this);
}

cell sizeof_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	if(indices.size() == 0)
	{
		cell static_size = operand->get_size(args);
		return static_size != invalid_size ? static_size : operand->execute(amx, args, info).get_size();
	}else{
		std::vector<cell> cell_indices;
		cell_indices.reserve(indices.size());
		for(const auto &expr : indices)
		{
			if(auto cell_expr = dynamic_cast<const cell_expression*>(expr.get()))
			{
				cell_indices.push_back(cell_expr->execute_inner(amx, args, info));
			}else{
				call_args_type indices;
				expr->execute_multi(amx, args, info, indices);
				for(const auto &index : indices)
				{
					cell value = index.get_cell(0);
					cell_indices.push_back(value);
				}
			}
		}
		return operand->execute(amx, args, info).get_size(cell_indices.data(), cell_indices.size());
	}
}

void sizeof_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("sizeof("));
	operand->to_string(str);
	for(const auto &expr : indices)
	{
		str.push_back('[');
		expr->to_string(str);
		str.push_back(']');
	}
	str.push_back(')');
}

const expression_ptr &sizeof_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr sizeof_expression::clone() const
{
	return expression_pool.emplace_derived<sizeof_expression>(*this);
}

cell rankof_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	cell static_rank = operand->get_rank(args);
	return static_rank != invalid_rank ? static_rank : operand->execute(amx, args, info).get_rank();
}

void rankof_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("rankof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &rankof_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr rankof_expression::clone() const
{
	return expression_pool.emplace_derived<rankof_expression>(*this);
}

dyn_object addressof_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto addr = operand->address(amx, args, info, {});
	auto ptr = reinterpret_cast<unsigned char*>(std::get<0>(addr));
	auto data = amx_GetData(amx);
	if((ptr < data || ptr >= data + amx->hea) && (ptr < data + amx->stk || ptr >= data + amx->stp))
	{
		cell amx_addr;
		cell size = std::get<1>(addr);
		if(size == 0)
		{
			size = 1;
		}
		amx_Allot(amx, size, &amx_addr, &reinterpret_cast<cell*&>(ptr));
		std::memcpy(ptr, std::get<0>(addr), std::get<1>(addr) * sizeof(cell));
	}
	return dyn_object(ptr - data, tags::find_tag((tags::find_tag(tags::tag_address)->name + "@" + std::get<2>(addr)->name).c_str()));
}

tag_ptr addressof_expression::get_tag(const args_type &args) const noexcept
{
	auto inner = operand->get_tag(args);
	if(inner)
	{
		return tags::find_tag((tags::find_tag(tags::tag_address)->name + "@" + inner->name).c_str());
	}
	return nullptr;
}

cell addressof_expression::get_size(const args_type &args) const noexcept
{
	return 1;
}

cell addressof_expression::get_rank(const args_type &args) const noexcept
{
	return 0;
}

void addressof_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("addressof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &addressof_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr addressof_expression::clone() const
{
	return expression_pool.emplace_derived<addressof_expression>(*this);
}

dyn_object nameof_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	strings::cell_string str;
	operand->to_string(str);
	return dyn_object(str.data(), str.size() + 1, tags::find_tag(tags::tag_char));
}

tag_ptr nameof_expression::get_tag(const args_type &args) const noexcept
{
	return tags::find_tag(tags::tag_char);
}

cell nameof_expression::get_size(const args_type &args) const noexcept
{
	strings::cell_string str;
	operand->to_string(str);
	return str.size() + 1;
}

cell nameof_expression::get_rank(const args_type &args) const noexcept
{
	return 1;
}

void nameof_expression::to_string(strings::cell_string &str) const noexcept
{
	str.append(strings::convert("nameof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &nameof_expression::get_operand() const noexcept
{
	return operand;
}

decltype(expression_pool)::object_ptr nameof_expression::clone() const
{
	return expression_pool.emplace_derived<nameof_expression>(*this);
}
dyn_object variant_value_expression::execute(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto var_value = var->execute(amx, args, info);
	if(!(var_value.tag_assignable(tags::find_tag(tags::tag_variant)->base)))
	{
		amx_ExpressionError("extract argument tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_variant)->format_name(), var_value.get_tag()->format_name());
	}
	if(var_value.get_rank() != 0)
	{
		amx_ExpressionError("extract operation requires a single cell value (value of rank %d provided)", var_value.get_rank());
	}
	cell c = var_value.get_cell(0);
	if(c == 0)
	{
		return {};
	}
	dyn_object *var;
	if(!variants::pool.get_by_id(c, var))
	{
		amx_ExpressionError(errors::pointer_invalid, "variant", c);
	}
	return *var;
}

dyn_object variant_value_expression::index_assign(AMX *amx, const args_type &args, const exec_info &info, const call_args_type &indices, dyn_object &&value) const
{
	auto var_value = var->execute(amx, args, info);
	if(!(var_value.tag_assignable(tags::find_tag(tags::tag_variant)->base)))
	{
		amx_ExpressionError("extract argument tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_variant)->format_name(), var_value.get_tag()->format_name());
	}
	if(var_value.get_rank() != 0)
	{
		amx_ExpressionError("extract operation requires a single cell value (value of rank %d provided)", var_value.get_rank());
	}
	cell c = var_value.get_cell(0);
	if(c == 0)
	{
		return {};
	}
	dyn_object *var;
	if(!variants::pool.get_by_id(c, var))
	{
		amx_ExpressionError(errors::pointer_invalid, "variant", c);
	}
	auto &obj = *var;
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("indexed assignment operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	if(!(value.tag_assignable(obj.get_tag())))
	{
		amx_ExpressionError("assigned value tag mismatch (%s: required, %s: provided)", obj.get_tag()->format_name(), value.get_tag()->format_name());
	}
	std::vector<cell> cell_indices;
	for(const auto &index : indices)
	{
		if(!(index.tag_assignable(tags::find_tag(tags::tag_cell))))
		{
			amx_ExpressionError("index tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_cell)->format_name(), index.get_tag()->format_name());
		}
		if(index.get_rank() != 0)
		{
			amx_ExpressionError("index operation requires a single cell value (value of rank %d provided)", index.get_rank());
		}
		cell c = index.get_cell(0);
		if(c < 0)
		{
			amx_ExpressionError("index out of bounds");
		}
		cell_indices.push_back(c);
	}
	obj.set_cell(cell_indices.data(), cell_indices.size(), value.get_cell(0));
	return value;
}

void variant_value_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('*');
	var->to_string(str);
}

const expression_ptr &variant_value_expression::get_operand() const noexcept
{
	return var;
}

decltype(expression_pool)::object_ptr variant_value_expression::clone() const
{
	return expression_pool.emplace_derived<variant_value_expression>(*this);
}

cell variant_expression::execute_inner(AMX *amx, const args_type &args, const exec_info &info) const
{
	auto value = this->value->execute(amx, args, info);
	if(value.is_null())
	{
		return 0;
	}
	auto &ptr = variants::pool.emplace(std::move(value));
	return variants::pool.get_id(ptr);
}

void variant_expression::to_string(strings::cell_string &str) const noexcept
{
	str.push_back('&');
	value->to_string(str);
}

const expression_ptr &variant_expression::get_operand() const noexcept
{
	return value;
}

decltype(expression_pool)::object_ptr variant_expression::clone() const
{
	return expression_pool.emplace_derived<variant_expression>(*this);
}
