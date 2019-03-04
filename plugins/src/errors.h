#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>
#include <stdio.h>
#include <stdarg.h>

[[noreturn]] void amx_FormalError(const char *format, ...);
[[noreturn]] void amx_LogicError(const char *format, ...);

namespace errors
{
	struct native_error
	{
		std::string message;
		int level;

		native_error(const char *format, va_list args, int level);
		native_error(const char *format, int level, ...);
	};

	struct end_of_arguments_error
	{
		const cell *argbase;
		size_t required;

		end_of_arguments_error(const cell *argbase, size_t required);
	};

	typedef const char *const error;

	extern error not_enough_args;
	extern error pointer_invalid;
	extern error cannot_acquire;
	extern error cannot_release;
	extern error operation_not_supported;
	extern error out_of_range;
	extern error key_not_present;
	extern error func_not_found;
	extern error arg_empty;
	extern error inner_error;
	extern error no_debug_error;
	extern error unhandled_exception;
}

namespace amx
{
	const char *StrError(int error);
}

#endif
