#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>
#include <stdio.h>
#include <stdarg.h>

[[noreturn]] void amx_FormalError(const char *format, ...);
[[noreturn]] void amx_LogicError(const char *format, ...);
cell *amx_GetAddrSafe(AMX *amx, cell amx_addr);
void amx_AllotSafe(AMX *amx, int cells, cell *amx_addr, cell **phys_addr);

namespace errors
{
	struct native_error
	{
		std::string message;
		int level;

		native_error(const char *format, va_list args, int level);
		native_error(const char *format, int level, ...);

		native_error(std::string &&message, int level) : level(level), message(std::move(message))
		{

		}
	};

	struct end_of_arguments_error
	{
		const cell *argbase;
		size_t required;

		end_of_arguments_error(const cell *argbase, size_t required) : argbase(argbase), required(required)
		{

		}
	};

	struct amx_error
	{
		int code;

		amx_error(int code) : code(code)
		{

		}
	};

	typedef const char *const error;

	extern error not_enough_args;
	extern error pointer_invalid;
	extern error cannot_acquire;
	extern error cannot_release;
	extern error operation_not_supported;
	extern error out_of_range;
	extern error element_not_present;
	extern error func_not_found;
	extern error var_not_found;
	extern error arg_empty;
	extern error inner_error;
	extern error no_debug_error;
	extern error unhandled_exception;
	extern error invalid_format;
	extern error invalid_expression;
	extern error inner_error_msg;
	extern error unhandled_system_exception;
}

namespace amx
{
	const char *StrError(int error);
}

#endif
