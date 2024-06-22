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

#ifndef _WIN32
#define _stricmp strcasecmp
#endif

decltype(encoding::type) parse_encoding_type(char *&spec)
{
	auto pos = std::strstr(spec, "+");
	if(pos)
	{
		pos[0] = '\0';
	}

	auto found = [&](decltype(encoding::type) type)
	{
		spec = pos ? pos + 1 : "";
		return type;
	};

	if(!_stricmp(spec, "ansi"))
	{
		return found(encoding::ansi);
	}
	if(!_stricmp(spec, "unicode"))
	{
		return found(encoding::unicode);
	}
	if(!_stricmp(spec, "utf"))
	{
		auto utf = spec + 3;
		if(utf[0] == '-')
		{
			++utf;
		}
		if(!_stricmp(utf, "8"))
		{
			return found(encoding::utf8);
		}
		if(!_stricmp(utf, "16"))
		{
			return found(encoding::utf16);
		}
		if(!_stricmp(utf, "32"))
		{
			return found(encoding::utf32);
		}
	}

	if(pos)
	{
		pos[0] = '+';
	}
	return encoding::ansi;
}

encoding strings::find_encoding(char *spec, bool default_if_empty)
{
	auto make_encoding = [&](char *name, decltype(encoding::type) type) -> encoding
	{
		if(name[0] == '\0' && !default_if_empty)
		{
			return {std::locale(), type};
		}
		return {std::locale(name), type};
	};

	if(!spec)
	{
		return make_encoding("", default_if_empty ? encoding::ansi : encoding::unspecified);
	}
	char *nextspec;
	while(nextspec = split_locale(spec))
	{
		auto type = parse_encoding_type(spec);
		try{
			return make_encoding(spec, type);
		}catch(const std::runtime_error &)
		{

		}
		spec = nextspec;
	}
	auto type = parse_encoding_type(spec);
	return make_encoding(spec, type);
}

template <class Facet, class... Args>
void add_facet(std::locale &target, Args&&... args)
{
	if(!std::has_facet<Facet>(target))
	{
		Facet::install(target, std::forward<Args>(args)...);
	}
}

template <class Facet>
void copy_facet(std::locale &target, const std::locale &source)
{
	if(std::has_facet<Facet>(source))
	{
		Facet::install(target, std::use_facet<Facet>(source));
	}
}

static std::locale merge_locale(const std::locale &base, const std::locale &specific, std::locale::category cat)
{
	std::locale result(base, specific, cat);
	copy_facet<std::ctype<char8_t>>(result, specific);
	copy_facet<std::ctype<char16_t>>(result, specific);
	copy_facet<std::ctype<char32_t>>(result, specific);
	copy_facet<std::ctype<cell>>(result, specific);
	return result;
}

std::locale encoding::install() const
{
	std::locale result = locale;
	switch(type)
	{
		case ansi:
			std::ctype<cell>::install<char>(result);
			break;
		case unicode:
			std::ctype<cell>::install<wchar_t>(result);
			break;
		case utf8:
			add_facet<std::ctype<char8_t>>(result);
			std::ctype<cell>::install<char8_t>(result);
			break;
		case utf16:
			if(sizeof(char16_t) == sizeof(wchar_t))
			{
				std::ctype<cell>::install<wchar_t>(result);
			}else{
				add_facet<std::ctype<char16_t>>(result);
				std::ctype<cell>::install<char16_t>(result);
			}
			break;
		case utf32:
			if(sizeof(char32_t) == sizeof(wchar_t))
			{
				std::ctype<cell>::install<wchar_t>(result);
			}else{
				add_facet<std::ctype<char32_t>>(result);
				std::ctype<cell>::install<char32_t>(result);
			}
			break;
	}
	return result;
}

void strings::set_encoding(const encoding &enc, cell category)
{
	auto set_global = [](std::locale loc)
	{
		std::locale::global(custom_locale = std::move(loc));
	};

	auto cat = get_category(category);
	if((cat & std::locale::all) == std::locale::all)
	{
		set_global(enc.install());
	}else if(cat & std::locale::ctype)
	{
		set_global(merge_locale(custom_locale, enc.install(), cat));
	}else{
		set_global(std::locale(custom_locale, enc.locale, cat));
	}
	custom_locale_name = enc.locale.name();
}

void strings::reset_locale()
{
	std::locale::global(std::locale::classic());
}

const std::string &strings::locale_name()
{
	return custom_locale_name;
}

using ctype_transform = const cell*(std::ctype<cell>::*)(cell*, const cell*) const;

template <ctype_transform Transform>
void transform_string(cell_string &str, const encoding &enc)
{
	std::locale locale = enc.install();
	const auto &facet = std::use_facet<std::ctype<cell>>(locale);
	(facet.*Transform)(&str[0], &str[str.size()]);
}

void strings::to_lower(cell_string &str, const encoding &enc)
{
	transform_string<static_cast<ctype_transform>(&std::ctype<cell>::tolower)>(str, enc);
}

void strings::to_upper(cell_string &str, const encoding &enc)
{
	transform_string<static_cast<ctype_transform>(&std::ctype<cell>::toupper)>(str, enc);
}
