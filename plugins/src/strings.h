#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>

namespace Strings
{
	typedef std::basic_string<cell> cell_string;
	extern cell NullValue1[1];
	extern cell NullValue2[2];
	cell_string *Create(const cell *addr, bool temp, bool wide);
	cell_string *Create(const cell *addr, bool temp, size_t length, bool wide);
	cell_string *Add(const std::string &str, bool temp);
	cell_string *Add(cell_string &&str, bool temp);
	bool Free(const cell_string *str);
	cell_string *Get(AMX *amx, cell addr);
	bool IsValid(const cell_string *str);
	void SetCache(const cell_string *str);
	const cell_string *FindCache(const cell *str);
	cell GetAddress(AMX *amx, const cell_string *str);
	bool IsNullAddress(AMX *amx, cell addr);
	bool MoveToGlobal(const cell_string *str);
	bool MoveToLocal(const cell_string *str);
	void ClearCacheAndTemporary();

	bool ClampRange(const cell_string &str, cell &start, cell &end);
	bool ClampPos(const cell_string &str, cell &pos);
}

#endif
