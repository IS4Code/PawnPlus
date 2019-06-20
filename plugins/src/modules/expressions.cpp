#include "expressions.h"
#include "tags.h"
#include "errors.h"
#include "modules/variants.h"

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

dyn_object expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	auto result = execute(amx, args, env);
	expression *ptr;
	if(!(result.tag_assignable(tags::find_tag(tags::tag_expression))) || result.get_rank() != 0 || !expression_pool.get_by_id(result.get_cell(0), ptr))
	{
		amx_ExpressionError("attempt to call a non-function expression");
	}
	args_type new_args;
	new_args.reserve(call_args.size());
	for(const auto &arg : call_args)
	{
		new_args.push_back(std::ref(arg));
	}
	return ptr->execute(amx, new_args, env);
}

dyn_object expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	amx_ExpressionError("attempt to assign to a non-lvalue expression");
	return {};
}

dyn_object expression::index(AMX *amx, const args_type &args, env_type &env, const std::vector<dyn_object> &indices) const
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
	auto value = execute(amx, args, env);
	return dyn_object(value.get_cell(cell_indices.data(), cell_indices.size()), value.get_tag());
}

std::tuple<cell*, size_t, tag_ptr> expression::address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const
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
	auto value = execute(amx, args, env);
	cell *arr, size;
	arr = value.get_array(cell_indices.data(), cell_indices.size(), size);
	cell amx_addr, *addr;
	amx_Allot(amx, size ? size : 1, &amx_addr, &addr);
	std::memcpy(addr, arr, size * sizeof(cell));
	return {addr, size, value.get_tag()};
}

tag_ptr expression::get_tag(const args_type &args) const
{
	return nullptr;
}

cell expression::get_size(const args_type &args) const
{
	return 0;
}

cell expression::get_rank(const args_type &args) const
{
	return -1;
}

int &expression::operator[](size_t index) const
{
	static int unused;
	return unused;
}

expression *expression_base::get()
{
	return this;
}

const expression *expression_base::get() const
{
	return this;
}

dyn_object constant_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	return value;
}

tag_ptr constant_expression::get_tag(const args_type &args) const
{
	return value.get_tag();
}

cell constant_expression::get_size(const args_type &args) const
{
	return value.get_size();
}

cell constant_expression::get_rank(const args_type &args) const
{
	return value.get_rank();
}

void constant_expression::to_string(strings::cell_string &str) const
{
	str.append(value.to_string());
}

decltype(expression_pool)::object_ptr constant_expression::clone() const
{
	return expression_pool.emplace_derived<constant_expression>(*this);
}

dyn_object weak_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	if(auto lock = ptr.lock())
	{
		return lock->execute(amx, args, env);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

dyn_object weak_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->call(amx, args, env, call_args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

dyn_object weak_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	if(auto lock = ptr.lock())
	{
		return lock->assign(amx, args, env, std::move(value));
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

tag_ptr weak_expression::get_tag(const args_type &args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->get_tag(args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return nullptr;
}

cell weak_expression::get_size(const args_type &args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->get_size(args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return 0;
}

cell weak_expression::get_rank(const args_type &args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->get_rank(args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return -1;
}

void weak_expression::to_string(strings::cell_string &str) const
{
	if(auto lock = ptr.lock())
	{
		lock->to_string(str);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
}

decltype(expression_pool)::object_ptr weak_expression::clone() const
{
	return expression_pool.emplace_derived<weak_expression>(*this);
}

dyn_object arg_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	if(index >= args.size())
	{
		amx_ExpressionError("expression argument was not provided");
	}
	return args[index].get();
}

tag_ptr arg_expression::get_tag(const args_type &args) const
{
	if(index >= args.size())
	{
		amx_ExpressionError("expression argument was not provided");
	}
	return args[index].get().get_tag();
}

cell arg_expression::get_size(const args_type &args) const
{
	if(index >= args.size())
	{
		amx_ExpressionError("expression argument was not provided");
	}
	return args[index].get().get_size();
}

cell arg_expression::get_rank(const args_type &args) const
{
	if(index >= args.size())
	{
		amx_ExpressionError("expression argument was not provided");
	}
	return args[index].get().get_rank();
}

void arg_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert("$arg"));
	str.append(strings::convert(std::to_string(index)));
}

decltype(expression_pool)::object_ptr arg_expression::clone() const
{
	return expression_pool.emplace_derived<arg_expression>(*this);
}

dyn_object comma_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	left->execute(amx, args, env);
	return right->execute(amx, args, env);
}

dyn_object comma_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	left->execute(amx, args, env);
	return right->call(amx, args, env, call_args);
}

dyn_object comma_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	left->execute(amx, args, env);
	return right->assign(amx, args, env, std::move(value));
}

tag_ptr comma_expression::get_tag(const args_type &args) const
{
	return right->get_tag(args);
}

cell comma_expression::get_size(const args_type &args) const
{
	return right->get_size(args);
}

cell comma_expression::get_rank(const args_type &args) const
{
	return right->get_rank(args);
}

void comma_expression::to_string(strings::cell_string &str) const
{
	str.push_back('(');
	left->to_string(str);
	str.push_back(',');
	str.push_back(' ');
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &comma_expression::get_left() const
{
	return left;
}

const expression_ptr &comma_expression::get_right() const
{
	return right;
}

decltype(expression_pool)::object_ptr comma_expression::clone() const
{
	return expression_pool.emplace_derived<comma_expression>(*this);
}

dyn_object assign_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	return left->assign(amx, args, env, right->execute(amx, args, env));
}

tag_ptr assign_expression::get_tag(const args_type &args) const
{
	return left->get_tag(args);
}

cell assign_expression::get_size(const args_type &args) const
{
	return left->get_size(args);
}

cell assign_expression::get_rank(const args_type &args) const
{
	return left->get_rank(args);
}

void assign_expression::to_string(strings::cell_string &str) const
{
	left->to_string(str);
	str.append(strings::convert(" = "));
	right->to_string(str);
}

const expression_ptr &assign_expression::get_left() const
{
	return left;
}

const expression_ptr &assign_expression::get_right() const
{
	return right;
}

decltype(expression_pool)::object_ptr assign_expression::clone() const
{
	return expression_pool.emplace_derived<assign_expression>(*this);
}

dyn_object try_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	try{
		return main->execute(amx, args, env);
	}catch(const errors::native_error&)
	{
		return fallback->execute(amx, args, env);
	}
}

dyn_object try_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	try{
		return main->call(amx, args, env, call_args);
	}catch(const errors::native_error&)
	{
		return fallback->call(amx, args, env, call_args);
	}
}

dyn_object try_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	try{
		return main->assign(amx, args, env, std::move(value));
	}catch(const errors::native_error&)
	{
		return fallback->assign(amx, args, env, std::move(value));
	}
}

void try_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert("try["));
	main->to_string(str);
	str.append(strings::convert("]catch["));
	fallback->to_string(str);
	str.push_back(']');
}

const expression_ptr &try_expression::get_left() const
{
	return main;
}

const expression_ptr &try_expression::get_right() const
{
	return fallback;
}

decltype(expression_pool)::object_ptr try_expression::clone() const
{
	return expression_pool.emplace_derived<try_expression>(*this);
}

dyn_object call_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	std::vector<dyn_object> func_args;
	for(const auto &arg : this->args)
	{
		func_args.push_back(arg->execute(amx, args, env));
	}
	return func->call(amx, args, env, func_args);
}

void call_expression::to_string(strings::cell_string &str) const
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

tag_ptr call_expression::get_tag(const args_type &args) const
{
	return func->get_tag(args);
}

cell call_expression::get_size(const args_type &args) const
{
	return func->get_size(args);
}

cell call_expression::get_rank(const args_type &args) const
{
	return func->get_rank(args);
}

const expression_ptr &call_expression::get_operand() const
{
	return func;
}

decltype(expression_pool)::object_ptr call_expression::clone() const
{
	return expression_pool.emplace_derived<call_expression>(*this);
}

dyn_object index_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	std::vector<dyn_object> indices;
	for(const auto &index : this->indices)
	{
		indices.push_back(index->execute(amx, args, env));
	}
	return arr->index(amx, args, env, indices);
}

dyn_object index_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("indexed assignment operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	auto addr = address(amx, args, env, {});
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

dyn_object index_expression::index(AMX *amx, const args_type &args, env_type &env, const std::vector<dyn_object> &indices) const
{
	std::vector<dyn_object> new_indices;
	new_indices.reserve(this->indices.size() + indices.size());
	for(const auto &index : this->indices)
	{
		new_indices.push_back(index->execute(amx, args, env));
	}
	for(const auto &index : indices)
	{
		new_indices.push_back(index);
	}
	return arr->index(amx, args, env, new_indices);
}

std::tuple<cell*, size_t, tag_ptr> index_expression::address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const
{
	std::vector<dyn_object> new_indices;
	new_indices.reserve(this->indices.size() + indices.size());
	for(const auto &index : this->indices)
	{
		new_indices.push_back(index->execute(amx, args, env));
	}
	for(const auto &index : indices)
	{
		new_indices.push_back(index);
	}
	return arr->address(amx, args, env, new_indices);
}

void index_expression::to_string(strings::cell_string &str) const
{
	arr->to_string(str);
	for(const auto &index : indices)
	{
		str.push_back('[');
		index->to_string(str);
		str.push_back(']');
	}
}

tag_ptr index_expression::get_tag(const args_type &args) const
{
	return arr->get_tag(args);
}

cell index_expression::get_size(const args_type &args) const
{
	return 1;
}

cell index_expression::get_rank(const args_type &args) const
{
	return 0;
}

const expression_ptr &index_expression::get_operand() const
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
			new_args.push_back(std::ref(const_expr->get_value()));
		}else{
			new_args.clear();
			return new_args;
		}
	}
	for(const auto &arg_ref : args)
	{
		new_args.push_back(arg_ref);
	}
	return new_args;
}

auto bind_expression::combine_args(AMX *amx, const args_type &args, env_type &env, std::vector<dyn_object> &storage) const -> args_type
{
	storage.reserve(base_args.size() + args.size());
	args_type new_args;
	new_args.reserve(base_args.size() + args.size());
	for(const auto &arg : base_args)
	{
		if(auto const_expr = dynamic_cast<const constant_expression*>(arg.get()))
		{
			new_args.push_back(std::ref(const_expr->get_value()));
		}else{
			storage.push_back(arg->execute(amx, args, env));
			new_args.push_back(std::ref(storage.back()));
		}
	}
	for(const auto &arg_ref : args)
	{
		new_args.push_back(arg_ref);
	}
	return new_args;
}

dyn_object bind_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	std::vector<dyn_object> storage;
	return operand->execute(amx, combine_args(amx, args, env, storage), env);
}

dyn_object bind_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	std::vector<dyn_object> storage;
	return operand->call(amx, combine_args(amx, args, env, storage), env, call_args);
}

dyn_object bind_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	std::vector<dyn_object> storage;
	return operand->assign(amx, combine_args(amx, args, env, storage), env, std::move(value));
}

void bind_expression::to_string(strings::cell_string &str) const
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

tag_ptr bind_expression::get_tag(const args_type &args) const
{
	auto all_args = combine_args(args);
	if(all_args.size() >= base_args.size())
	{
		return operand->get_tag(all_args);
	}
	return nullptr;
}

cell bind_expression::get_size(const args_type &args) const
{
	auto all_args = combine_args(args);
	if(all_args.size() >= base_args.size())
	{
		operand->get_size(all_args);
	}
	return 0;
}

cell bind_expression::get_rank(const args_type &args) const
{
	auto all_args = combine_args(args);
	if(all_args.size() >= base_args.size())
	{
		operand->get_rank(all_args);
	}
	return -1;
}

const expression_ptr &bind_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr bind_expression::clone() const
{
	return expression_pool.emplace_derived<bind_expression>(*this);
}

dyn_object cast_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	return dyn_object(operand->execute(amx, args, env), new_tag);
}

dyn_object cast_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	return dyn_object(operand->call(amx, args, env, call_args), new_tag);
}

dyn_object cast_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	return dyn_object(operand->assign(amx, args, env, dyn_object(std::move(value), operand->get_tag(args))), new_tag);
}

void cast_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert(new_tag->format_name()));
	str.push_back(':');
	operand->to_string(str);
}

tag_ptr cast_expression::get_tag(const args_type &args) const
{
	return new_tag;
}

cell cast_expression::get_size(const args_type &args) const
{
	return operand->get_size(args);
}

cell cast_expression::get_rank(const args_type &args) const
{
	return operand->get_rank(args);
}

const expression_ptr &cast_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr cast_expression::clone() const
{
	return expression_pool.emplace_derived<cast_expression>(*this);
}

dyn_object array_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	std::vector<cell> data;
	tag_ptr tag = nullptr;
	for(const auto &arg : this->args)
	{
		dyn_object value = arg->execute(amx, args, env);
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
	return dyn_object(data.data(), data.size(), tag ? tag : tags::find_tag(tags::tag_cell));
}

tag_ptr array_expression::get_tag(const args_type &args) const
{
	if(args.size() > 0)
	{
		return this->args[0]->get_tag(args);
	}else{
		return tags::find_tag(tags::tag_cell);
	}
}

cell array_expression::get_size(const args_type &args) const
{
	return args.size();
}

cell array_expression::get_rank(const args_type &args) const
{
	return 1;
}

void array_expression::to_string(strings::cell_string &str) const
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

dyn_object global_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	auto it = env.find(key);
	if(it == env.end())
	{
		amx_ExpressionError("symbol is not defined");
	}
	return it->second;
}

dyn_object global_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	return env[key] = std::move(value);
}

void global_expression::to_string(strings::cell_string &str) const
{
	str.append(name);
}

decltype(expression_pool)::object_ptr global_expression::clone() const
{
	return expression_pool.emplace_derived<global_expression>(*this);
}

dyn_object symbol_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			amx_ExpressionError("attempt to obtain the value of a function");
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
				return {};
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

dyn_object symbol_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident != iFUNCTN)
		{
			return expression::call(amx, args, env, call_args);
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
				}else if(!argsneeded)
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
		amx_ExpressionError("inner function raised an AMX error %d", err);
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

dyn_object symbol_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			return expression::assign(amx, args, env, std::move(value));
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
				return {};
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

dyn_object symbol_expression::index(AMX *amx, const args_type &args, env_type &env, const std::vector<dyn_object> &indices) const
{
	auto addr = address(amx, args, env, indices);
	if(std::get<1>(addr) == 0)
	{
		amx_ExpressionError("index out of bounds");
	}
	return dyn_object(std::get<0>(addr)[0], std::get<2>(addr));
}

std::tuple<cell*, size_t, tag_ptr> symbol_expression::address(AMX *amx, const args_type &args, env_type &env, const call_args_type &indices) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			return expression::address(amx, args, env, indices);
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
				return {};
			}
			ptr = reinterpret_cast<cell*>(data + frm + symbol->address);
		}

		if(symbol->ident == iREFERENCE || symbol->ident == iREFARRAY)
		{
			ptr = reinterpret_cast<cell*>(data + *ptr);
		}
		tag_ptr tag_ptr = get_tag(args);

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
				return {ptr, 1, tag_ptr};
			}else if(symbol->dim == 1)
			{
				const AMX_DBG_SYMDIM *dim;
				if(dbg_GetArrayDim(debug, symbol, &dim) == AMX_ERR_NONE)
				{
					if(cell_indices[0] > dim[0].size)
					{
						amx_ExpressionError("index out of bounds");
					}
					return {ptr + cell_indices[0], dim[0].size - cell_indices[0], tag_ptr};
				}
			}
			amx_ExpressionError("array rank %d not supported", symbol->dim);
		}
		amx_ExpressionError("incorrect number of indices (%d needed, %d given)", symbol->dim, indices.size());
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

tag_ptr symbol_expression::get_tag(const args_type &args) const
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
	amx_ExpressionError("target AMX was unloaded");
	return nullptr;
}

cell symbol_expression::get_size(const args_type &args) const
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
	amx_ExpressionError("target AMX was unloaded");
	return 0;
}

cell symbol_expression::get_rank(const args_type &args) const
{
	if(!target_amx.expired())
	{
		return symbol->dim;
	}
	amx_ExpressionError("target AMX was unloaded");
	return -1;
}

void symbol_expression::to_string(strings::cell_string &str) const
{
	if(!target_amx.expired())
	{
		str.append(strings::convert(symbol->name));
		return;
	}
	amx_ExpressionError("target AMX was unloaded");
}

decltype(expression_pool)::object_ptr symbol_expression::clone() const
{
	return expression_pool.emplace_derived<symbol_expression>(*this);
}

dyn_object native_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	amx_ExpressionError("attempt to obtain the value of a function");
	return {};
}

dyn_object native_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
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
		return dyn_object(ret, get_tag(args));
	}
	amx_ExpressionError("inner function raised an AMX error %d", err);
	return {};
}

tag_ptr native_expression::get_tag(const args_type &args) const
{
	return tags::find_tag(tags::tag_cell);
}

cell native_expression::get_size(const args_type &args) const
{
	return 1;
}

cell native_expression::get_rank(const args_type &args) const
{
	return 0;
}

void native_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert(name));
}

decltype(expression_pool)::object_ptr native_expression::clone() const
{
	return expression_pool.emplace_derived<native_expression>(*this);
}

dyn_object local_native_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		return native_expression::call(*obj, args, env, call_args);
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

decltype(expression_pool)::object_ptr local_native_expression::clone() const
{
	return expression_pool.emplace_derived<local_native_expression>(*this);
}

cell quote_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	return expression_pool.get_id(static_cast<const expression_base*>(operand.get())->clone());
}

void quote_expression::to_string(strings::cell_string &str) const
{
	str.push_back('<');
	operand->to_string(str);
	str.push_back('>');
}

const expression_ptr &quote_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr quote_expression::clone() const
{
	return expression_pool.emplace_derived<quote_expression>(*this);
}

bool logic_and_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	if(auto left_logic = dynamic_cast<const bool_expression*>(left.get()))
	{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return left_logic->execute_inner(amx, args, env) && right_logic->execute_inner(amx, args, env);
		}else{
			return left_logic->execute_inner(amx, args, env) && !!right->execute(amx, args, env);
		}
	}else{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return !!left->execute(amx, args, env) && right_logic->execute_inner(amx, args, env);
		}else{
			return !!left->execute(amx, args, env) && !!right->execute(amx, args, env);
		}
	}
}

void logic_and_expression::to_string(strings::cell_string &str) const
{
	str.push_back('(');
	left->to_string(str);
	str.append(strings::convert(" && "));
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &logic_and_expression::get_left() const
{
	return left;
}

const expression_ptr &logic_and_expression::get_right() const
{
	return right;
}

decltype(expression_pool)::object_ptr logic_and_expression::clone() const
{
	return expression_pool.emplace_derived<logic_and_expression>(*this);
}

bool logic_or_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	if(auto left_logic = dynamic_cast<const bool_expression*>(left.get()))
	{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return left_logic->execute_inner(amx, args, env) || right_logic->execute_inner(amx, args, env);
		}else{
			return left_logic->execute_inner(amx, args, env) || !!right->execute(amx, args, env);
		}
	}else{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return !!left->execute(amx, args, env) || right_logic->execute_inner(amx, args, env);
		}else{
			return !!left->execute(amx, args, env) || !!right->execute(amx, args, env);
		}
	}
}

void logic_or_expression::to_string(strings::cell_string &str) const
{
	str.push_back('(');
	left->to_string(str);
	str.append(strings::convert(" || "));
	right->to_string(str);
	str.push_back(')');
}

const expression_ptr &logic_or_expression::get_left() const
{
	return left;
}

const expression_ptr &logic_or_expression::get_right() const
{
	return right;
}

decltype(expression_pool)::object_ptr logic_or_expression::clone() const
{
	return expression_pool.emplace_derived<logic_or_expression>(*this);
}

dyn_object conditional_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args, env) ? on_true->execute(amx, args, env) : on_false->execute(amx, args, env);
	}else{
		return !cond->execute(amx, args, env) ? on_false->execute(amx, args, env) : on_true->execute(amx, args, env);
	}
}

dyn_object conditional_expression::call(AMX *amx, const args_type &args, env_type &env, const call_args_type &call_args) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args, env) ? on_true->call(amx, args, env, call_args) : on_false->call(amx, args, env, call_args);
	}else{
		return !cond->execute(amx, args, env) ? on_false->call(amx, args, env, call_args) : on_true->call(amx, args, env, call_args);
	}
}

dyn_object conditional_expression::assign(AMX *amx, const args_type &args, env_type &env, dyn_object &&value) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args, env) ? on_true->assign(amx, args, env, std::move(value)) : on_false->assign(amx, args, env, std::move(value));
	}else{
		return !cond->execute(amx, args, env) ? on_false->assign(amx, args, env, std::move(value)) : on_true->assign(amx, args, env, std::move(value));
	}
}

const expression_ptr &conditional_expression::get_operand() const
{
	return cond;
}

const expression_ptr &conditional_expression::get_left() const
{
	return on_true;
}

const expression_ptr &conditional_expression::get_right() const
{
	return on_false;
}

void conditional_expression::to_string(strings::cell_string &str) const
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

cell tagof_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	tag_ptr static_tag = operand->get_tag(args);
	return (static_tag ? static_tag : operand->execute(amx, args, env).get_tag())->get_id(amx);
}

void tagof_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert("tagof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &tagof_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr tagof_expression::clone() const
{
	return expression_pool.emplace_derived<tagof_expression>(*this);
}

cell sizeof_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	if(indices.size() == 0)
	{
		cell static_size = operand->get_size(args);
		return static_size ? static_size : operand->execute(amx, args, env).get_size();
	}else{
		std::vector<cell> cell_indices;
		cell_indices.reserve(indices.size());
		for(const auto &expr : indices)
		{
			if(auto cell_expr = dynamic_cast<const cell_expression*>(expr.get()))
			{
				cell_indices.push_back(cell_expr->execute_inner(amx, args, env));
			}else{
				cell value = expr->execute(amx, args, env).get_cell(0);
				cell_indices.push_back(value);
			}
		}
		return operand->execute(amx, args, env).get_size(cell_indices.data(), cell_indices.size());
	}
}

void sizeof_expression::to_string(strings::cell_string &str) const
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

const expression_ptr &sizeof_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr sizeof_expression::clone() const
{
	return expression_pool.emplace_derived<sizeof_expression>(*this);
}

cell rankof_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	cell static_rank = operand->get_rank(args);
	return static_rank != -1 ? static_rank : operand->execute(amx, args, env).get_rank();
}

void rankof_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert("rankof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &rankof_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr rankof_expression::clone() const
{
	return expression_pool.emplace_derived<rankof_expression>(*this);
}

dyn_object addressof_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	auto addr = operand->address(amx, args, env, {});
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

tag_ptr addressof_expression::get_tag(const args_type &args) const
{
	auto inner = operand->get_tag(args);
	if(inner)
	{
		return tags::find_tag((tags::find_tag(tags::tag_address)->name + "@" + inner->name).c_str());
	}
	return nullptr;
}

cell addressof_expression::get_size(const args_type &args) const
{
	return 1;
}

cell addressof_expression::get_rank(const args_type &args) const
{
	return 0;
}

void addressof_expression::to_string(strings::cell_string &str) const
{
	str.append(strings::convert("addressof("));
	operand->to_string(str);
	str.push_back(')');
}

const expression_ptr &addressof_expression::get_operand() const
{
	return operand;
}

decltype(expression_pool)::object_ptr addressof_expression::clone() const
{
	return expression_pool.emplace_derived<addressof_expression>(*this);
}

dyn_object variant_value_expression::execute(AMX *amx, const args_type &args, env_type &env) const
{
	auto value = var->execute(amx, args, env);
	if(!(value.tag_assignable(tags::find_tag(tags::tag_variant)->base)))
	{
		amx_ExpressionError("extract argument tag mismatch (%s: required, %s: provided)", tags::find_tag(tags::tag_variant)->format_name(), value.get_tag()->format_name());
	}
	if(value.get_rank() != 0)
	{
		amx_ExpressionError("extract operation requires a single cell value (value of rank %d provided)", value.get_rank());
	}
	cell c = value.get_cell(0);
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

void variant_value_expression::to_string(strings::cell_string &str) const
{
	str.push_back('*');
	var->to_string(str);
}

const expression_ptr &variant_value_expression::get_operand() const
{
	return var;
}

decltype(expression_pool)::object_ptr variant_value_expression::clone() const
{
	return expression_pool.emplace_derived<variant_value_expression>(*this);
}

cell variant_expression::execute_inner(AMX *amx, const args_type &args, env_type &env) const
{
	auto value = this->value->execute(amx, args, env);
	if(value.is_null())
	{
		return 0;
	}
	auto &ptr = variants::pool.emplace(std::move(value));
	return variants::pool.get_id(ptr);
}

void variant_expression::to_string(strings::cell_string &str) const
{
	str.push_back('&');
	value->to_string(str);
}

const expression_ptr &variant_expression::get_operand() const
{
	return value;
}

decltype(expression_pool)::object_ptr variant_expression::clone() const
{
	return expression_pool.emplace_derived<variant_expression>(*this);
}
