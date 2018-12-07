#include "errors.h"

namespace errors
{
	error not_enough_args = "not enough arguments (%d expected, got %d)";
	error pointer_invalid = "%s reference is invalid (value 0x%x)";
	error cannot_acquire = "%s reference cannot be acquired (null or invalid)";
	error cannot_release = "%s reference cannot be released (null, invalid, or local)";
	error operation_not_supported = "this operation is not supported for the current state of the %s instance";
	error out_of_range = "%s is out of range";
	error key_not_present = "the key is not present in the map";
	error func_not_found = "%s function '%s' was not found";
	error arg_empty = "argument is empty";
}

errors::native_error::native_error(const char *format, va_list args, int code) : message(vsnprintf(NULL, 0, format, args), '\0'), code(code)
{
	vsprintf(&message[0], format, args);
}

errors::native_error::native_error(const char *format, int code, ...) : code(code)
{
	va_list args;
	va_start(args, code);
	message = std::string(vsnprintf(NULL, 0, format, args), '\0');
	vsprintf(&message[0], format, args);
	va_end(args);
}

errors::end_of_arguments_error::end_of_arguments_error(const cell *argbase, size_t required) : argbase(argbase), required(required)
{

}

[[noreturn]] void amx_FormalError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	errors::native_error err(format, args, AMX_ERR_NATIVE);
	va_end(args);
	throw err;
}

[[noreturn]] void amx_LogicError(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	errors::native_error err(format, args, AMX_ERR_NATIVE);
	va_end(args);
	throw err;
}
