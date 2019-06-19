#include "expressions.h"
#include "tags.h"
#include "errors.h"

#include <string>
#include <stdarg.h>

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

dyn_object expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	amx_ExpressionError("attempt to call a non-function expression");
	return {};
}

dyn_object expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	amx_ExpressionError("attempt to assign to a non-lvalue expression");
	return {};
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

dyn_object constant_expression::execute(AMX *amx, const args_type &args) const
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

expression_ptr constant_expression::clone() const
{
	return std::make_shared<constant_expression>(*this);
}

dyn_object weak_expression::execute(AMX *amx, const args_type &args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->execute(amx, args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

dyn_object weak_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	if(auto lock = ptr.lock())
	{
		return lock->call(amx, args, call_args);
	}
	amx_ExpressionError("weakly linked expression was destroyed");
	return {};
}

dyn_object weak_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	if(auto lock = ptr.lock())
	{
		return lock->assign(amx, args, std::move(value));
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

expression_ptr weak_expression::clone() const
{
	return std::make_shared<weak_expression>(*this);
}

dyn_object arg_expression::execute(AMX *amx, const args_type &args) const
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

expression_ptr arg_expression::clone() const
{
	return std::make_shared<arg_expression>(*this);
}

dyn_object comma_expression::execute(AMX *amx, const args_type &args) const
{
	left->execute(amx, args);
	return right->execute(amx, args);
}

dyn_object comma_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	left->execute(amx, args);
	return right->call(amx, args, call_args);
}

dyn_object comma_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	left->execute(amx, args);
	return right->assign(amx, args, std::move(value));
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

expression_ptr comma_expression::clone() const
{
	return std::make_shared<comma_expression>(*this);
}

dyn_object assign_expression::execute(AMX *amx, const args_type &args) const
{
	return left->assign(amx, args, right->execute(amx, args));
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

expression_ptr assign_expression::clone() const
{
	return std::make_shared<assign_expression>(*this);
}

dyn_object try_expression::execute(AMX *amx, const args_type &args) const
{
	try{
		return main->execute(amx, args);
	}catch(const errors::native_error&)
	{
		return fallback->execute(amx, args);
	}
}

dyn_object try_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	try{
		return main->call(amx, args, call_args);
	}catch(const errors::native_error&)
	{
		return fallback->call(amx, args, call_args);
	}
}

dyn_object try_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	try{
		return main->assign(amx, args, std::move(value));
	}catch(const errors::native_error&)
	{
		return fallback->assign(amx, args, std::move(value));
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

expression_ptr try_expression::clone() const
{
	return std::make_shared<try_expression>(*this);
}

dyn_object call_expression::execute(AMX *amx, const args_type &args) const
{
	std::vector<dyn_object> func_args;
	for(const auto &arg : this->args)
	{
		func_args.push_back(arg->execute(amx, args));
	}
	return func->call(amx, args, func_args);
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

expression_ptr call_expression::clone() const
{
	return std::make_shared<call_expression>(*this);
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

auto bind_expression::combine_args(AMX *amx, const args_type &args, std::vector<dyn_object> &storage) const -> args_type
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
			storage.push_back(arg->execute(amx, args));
			new_args.push_back(std::ref(storage.back()));
		}
	}
	for(const auto &arg_ref : args)
	{
		new_args.push_back(arg_ref);
	}
	return new_args;
}

dyn_object bind_expression::execute(AMX *amx, const args_type &args) const
{
	std::vector<dyn_object> storage;
	return operand->execute(amx, combine_args(amx, args, storage));
}

dyn_object bind_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	std::vector<dyn_object> storage;
	return operand->call(amx, combine_args(amx, args, storage), call_args);
}

dyn_object bind_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	std::vector<dyn_object> storage;
	return operand->assign(amx, combine_args(amx, args, storage), std::move(value));
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

expression_ptr bind_expression::clone() const
{
	return std::make_shared<bind_expression>(*this);
}

dyn_object cast_expression::execute(AMX *amx, const args_type &args) const
{
	return dyn_object(operand->execute(amx, args), new_tag);
}

dyn_object cast_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	return dyn_object(operand->call(amx, args, call_args), new_tag);
}

dyn_object cast_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	return dyn_object(operand->assign(amx, args, dyn_object(std::move(value), operand->get_tag(args))), new_tag);
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

expression_ptr cast_expression::clone() const
{
	return std::make_shared<cast_expression>(*this);
}

dyn_object symbol_expression::execute(AMX *amx, const args_type &args) const
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
						amx_ExpressionError("unrecognized array rank %d", symbol->dim);
				}
			}
		}
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

dyn_object symbol_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident != iFUNCTN)
		{
			return expression::call(amx, args, call_args);
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
					if(test_tag && !arg.tag_assignable(test_tag))
					{
						amx_ExpressionError("argument tag mismatch (%s: required, %s: provided)", test_tag->format_name(), arg.get_tag()->format_name());
					}
				}
			}
		}
		if(argsneeded)
		{
			amx_ExpressionError("too few arguments provided to a function (%d needed, %d given)", argsneeded / sizeof(cell), argslen / sizeof(cell));
		}

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
		cell ret;
		int old_error = amx->error;
		int err = amx_Exec(amx, &ret, AMX_EXEC_CONT);
		amx->error = old_error;
		if(err == AMX_ERR_NONE)
		{
			return dyn_object(ret, get_tag(args));
		}
		amx_ExpressionError("inner function raised an AMX error %d", err);
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

dyn_object symbol_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	if(auto obj = target_amx.lock())
	{
		if(symbol->ident == iFUNCTN)
		{
			return expression::assign(amx, args, std::move(value));
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
		}
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

expression_ptr symbol_expression::clone() const
{
	return std::make_shared<symbol_expression>(*this);
}

dyn_object native_expression::execute(AMX *amx, const args_type &args) const
{
	amx_ExpressionError("attempt to obtain the value of a function");
	return {};
}

dyn_object native_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	auto data = amx_GetData(amx);
	auto stk = reinterpret_cast<cell*>(data + amx->stk);

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

expression_ptr native_expression::clone() const
{
	return std::make_shared<native_expression>(*this);
}

dyn_object local_native_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	if(auto obj = target_amx.lock())
	{
		return native_expression::call(*obj, args, call_args);
	}
	amx_ExpressionError("target AMX was unloaded");
	return {};
}

expression_ptr local_native_expression::clone() const
{
	return std::make_shared<local_native_expression>(*this);
}

bool logic_and_expression::execute_inner(AMX *amx, const args_type &args) const
{
	if(auto left_logic = dynamic_cast<const bool_expression*>(left.get()))
	{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return left_logic->execute_inner(amx, args) && right_logic->execute_inner(amx, args);
		}else{
			return left_logic->execute_inner(amx, args) && !!right->execute(amx, args);
		}
	}else{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return !!left->execute(amx, args) && right_logic->execute_inner(amx, args);
		}else{
			return !!left->execute(amx, args) && !!right->execute(amx, args);
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

expression_ptr logic_and_expression::clone() const
{
	return std::make_shared<logic_and_expression>(*this);
}

bool logic_or_expression::execute_inner(AMX *amx, const args_type &args) const
{
	if(auto left_logic = dynamic_cast<const bool_expression*>(left.get()))
	{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return left_logic->execute_inner(amx, args) || right_logic->execute_inner(amx, args);
		}else{
			return left_logic->execute_inner(amx, args) || !!right->execute(amx, args);
		}
	}else{
		if(auto right_logic = dynamic_cast<const bool_expression*>(right.get()))
		{
			return !!left->execute(amx, args) || right_logic->execute_inner(amx, args);
		}else{
			return !!left->execute(amx, args) || !!right->execute(amx, args);
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

expression_ptr logic_or_expression::clone() const
{
	return std::make_shared<logic_or_expression>(*this);
}

dyn_object conditional_expression::execute(AMX *amx, const args_type &args) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args) ? on_true->execute(amx, args) : on_false->execute(amx, args);
	}else{
		return !cond->execute(amx, args) ? on_false->execute(amx, args) : on_true->execute(amx, args);
	}
}

dyn_object conditional_expression::call(AMX *amx, const args_type &args, const call_args_type &call_args) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args) ? on_true->call(amx, args, call_args) : on_false->call(amx, args, call_args);
	}else{
		return !cond->execute(amx, args) ? on_false->call(amx, args, call_args) : on_true->call(amx, args, call_args);
	}
}

dyn_object conditional_expression::assign(AMX *amx, const args_type &args, dyn_object &&value) const
{
	if(auto logic_cond = dynamic_cast<const bool_expression*>(cond.get()))
	{
		return logic_cond->execute_inner(amx, args) ? on_true->assign(amx, args, std::move(value)) : on_false->assign(amx, args, std::move(value));
	}else{
		return !cond->execute(amx, args) ? on_false->assign(amx, args, std::move(value)) : on_true->assign(amx, args, std::move(value));
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

expression_ptr conditional_expression::clone() const
{
	return std::make_shared<conditional_expression>(*this);
}

cell tagof_expression::execute_inner(AMX *amx, const args_type &args) const
{
	tag_ptr static_tag = operand->get_tag(args);
	return (static_tag ? static_tag : operand->execute(amx, args).get_tag())->get_id(amx);
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

expression_ptr tagof_expression::clone() const
{
	return std::make_shared<tagof_expression>(*this);
}

cell sizeof_expression::execute_inner(AMX *amx, const args_type &args) const
{
	if(indices.size() == 0)
	{
		cell static_size = operand->get_size(args);
		return static_size ? static_size : operand->execute(amx, args).get_size();
	}else{
		std::vector<cell> cell_indices;
		cell_indices.reserve(indices.size());
		for(const auto &expr : indices)
		{
			if(auto cell_expr = dynamic_cast<const cell_expression*>(expr.get()))
			{
				cell_indices.push_back(cell_expr->execute_inner(amx, args));
			}else{
				cell value = expr->execute(amx, args).get_cell(0);
				cell_indices.push_back(value);
			}
		}
		return operand->execute(amx, args).get_size(cell_indices.data(), cell_indices.size());
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

expression_ptr sizeof_expression::clone() const
{
	return std::make_shared<sizeof_expression>(*this);
}

cell rankof_expression::execute_inner(AMX *amx, const args_type &args) const
{
	cell static_rank = operand->get_rank(args);
	return static_rank != -1 ? static_rank : operand->execute(amx, args).get_rank();
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

expression_ptr rankof_expression::clone() const
{
	return std::make_shared<rankof_expression>(*this);
}
