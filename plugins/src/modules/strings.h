#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include "objects/object_pool.h"
#include "sdk/amx/amx.h"
#include <string>

namespace strings
{
	typedef std::basic_string<cell> cell_string;
	extern cell null_value1[1];
	extern cell null_value2[2];
	extern object_pool<cell_string> pool;

	cell create(const cell *addr, bool truncate, bool fixnulls);
	cell create(const cell *addr, size_t length, bool packed, bool truncate, bool fixnulls);
	cell create(const std::string &str);

	void format(AMX *amx, strings::cell_string &str, const cell *format, int flen, int argc, cell *args);

	cell_string convert(const std::string &str);
	bool clamp_range(const cell_string &str, cell &start, cell &end);
	bool clamp_pos(const cell_string &str, cell &pos);

	inline cell to_lower(cell c)
	{
		if(65 <= c && c <= 90) return c + 32;
		return c;
	}

	inline cell to_upper(cell c)
	{
		if(97 <= c && c <= 122) return c - 32;
		return c;
	}

	bool regex_search(const cell_string &str, const cell *pattern, cell options);
	bool regex_search(const cell_string &str, const cell_string &pattern, cell options);
	cell regex_extract(const cell_string &str, const cell *pattern, cell options);
	cell regex_extract(const cell_string &str, const cell_string &pattern, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell options);
	cell_string regex_replace(const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell options);
}

#endif
