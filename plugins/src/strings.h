#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include "object_pool.h"
#include "sdk/amx/amx.h"
#include <string>

namespace strings
{
	typedef std::basic_string<cell> cell_string;
	extern cell null_value1[1];
	extern cell null_value2[2];
	extern object_pool<cell_string> pool;

	cell_string *create(const cell *addr, bool temp, bool truncate, bool fixnulls);
	cell_string *create(const cell *addr, bool temp, size_t length, bool truncate, bool fixnulls);
	cell_string *create(const std::string &str, bool temp);

	cell_string format(AMX *amx, const cell *format, int flen, int argc, cell *args);

	bool clamp_range(const cell_string &str, cell &start, cell &end);
	bool clamp_pos(const cell_string &str, cell &pos);
}

#endif
