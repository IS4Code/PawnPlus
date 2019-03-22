#include "strings.h"
#include "errors.h"

#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <bitset>
#include <iomanip>

using namespace strings;

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

object_pool<cell_string> strings::pool;

cell strings::null_value1[1] = {0};
cell strings::null_value2[2] = {0, 1};

cell strings::create(const cell *addr, bool truncate, bool fixnulls)
{
	if(addr == nullptr || !addr[0])
	{
		return pool.get_id(pool.add());
	}else if(addr[0] & 0xFF000000)
	{
		const unsigned char *c;
		size_t pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const unsigned char*>(addr) + pos / 4 + ipos;
		}while(*c);

		auto &str = pool.emplace(pos, 0);

		pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const unsigned char*>(addr) + pos / 4 + ipos;
			(*str)[pos] = *c;
		}while(*c);

		return pool.get_id(str);
	}else{
		size_t pos = -1;
		const cell *c;
		do{
			pos++;
			c = addr + pos;
		}while (*c);

		auto &str = pool.emplace(addr, pos);
		if(truncate)
		{
			for(size_t i = 0; i < pos; i++)
			{
				cell &c = (*str)[i];
				c &= 0xFF;
				if(fixnulls && c == 0) c = 0x00FFFF00;
			}
		}
		return pool.get_id(str);
	}
}

cell strings::create(const cell *addr, size_t length, bool packed, bool truncate, bool fixnulls)
{
	if(addr == nullptr || length == 0)
	{
		return pool.get_id(pool.add());
	}
	if(packed && (addr[0] & 0xFF000000))
	{
		cell last = addr[length - 1];
		size_t rem = 0;
		if(last & 0xFF) rem = 4;
		else if(last & 0xFF00) rem = 3;
		else if(last & 0xFF0000) rem = 2;
		else if(last & 0xFF000000) rem = 1;

		length = (length - 1) * 4 + rem;

		auto &str = pool.emplace(length, '\0');
		size_t pos = 0;
		do{
			size_t ipos = pos / 4 * 4 + 3 - pos % 4;
			cell c = reinterpret_cast<const unsigned char*>(addr)[ipos];
			if(fixnulls)
			{
				if(c == 0) c = 0x00FFFF00;
			}
			(*str)[pos] = c;
			pos++;
		}while(pos < length);
		return pool.get_id(str);
	}else{
		auto &str = pool.emplace(addr, length);
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
		return pool.get_id(str);
	}
}

cell_string strings::convert(const std::string &str)
{
	size_t len = str.size();
	cell_string cstr(len, '\0');
	for(size_t i = 0; i < len; i++)
	{
		cstr[i] = static_cast<cell>(str[i]);
	}
	return cstr;
}

cell strings::create(const std::string &str)
{
	return pool.get_id(pool.emplace(convert(str)));
}

void add_query(strings::cell_string &buf, const cell *str, int len)
{
	const cell *start = str;
	while(len--)
	{
		if(*str == '\'')
		{
			buf.append(start, str - start + 1);
			buf.append(1, '\'');
			start = str + 1;
		}
		str++;
	}
	buf.append(start, str - start);
}

namespace aux
{
	void push_args(std::ostream &ostream)
	{

	}

	template <class Arg, class... Args>
	void push_args(std::ostream &ostream, Arg &&arg, Args&&... args)
	{
		ostream << std::forward<Arg>(arg);
		push_args(ostream, std::forward<Args>(args)...);
	}

	template <class Obj, class... Args>
	std::string to_string(Obj &&obj, Args&&... args)
	{
		std::ostringstream ostream;
		push_args(ostream, std::forward<Args>(args)...);
		ostream << std::forward<Obj>(obj);
		return ostream.str();
	}

	template <class NumType>
	NumType parse_num(const cell *str, size_t &pos)
	{
		bool neg = str[pos] == '-';
		if(neg) pos++;
		NumType val = 0;
		cell c;
		while(std::isdigit(c = str[pos++]))
		{
			val = (val * 10) + (c - '0');
		}
		return neg ? -val : val;
	}
}

void add_format(strings::cell_string &buf, const cell *begin, const cell *end, cell *arg)
{
	ptrdiff_t flen = end - begin;
	switch(*end)
	{
		case 's':
		{
			int len;
			amx_StrLen(arg, &len);
			buf.append(arg, len);
		}
		break;
		case 'S':
		{
			cell_string *str;
			if(pool.get_by_id(*arg, str))
			{
				buf.append(&(*str)[0], str->size());
			}
		}
		break;
		case 'q':
		{
			int len;
			amx_StrLen(arg, &len);
			add_query(buf, arg, len);
		}
		break;
		case 'Q':
		{
			cell_string *str;
			if(pool.get_by_id(*arg, str))
			{
				add_query(buf, &(*str)[0], str->size());
			}
		}
		break;
		case 'd':
		case 'i':
		{
			buf.append(convert(std::to_string(*arg)));
		}
		break;
		case 'f':
		{
			if(*begin == '.')
			{
				size_t pos = 0;
				auto precision = aux::parse_num<std::streamsize>(begin + 1, pos);
				buf.append(convert(aux::to_string(amx_ctof(*arg), std::setprecision(precision), std::fixed)));
			}else{
				buf.append(convert(std::to_string(amx_ctof(*arg))));
			}
		}
		break;
		case 'c':
		{
			buf.append(1, *arg);
		}
		break;
		case 'h':
		case 'x':
		{
			buf.append(convert(aux::to_string(*arg, std::hex, std::uppercase)));
		}
		break;
		case 'o':
		{
			buf.append(convert(aux::to_string(*arg, std::oct)));
		}
		break;
		case 'b':
		{
			std::bitset<8> bits(*arg);
			buf.append(bits.to_string<cell>());
		}
		break;
		case 'u':
		{
			buf.append(convert(std::to_string(static_cast<ucell>(*arg))));
		}
		break;
		default:
		{
			amx_FormalError("invalid format specifier '%c'", *end);
		}
		break;
	}
}

void strings::format(AMX *amx, strings::cell_string &buf, const cell *format, int flen, int argc, cell *args)
{
	buf.reserve(flen+8*argc);

	int argn = 0;

	const cell *c = format;
	while(flen--)
	{
		if(*c == '%' && flen > 0)
		{
			buf.append(format, c - format);

			const cell *start = ++c;
			if(*c == '%')
			{
				buf.append(1, '%');
				format = c + 1;
				flen--;
			}else{
				while(flen-- && !std::isalpha(*c)) c++;
				if(flen < 0) break;
				if(argn >= argc)
				{
					throw errors::end_of_arguments_error(args, argn + 1);
				}else{
					cell *argv;
					amx_GetAddr(amx, args[argn++], &argv);
					add_format(buf, start, c, argv);
				}
				format = c + 1;
			}
		}
		c++;
	}
	buf.append(format, c - format);
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
