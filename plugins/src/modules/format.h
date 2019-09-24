#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED

#include "modules/strings.h"
#include "modules/containers.h"

#include <stack>

namespace strings
{
	extern std::stack<std::weak_ptr<map_t>> format_env;

	void format(AMX *amx, strings::cell_string &str, const cell_string &format, cell argc, cell *args);
	void format(AMX *amx, strings::cell_string &str, const cell *format, cell argc, cell *args);
}

#endif
