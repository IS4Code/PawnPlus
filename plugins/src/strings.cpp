#include "strings.h"

#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <bitset>

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

cell_string convert(const std::string &str)
{
	size_t len = str.size();
	cell_string cstr(len, '\0');
	for(size_t i = 0; i < len; i++)
	{
		cstr[i] = static_cast<cell>(str[i]);
	}
	return cstr;
}

cell_string *strings::create(const std::string &str, bool temp)
{
	return pool.add(convert(str), temp);
}

void add_format(std::basic_ostringstream<cell> &oss, std::basic_stringbuf<cell> *buf, const cell *begin, const cell *end, cell *arg)
{
	ptrdiff_t flen = end - begin;
	switch(*end)
	{
		case 's':
		{
			int len;
			amx_StrLen(arg, &len);
			buf->sputn(arg, len);
		}
		break;
		case 'S':
		{
			auto str = reinterpret_cast<cell_string*>(*arg);
			buf->sputn(&(*str)[0], str->size());
		}
		break;
		case 'd':
		case 'i':
		{
			oss << convert(std::to_string(*arg));
		}
		break;
		case 'f':
		{
			oss << convert(std::to_string(amx_ctof(*arg)));
		}
		break;
		case 'c':
		{
			buf->sputc(*arg);
		}
		break;
		case 'x':
		{
			//thanks MSVC!!!
			std::ostringstream s;
			s << std::hex << std::uppercase << *arg;
			oss << convert(s.str());
		}
		break;
		case 'b':
		{
			std::bitset<8> bits(*arg);
			oss << bits;
		}
		break;
		case 'u':
		{
			oss << convert(std::to_string(static_cast<ucell>(*arg)));
		}
		break;
	}
}

cell_string *strings::format(AMX *amx, const cell *format, int flen, int argc, cell *args, bool temp)
{
	std::basic_ostringstream<cell> oss;

	std::basic_stringbuf<cell> *buf = oss.rdbuf();

	int argn = 0;

	const cell *c = format;
	while(flen--)
	{
		if(*c == '%' && flen > 0)
		{
			buf->sputn(format, c - format);

			const cell *start = ++c;
			if(*c == '%')
			{
				buf->sputc('%');
				format = c + 1;
				flen--;
			}else{
				while(flen-- && !std::isalpha(*c)) c++;
				if(flen < 0) break;
				if(argn >= argc)
				{
					//error
				}else{
					cell *argv;
					amx_GetAddr(amx, args[argn++], &argv);
					add_format(oss, buf, start, c, argv);
				}
				format = c + 1;
			}
		}
		c++;
	}
	buf->sputn(format, c - format);

	return pool.add(oss.str(), temp);
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
