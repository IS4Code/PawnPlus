#include "strings.h"
#include "errors.h"
#include "modules/containers.h"

#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <bitset>
#include <iomanip>
#include <regex>
#include <unordered_map>
#include <iterator>
#include <limits>
#include <locale>

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

cell_string strings::convert(const cell *str)
{
	int len;
	amx_StrLen(str, &len);
	if(static_cast<ucell>(*str) <= UNPACKEDMAX)
	{
		return cell_string(str, str + len);
	}else{
		const_char_iterator it(str);
		return cell_string(it, it + len);
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


template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

const std::ctype<char> *const base_facet(&std::use_facet<std::ctype<char>>(std::locale::classic()));

namespace std
{
	template <class Type1, class Type2>
	struct hash<std::pair<Type1, Type2>>
	{
		size_t operator()(const std::pair<Type1, Type2> &obj) const
		{
			return std::hash<Type1>()(obj.first) ^ std::hash<Type2>()(obj.second);
		}
	};

	template <class Type>
	struct hash<std::basic_string<Type>>
	{
		size_t operator()(const std::basic_string<Type> &obj) const
		{
			size_t seed = 0;
			for(const auto &c : obj)
			{
				hash_combine(seed, c);
			}
			return seed;
		}
	};

	template <>
	struct hash<std::regex_constants::error_type>
	{
		size_t operator()(const std::regex_constants::error_type &obj) const
		{
			return std::hash<size_t>()(obj);
		}
	};

	template <>
	class ctype<cell> : public locale::facet
	{
	public:
		typedef cell char_type;

		enum
		{
			alnum = std::ctype_base::alnum,
			alpha = std::ctype_base::alpha,
			cntrl = std::ctype_base::cntrl,
			digit = std::ctype_base::digit,
			graph = std::ctype_base::graph,
			lower = std::ctype_base::lower,
			print = std::ctype_base::print,
			punct = std::ctype_base::punct,
			space = std::ctype_base::space,
			upper = std::ctype_base::upper,
			xdigit = std::ctype_base::xdigit
		};
		typedef std::ctype_base::mask mask;

		static locale::id id;

		bool is(mask mask, cell ch) const
		{
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return false;
			}
			return base_facet->is(mask, static_cast<unsigned char>(ch));
		}

		char narrow(cell ch, char dflt = '\0') const
		{
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return dflt;
			}
			return static_cast<unsigned char>(ch);
		}

		const char *narrow(const cell *first, const cell *last, char dflt, char *dest) const
		{
			return std::transform(first, last, dest, [&](cell c){return narrow(c, dflt);});
		}

		cell widen(char c) const
		{
			return static_cast<unsigned char>(c);
		}

		const cell *widen(const char *first, const char *last, cell *dest) const
		{
			return std::transform(first, last, dest, [&](char c){return widen(c);});
		}

		cell tolower(cell ch) const
		{
			return to_lower(ch);
		}

		const cell *tolower(cell *first, const cell *last) const
		{
			return std::transform(first, const_cast<cell*>(last), first, to_lower);
		}

		cell toupper(cell ch) const
		{
			return to_upper(ch);
		}

		const cell *toupper(cell *first, const cell *last) const
		{
			return std::transform(first, const_cast<cell*>(last), first, to_upper);
		}
	};
}

std::locale::id std::ctype<cell>::id;

const std::ctype<cell> cell_ctype;

const std::locale custom_locale(std::locale::classic(), &cell_ctype);

struct regex_traits
{
	typedef cell char_type;
	typedef cell_string string_type;
	typedef size_t size_type;
	typedef std::locale locale_type;
	typedef std::ctype_base::mask char_class_type;

	typedef ucell _Uelem;
	enum _Char_class_type
	{
		_Ch_none = 0,
		_Ch_alnum = std::ctype_base::alnum,
		_Ch_alpha = std::ctype_base::alpha,
		_Ch_cntrl = std::ctype_base::cntrl,
		_Ch_digit = std::ctype_base::digit,
		_Ch_graph = std::ctype_base::graph,
		_Ch_lower = std::ctype_base::lower,
		_Ch_print = std::ctype_base::print,
		_Ch_punct = std::ctype_base::punct,
		_Ch_space = std::ctype_base::space,
		_Ch_upper = std::ctype_base::upper,
		_Ch_xdigit = std::ctype_base::xdigit
	};

	regex_traits()
	{
		cache_locale();
	}

	int value(cell ch, int base) const
	{
		if((base != 8 && '0' <= ch && ch <= '9') || (base == 8 && '0' <= ch && ch <= '7'))
		{
			return (ch - '0');
		}

		if(base != 16)
		{
			return (-1);
		}

		if('a' <= ch && ch <= 'f')
		{
			return (ch - 'a' + 10);
		}

		if('A' <= ch && ch <= 'F')
		{
			return (ch - 'A' + 10);
		}

		return -1;
	}

	static size_type length(const cell *str)
	{
		return std::char_traits<cell>::length(str);
	}

	cell translate(cell ch) const
	{
		return ch;
	}

	cell translate_nocase(cell ch) const
	{
		return to_lower(ch);
	}

	template<class Iterator>
	string_type transform(Iterator first, Iterator last) const
	{
		return string_type(first, last);
	}

	template<class Iterator>
	string_type transform_primary(Iterator first, Iterator last) const
	{
		string_type res(first, last);
		std::transform(res.begin(), res.end(), res.begin(), to_lower);
		return res;
	}

	bool isctype(cell ch, std::ctype_base::mask ctype) const
	{
		if(ctype != static_cast<std::ctype_base::mask>(-1))
		{
			return _ctype->is(ctype, ch);
		}else{
			return ch == '_' || _ctype->is(std::ctype_base::alnum, ch);
		}
	}

	template<class Iterator>
	std::ctype_base::mask lookup_classname(Iterator first, Iterator last, bool icase = false) const
	{
		static std::unordered_map<cell_string, std::ctype_base::mask> map{
			{convert("alnum"), std::ctype_base::alnum},
			{convert("alpha"), std::ctype_base::alpha},
			{convert("cntrl"), std::ctype_base::cntrl},
			{convert("digit"), std::ctype_base::digit},
			{convert("graph"), std::ctype_base::graph},
			{convert("lower"), std::ctype_base::lower},
			{convert("print"), std::ctype_base::print},
			{convert("punct"), std::ctype_base::punct},
			{convert("space"), std::ctype_base::space},
			{convert("upper"), std::ctype_base::upper},
			{convert("xdigit"), std::ctype_base::xdigit}
		};

		cell_string key(first, last);
		auto it = map.find(key);
		if(it != map.end())
		{
			auto mask = it->second;
			if(icase && (mask & (std::ctype_base::lower | std::ctype_base::upper)))
			{
				mask |= std::ctype_base::lower | std::ctype_base::upper;
			}
			return mask;
		}
		return {};
	}

	template<class Iterator>
	string_type lookup_collatename(Iterator first, Iterator last) const
	{
		return string_type(first, last);
	}

	locale_type imbue(locale_type loc)
	{
		locale_type tmp = _locale;
		_locale = loc;
		cache_locale();
		return tmp;
	}

	locale_type getloc() const
	{
		return _locale;
	}

private:
	void cache_locale()
	{
		_ctype = &std::use_facet<std::ctype<cell>>(_locale);
	}

	const std::ctype<cell> *_ctype;
	locale_type _locale;
};


template<class BidirIt, class CharT = typename std::iterator_traits<BidirIt>::value_type, class Traits = std::regex_traits<CharT>>
static std::regex_iterator<BidirIt, CharT, Traits> make_regex_iterator(BidirIt a, BidirIt b, const std::basic_regex<CharT, Traits>& re, std::regex_constants::match_flag_type m = std::regex_constants::match_default)
{
	return {a, b, re, m};
}

constexpr const cell cache_flag = 4194304;

static void regex_options(cell options, std::regex_constants::syntax_option_type &syntax, std::regex_constants::match_flag_type &match)
{
	switch(options & 7)
	{
		default:
			syntax = std::regex_constants::ECMAScript;
			break;
		case 1:
			syntax = std::regex_constants::basic;
			break;
		case 2:
			syntax = std::regex_constants::extended;
			break;
		case 3:
			syntax = std::regex_constants::awk;
			break;
		case 4:
			syntax = std::regex_constants::grep;
			break;
		case 5:
			syntax = std::regex_constants::egrep;
			break;
	}
	options &= ~7;
	if(options & 8)
	{
		syntax |= std::regex_constants::icase;
	}
	if(options & 16)
	{
		syntax |= std::regex_constants::nosubs;
	}
	if(options & 32)
	{
		syntax |= std::regex_constants::optimize;
	}
	if(options & 64)
	{
		syntax |= std::regex_constants::collate;
	}

	match = {};
	if(options & 256)
	{
		match |= std::regex_constants::match_not_bol;
	}
	if(options & 512)
	{
		match |= std::regex_constants::match_not_eol;
	}
	if(options & 1024)
	{
		match |= std::regex_constants::match_not_bow;
	}
	if(options & 2048)
	{
		match |= std::regex_constants::match_not_eow;
	}
	if(options & 4096)
	{
		match |= std::regex_constants::match_any;
	}
	if(options & 8192)
	{
		match |= std::regex_constants::match_not_null;
	}
	if(options & 16384)
	{
		match |= std::regex_constants::match_continuous;
	}
	if(options & 65536)
	{
		match |= std::regex_constants::format_no_copy;
	}
	if(options & 131072)
	{
		match |= std::regex_constants::format_first_only;
	}

	std::locale::global(custom_locale);
}

static const char *get_error(std::regex_constants::error_type code)
{
	static std::unordered_map<std::regex_constants::error_type, const char *> map{
		{std::regex_constants::error_collate, "collate"},
		{std::regex_constants::error_ctype, "ctype"},
		{std::regex_constants::error_escape, "escape"},
		{std::regex_constants::error_backref, "backref"},
		{std::regex_constants::error_brack, "brack"},
		{std::regex_constants::error_paren, "paren"},
		{std::regex_constants::error_brace, "brace"},
		{std::regex_constants::error_badbrace, "badbrace"},
		{std::regex_constants::error_range, "range"},
		{std::regex_constants::error_space, "space"},
		{std::regex_constants::error_badrepeat, "badrepeat"},
		{std::regex_constants::error_complexity, "complexity"},
		{std::regex_constants::error_stack, "stack"}
	};
	auto it = map.find(code);
	if(it != map.end())
	{
		return it->second;
	}
	return "unknown";
}

static std::unordered_map<std::pair<cell_string, cell>, std::basic_regex<cell, regex_traits>> regex_cache;

static const std::basic_regex<cell, regex_traits> &get_cached(const cell *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(pattern, options), std::forward_as_tuple(pattern, syntax_options)).first->second;
}

static const std::basic_regex<cell, regex_traits> &get_cached(const cell_string &pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(pattern, options), std::forward_as_tuple(pattern, syntax_options)).first->second;
}

bool strings::regex_search(const cell_string &str, const cell *pattern, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);
		
	try{
		return std::regex_search(str, options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), match_options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);

	try{
		return std::regex_search(str, options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), match_options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);

	try{
		std::match_results<cell_string::const_iterator> match;
		if(std::regex_search(str, match, options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), match_options))
		{
			tag_ptr chartag = tags::find_tag(tags::tag_char);
			auto list = list_pool.add();
			for(auto &group : match)
			{
				list->push_back(dyn_object(&*group.first, group.length(), chartag));
			}
			return list_pool.get_id(list);
		}
		return 0;
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);
		
	try{
		std::match_results<cell_string::const_iterator> match;
		if(std::regex_search(str, match, options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), match_options))
		{
			tag_ptr chartag = tags::find_tag(tags::tag_char);
			auto list = list_pool.add();
			for(auto &group : match)
			{
				list->push_back(dyn_object(&*group.first, group.length(), chartag));
			}
			return list_pool.get_id(list);
		}
		return 0;
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);

	try{
		std::regex_replace(std::back_inserter(target), str.cbegin(), str.cend(), options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), {replacement}, match_options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

cell_string strings::regex_replace(const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell options)
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(options, syntax_options, match_options);

	try{
		return std::regex_replace(str, options & cache_flag ? get_cached(pattern, options, syntax_options) : std::basic_regex<cell, regex_traits>(pattern, syntax_options), replacement, match_options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}
