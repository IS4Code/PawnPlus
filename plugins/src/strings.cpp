#include "strings.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

using namespace strings;

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

object_pool<cell_string> strings::pool;

cell strings::null_value1[1] = {0};
cell strings::null_value2[2] = {0, 1};

cell_string *strings::create(const cell *addr, bool temp, bool truncate, bool fixnulls)
{
	if(addr == nullptr || !addr[0])
	{
		return pool.add(temp);
	}else if(addr[0] & 0xFF000000)
	{
		const char *c;
		size_t pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const char*>(addr) + pos / 4 + ipos;
		}while(*c);

		cell_string *str = pool.add(cell_string(pos, 0), temp);

		pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const char*>(addr) + pos / 4 + ipos;
			(*str)[pos] = *c;
		}while(*c);

		return str;
	}else{
		size_t pos = -1;
		const cell *c;
		do{
			pos++;
			c = addr + pos;
		}while (*c);

		cell_string *str = pool.add(cell_string(addr, pos), temp);
		if(truncate)
		{
			for(size_t i = 0; i < pos; i++)
			{
				cell &c = (*str)[i];
				c &= 0xFF;
				if(fixnulls && c == 0) c = 0x00FFFF00;
			}
		}
		return str;
	}
}

cell_string *strings::create(const cell *addr, bool temp, size_t length, bool truncate, bool fixnulls)
{
	cell_string *str = pool.add(cell_string(addr, length), temp);
	if(truncate)
	{
		for(size_t i = 0; i < length; i++)
		{
			cell &c = (*str)[i];
			c &= 0xFF;
			if(fixnulls && c == 0) c = 0x00FFFF00;
		}
	}else if(fixnulls)
	{
		for(size_t i = 0; i < length; i++)
		{
			cell &c = (*str)[i];
			if(c == 0) c = 0x00FFFF00;
		}
	}
	return str;
}

cell_string *strings::create(const std::string &str, bool temp)
{
	size_t len = str.size();
	cell_string *cstr = pool.add(cell_string(len, '\0'), temp);
	for(size_t i = 0; i < len; i++)
	{
		(*cstr)[i] = static_cast<cell>(str[i]);
	}
	return cstr;
}

bool strings::clamp_range(const cell_string &str, cell &start, cell &end)
{
	clamp_pos(str, start);
	clamp_pos(str, end);

	return start <= end;
}

bool strings::clamp_pos(const cell_string &str, cell &pos)
{
	size_t size = str.size();
	if(pos < 0) pos += size;
	if(static_cast<size_t>(pos) >= size)
	{
		pos = size;
		return false;
	}
	return true;
}
