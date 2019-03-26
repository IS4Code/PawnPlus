#include "stored_param.h"
#include "errors.h"

stored_param stored_param::create(AMX *amx, char format, const cell *args, size_t &argi, size_t numargs)
{
	cell *addr, *addr2;
	switch(format)
	{
		case 'a':
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 2);
			addr = amx_GetAddrSafe(amx, args[argi]);
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr2 = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(std::basic_string<cell>(addr, *addr2));
		case 's':
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(std::basic_string<cell>(addr));
		case 'e':
			return stored_param();
		default:
			if(++argi >= numargs) throw errors::end_of_arguments_error(args, numargs + 1);
			addr = amx_GetAddrSafe(amx, args[argi]);
			return stored_param(*addr);
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
			amx_PushArray(amx, &addr, &phys_addr, string_value.c_str(), string_value.size() + 1);
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

stored_param::stored_param(const stored_param &obj) noexcept : type(obj.type)
{
	if(type == param_type::string_param)
	{
		new (&string_value) std::basic_string<cell>(obj.string_value);
	}else{
		cell_value = obj.cell_value;
	}
}

stored_param::stored_param(stored_param &&obj) noexcept : type(obj.type)
{
	if(type == param_type::string_param)
	{
		new (&string_value) std::basic_string<cell>(std::move(obj.string_value));
	}else{
		cell_value = obj.cell_value;
	}
}

stored_param &stored_param::operator=(const stored_param &obj) noexcept
{
	if(this != &obj)
	{
		if(type == param_type::string_param)
		{
			if(obj.type == type)
			{
				string_value = obj.string_value;
			}else{
				string_value.~basic_string();
				cell_value = obj.cell_value;
			}
		}else if(obj.type == param_type::string_param)
		{
			new (&string_value) std::basic_string<cell>(obj.string_value);
		}else{
			cell_value = obj.cell_value;
		}

		type = obj.type;
	}
	return *this;
}

stored_param &stored_param::operator=(stored_param &&obj) noexcept
{
	if(this != &obj)
	{
		if(type == param_type::string_param)
		{
			if(obj.type == type)
			{
				string_value = std::move(obj.string_value);
			}else{
				string_value.~basic_string();
				cell_value = obj.cell_value;
			}
		}else if(obj.type == param_type::string_param)
		{
			new (&string_value) std::basic_string<cell>(std::move(obj.string_value));
		}else{
			cell_value = obj.cell_value;
		}

		type = obj.type;
	}
	return *this;
}

stored_param::~stored_param()
{
	if(type == param_type::string_param)
	{
		string_value.~basic_string();
	}
}
