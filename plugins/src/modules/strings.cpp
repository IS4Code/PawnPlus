#include "strings.h"

#include <stddef.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <unordered_map>
#include <iterator>
#include <limits>

using namespace strings;

object_pool<cell_string> strings::pool;

cell strings::null_value1[1] = {0};
cell strings::null_value2[2] = {0, 1};

cell strings::create(const cell *addr, bool truncate, bool fixnulls)
{
	auto &ptr = pool.emplace(convert(addr));
	if(truncate || fixnulls)
	{
		for(auto &c : *ptr)
		{
			if(truncate)
			{
				c &= 0xFF;
			}
			if(fixnulls && c == 0)
			{
				c = 0x00FFFF00;
			}
		}
	}
	return pool.get_id(ptr);
}

cell strings::create(const cell *addr, size_t length, bool packed, bool truncate, bool fixnulls)
{
	auto &ptr = pool.emplace(convert(addr, length, packed));
	if(truncate || fixnulls)
	{
		for(auto &c : *ptr)
		{
			if(truncate)
			{
				c &= 0xFF;
			}
			if(fixnulls && c == 0)
			{
				c = 0x00FFFF00;
			}
		}
	}
	return pool.get_id(ptr);
}

cell_string strings::convert(const cell *str)
{
	if(!str)
	{
		return {};
	}
	int len;
	amx_StrLen(str, &len);
	return convert(str, len, static_cast<ucell>(*str) > UNPACKEDMAX);
}

cell_string strings::convert(const cell *str, size_t length, bool packed)
{
	if(!str)
	{
		return {};
	}
	if(!packed)
	{
		return cell_string(str, str + length);
	}else if(reinterpret_cast<std::intptr_t>(str) % sizeof(cell) == 0)
	{
		aligned_const_char_iterator it(str);
		return cell_string(it, it + length);
	}else{
		unaligned_const_char_iterator it(str);
		return cell_string(it, it + length);
	}
}

cell_string strings::convert(const std::string &str)
{
	size_t len = str.size();
	cell_string cstr(len, '\0');
	for(size_t i = 0; i < len; i++)
	{
		cstr[i] = static_cast<unsigned char>(str[i]);
	}
	return cstr;
}

cell strings::create(const std::string &str)
{
	return pool.get_id(pool.emplace(convert(str)));
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

std::locale custom_locale;
std::string custom_locale_name;

std::locale::category get_category(cell category)
{
	if(category == -1) return std::locale::all;
	std::locale::category cat = std::locale::none;
	if(category & 1)
	{
		cat |= std::locale::collate;
	}
	if(category & 2)
	{
		cat |= std::locale::ctype;
	}
	if(category & 4)
	{
		cat |= std::locale::monetary;
	}
	if(category & 8)
	{
		cat |= std::locale::numeric;
	}
	if(category & 16)
	{
		cat |= std::locale::time;
	}
	if(category & 32)
	{
		cat |= std::locale::messages;
	}
	return cat;
}

char *split_locale(char *str)
{
	auto pos = std::strstr(str, "|");
	if(pos)
	{
		*pos = '\0';
		++pos;
	}
	return pos;
}

std::locale strings::find_locale(char *spec)
{
	if(!spec)
	{
		return std::locale("");
	}
	char *nextspec;
	while(nextspec = split_locale(spec))
	{
		try{
			return std::locale(spec);
		}catch(const std::runtime_error &)
		{

		}
		spec = nextspec;
	}
	return std::locale(spec);
}

static void set_global_locale(std::locale loc, bool is_wide)
{
	if(is_wide)
	{
		std::ctype<cell>::install<wchar_t>(loc);
	}else{
		std::ctype<cell>::install<char>(loc);
	}
	std::locale::global(custom_locale = loc);
}

void strings::set_locale(const std::locale &loc, cell category, bool is_wide)
{
	auto cat = get_category(category);
	if(cat == std::locale::all)
	{
		set_global_locale(loc, is_wide);
	}else{
		set_global_locale(std::locale(custom_locale, loc, cat), is_wide);
	}
	custom_locale_name = loc.name();
}

void strings::reset_locale()
{
	std::locale::global(std::locale::classic());
}

const std::string &strings::locale_name()
{
	return custom_locale_name;
}

encoding strings::find_encoding(char *spec)
{
	auto locale = spec ? find_locale(spec) : std::locale();
	return {locale};
}

void strings::to_lower(cell_string &str, const encoding &enc)
{
	enc.ctype().tolower(&str[0], &str[str.size()]);
}

void strings::to_upper(cell_string &str, const encoding &enc)
{
	enc.ctype().toupper(&str[0], &str[str.size()]);
}
