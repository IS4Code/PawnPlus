#include "stored_param.h"
#include "modules/variants.h"
#include "errors.h"
#include <cstring>
#include <limits>

stored_param stored_param::create(AMX *amx, char format, const cell *args, size_t &argi, size_t numargs)
{
	cell *addr, *addr2;
	switch(format)
	{
		case 'a':
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 2);
			addr = amx_GetAddrSafe(amx, args[argi]);
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr2 = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(std::basic_string<cell>(addr, *addr2));
		}
		case 's':
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(std::basic_string<cell>(addr));
		}
		case 'e':
		{
			return stored_param();
		}
		case 'v':
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			dyn_object *ptr;
			if(!variants::pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "variant", *addr);
			return stored_param(*ptr);
		}
		case 'h':
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			std::shared_ptr<handle_t> ptr;
			if(!handle_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "handle", *addr);
			return stored_param(std::move(ptr));
		}
		case 'x':
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			expression_ptr ptr;
			if(!expression_pool.get_by_id(*addr, ptr)) amx_LogicError(errors::pointer_invalid, "expression", *addr);
			return stored_param(std::move(ptr));
		}
		default:
		{
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(*addr);
		}
	}
}

void stored_param::push(AMX *amx, int id) const
{
	switch(type)
	{
		case param_type::cell_param:
			amx_Push(amx, cell_value);
			break;
		case param_type::self_id_param:
			amx_Push(amx, id);
			break;
		case param_type::string_param:
			cell addr, *phys_addr;
			amx_AllotSafe(amx, string_value.size() + 1, &addr, &phys_addr);
			std::memcpy(phys_addr, string_value.c_str(), (string_value.size() + 1) * sizeof(cell));
			amx_Push(amx, addr);
			break;
		case param_type::variant_param:
			amx_Push(amx, variant_value.store(amx));
			break;
		case param_type::handle_param:
			amx_Push(amx, handle_value->get().store(amx));
			break;
		case param_type::expression_param:
			try{
				auto result = expr_value->execute(expression::args_type(), expression::exec_info(amx));
				amx_Push(amx, result.store(amx));
			}catch(const errors::native_error &err)
			{
				logprintf("[PawnPlus] Unhandled error in stored parameter: %s", err.message.c_str());
				amx_Push(amx, std::numeric_limits<cell>::min());
			}catch(const errors::amx_error &err)
			{
				logprintf("[PawnPlus] Unhandled AMX error %d in stored parameter: %s", err.code, amx::StrError(err.code));
				amx_Push(amx, std::numeric_limits<cell>::min());
			}
			break;
	}
}

stored_param::stored_param() noexcept : type(param_type::self_id_param)
{

}

stored_param::stored_param(cell val) noexcept : type(param_type::cell_param), cell_value(val)
{

}

stored_param::stored_param(std::basic_string<cell> &&str) noexcept : type(param_type::string_param), string_value(std::move(str))
{

}

stored_param::stored_param(const dyn_object &obj) noexcept : type(param_type::variant_param), variant_value(obj)
{

}

stored_param::stored_param(std::shared_ptr<handle_t> &&handle) noexcept : type(param_type::handle_param), handle_value(std::move(handle))
{

}

stored_param::stored_param(expression_ptr &&expr) noexcept : type(param_type::expression_param), expr_value(std::move(expr))
{

}

stored_param::stored_param(const stored_param &obj) noexcept : type(obj.type)
{
	switch(type)
	{
		case param_type::cell_param:
			cell_value = obj.cell_value;
			break;
		case param_type::string_param:
			new (&string_value) std::basic_string<cell>(obj.string_value);
			break;
		case param_type::variant_param:
			new (&variant_value) dyn_object(obj.variant_value);
			break;
		case param_type::handle_param:
			new (&handle_value) std::shared_ptr<handle_t>(obj.handle_value);
			break;
		case param_type::expression_param:
			new (&expr_value) expression_ptr(obj.expr_value);
			break;
	}
}

stored_param::stored_param(stored_param &&obj) noexcept : type(obj.type)
{
	switch(type)
	{
		case param_type::cell_param:
			cell_value = obj.cell_value;
			break;
		case param_type::string_param:
			new (&string_value) std::basic_string<cell>(std::move(obj.string_value));
			break;
		case param_type::variant_param:
			new (&variant_value) dyn_object(std::move(obj.variant_value));
			break;
		case param_type::handle_param:
			new (&handle_value) std::shared_ptr<handle_t>(std::move(obj.handle_value));
			break;
		case param_type::expression_param:
			new (&expr_value) expression_ptr(std::move(obj.expr_value));
			break;
	}
}

stored_param &stored_param::operator=(const stored_param &obj) noexcept
{
	if(this != &obj)
	{
		this->~stored_param();
		new (this) stored_param(obj);
	}
	return *this;
}

stored_param &stored_param::operator=(stored_param &&obj) noexcept
{
	if(this != &obj)
	{
		this->~stored_param();
		new (this) stored_param(std::move(obj));
	}
	return *this;
}

stored_param::~stored_param()
{
	switch(type)
	{
		case param_type::string_param:
			string_value.~basic_string();
			break;
		case param_type::variant_param:
			variant_value.~dyn_object();
			break;
		case param_type::handle_param:
			handle_value.~shared_ptr<handle_t>();
			break;
		case param_type::expression_param:
			expr_value.~expression_ptr();
			break;
	}
}
