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

	template <class NumType, class Iter>
	NumType parse_num(Iter &begin, Iter end)
	{
		bool neg;
		if(std::numeric_limits<NumType>::is_signed)
		{
			neg = *begin == '-';
			if(neg) ++begin;
		}else{
			neg = false;
		}
		NumType val = 0;
		cell c;
		while(std::isdigit(c = *begin))
		{
			val = (val * 10) + (c - '0');
			++begin;
		}
		return neg ? -val : val;
	}
}

template <class Iter>
struct format_base
{
	template <class QueryIter>
	struct add_query_base
	{
		bool operator()(QueryIter begin, QueryIter end, Iter escape_begin, Iter escape_end, cell_string &buf) const
		{
			bool custom = escape_begin != escape_end;
			cell escape_char;
			if(custom)
			{
				escape_char = *escape_begin;
				++escape_begin;
				custom = escape_begin != escape_end;
			}else{
				escape_char = '\\';
			}
			auto last = begin;
			while(begin != end)
			{
				if((!custom && (*begin == '\'' || *begin == '\\')) || std::find(escape_begin, escape_end, *begin) != escape_end)
				{
					buf.append(last, begin);
					buf.append(1, escape_char);
					buf.append(1, *begin);
					++begin;
					last = begin;
				}else{
					++begin;
				}
			}
			buf.append(last, end);
			return true;
		}
	};

	template <class StringIter>
	struct append_base
	{
		bool operator()(StringIter begin, StringIter end, Iter param_begin, Iter param_end, cell_string &buf) const
		{
			if(param_begin == param_end)
			{
				buf.append(begin, end);
				return true;
			}else{
				if(*param_begin == '.')
				{
					++param_begin;
					auto max = aux::parse_num<typename std::iterator_traits<Iter>::difference_type>(param_begin, param_end);
					if(param_begin != param_end || max < 0)
					{
						return false;
					}
					auto new_end = begin + max;
					if(new_end > end)
					{
						new_end = end;
					}
					buf.append(begin, new_end);
					return true;
				}else{
					cell padding = *param_begin;
					++param_begin;
					auto width = aux::parse_num<typename std::iterator_traits<Iter>::difference_type>(param_begin, param_end);
					if(width < 0)
					{
						return false;
					}
					if(param_begin == param_end)
					{
						auto size = end - begin;
						if(width > size)
						{
							buf.append(width - size, padding);
						}
						buf.append(begin, end);
						return true;
					}else if(*param_begin == '.')
					{
						++param_begin;
						auto max = aux::parse_num<typename std::iterator_traits<Iter>::difference_type>(param_begin, param_end);
						if(param_begin != param_end || max < 0)
						{
							return false;
						}
						auto new_end = begin + max;
						if(new_end <= end)
						{
							buf.append(begin, new_end);
						}else{
							auto size = end - begin;
							if(size < width)
							{
								if(width > max)
								{
									width = max;
								}
								buf.append(width - size, padding);
							}
							buf.append(begin, end);
						}
						return true;
					}
					return false;
				}
			}
		}
	};

	void add_format(cell_string &buf, Iter begin, Iter end, const cell *arg) const
	{
		ptrdiff_t flen = end - begin;
		switch(*end)
		{
			case 's':
			{
				if(select_iterator<append_base>(arg, begin, end, buf))
				{
					return;
				}
			}
			break;
			case 'S':
			{
				cell_string *str;
				if(pool.get_by_id(*arg, str) || str == nullptr)
				{
					if(select_iterator<append_base>(str, begin, end, buf))
					{
						return;
					}
				}else{
					amx_LogicError(errors::pointer_invalid, "string", *arg);
					return;
				}
			}
			break;
			case 'q':
			case 'e':
			{
				if(select_iterator<add_query_base>(arg, begin, end, buf))
				{
					return;
				}
			}
			break;
			case 'Q':
			case 'E':
			{
				cell_string *str;
				if(pool.get_by_id(*arg, str) || str == nullptr)
				{
					if(select_iterator<add_query_base>(str, begin, end, buf))
					{
						return;
					}
				}else{
					amx_LogicError(errors::pointer_invalid, "string", *arg);
					return;
				}
			}
			break;
			case 'd':
			case 'i':
			{
				if(begin != end)
				{
					char padding = static_cast<ucell>(*begin);
					++begin;
					auto width = aux::parse_num<std::streamsize>(begin, end);
					if(begin == end && width >= 0)
					{
						buf.append(convert(aux::to_string(*arg, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(std::to_string(*arg)));
					return;
				}
			}
			break;
			case 'u':
			{
				ucell val = static_cast<ucell>(*arg);
				if(begin != end)
				{
					char padding = static_cast<ucell>(*begin);
					++begin;
					auto width = aux::parse_num<std::streamsize>(begin, end);
					if(begin == end && width >= 0)
					{
						buf.append(convert(aux::to_string(val, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(std::to_string(val)));
					return;
				}
			}
			break;
			case 'h':
			case 'x':
			{
				if(begin != end)
				{
					char padding = static_cast<ucell>(*begin);
					++begin;
					auto width = aux::parse_num<std::streamsize>(begin, end);
					if(begin == end && width >= 0)
					{
						buf.append(convert(aux::to_string(*arg, std::hex, std::uppercase, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(aux::to_string(*arg, std::hex, std::uppercase)));
					return;
				}
			}
			break;
			case 'o':
			{
				if(begin != end)
				{
					char padding = static_cast<ucell>(*begin);
					++begin;
					auto width = aux::parse_num<std::streamsize>(begin, end);
					if(begin == end && width >= 0)
					{
						buf.append(convert(aux::to_string(*arg, std::oct, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(aux::to_string(*arg, std::oct)));
					return;
				}
			}
			break;
			case 'f':
			{
				float val = amx_ctof(*arg);
				if(*begin == '.')
				{
					++begin;
					auto precision = aux::parse_num<std::streamsize>(begin, end);
					if(begin == end && precision >= 0)
					{
						buf.append(convert(aux::to_string(val, std::setprecision(precision), std::fixed)));
						return;
					}
				}else if(begin == end)
				{
					buf.append(convert(std::to_string(val)));
					return;
				}
			}
			break;
			case 'c':
			{
				buf.append(1, *arg);
			}
			break;
			case 'b':
			{
				std::bitset<8> bits(*arg);
				buf.append(bits.to_string<cell>());
			}
			break;
			default:
			{
				amx_FormalError(errors::invalid_format, "invalid specifier");
			}
			break;
		}
		if(begin != end)
		{
			amx_FormalError(errors::invalid_format, "invalid specifier parameter");
		}
	}

	template <class ArgFunc>
	void operator()(Iter format_begin, Iter format_end, strings::cell_string &buf, cell argc, ArgFunc argf) const
	{
		auto flen = format_end - format_begin;
		buf.reserve(flen + 8 * argc);

		cell argn = -1;
		cell maxargn = -1;

		auto last = format_begin;
		while(format_begin != format_end)
		{
			if(*format_begin == '%')
			{
				buf.append(last, format_begin);

				++format_begin;
				if(format_begin == format_end)
				{
					amx_FormalError(errors::invalid_format, "unexpected end");
					return;
				}

				if(*format_begin == '%' || *format_begin == '{' || *format_begin == '}')
				{
					buf.append(1, *format_begin);
				}else{
					last = format_begin;
					bool pos_found = false;
					Iter pos_end = format_begin;
					while(format_begin != format_end && !std::isalpha(*format_begin))
					{
						if(*format_begin == '$')
						{
							pos_found = true;
							pos_end = format_begin;
						}
						++format_begin;
					}
					if(format_begin == format_end)
					{
						amx_FormalError(errors::invalid_format, "unexpected end");
						return;
					}
					if(pos_found && std::isdigit(*last))
					{
						cell argi = aux::parse_num<cell>(last, pos_end);
						if(last != pos_end)
						{
							amx_FormalError(errors::invalid_format, "expected '$'");
							return;
						}
						
						if(argi < 0)
						{
							amx_FormalError(errors::invalid_format, "negative argument index");
							return;
						}else if(argi < argc)
						{
							++pos_end;

							add_format(buf, pos_end, format_begin, argf(argi));
						}else if(argi > maxargn)
						{
							maxargn = argi;
						}
					}else{
						argn++;
						if(argn < argc)
						{
							add_format(buf, last, format_begin, argf(argn));
						}else if(argn > maxargn)
						{
							maxargn = argn;
						}
					}
				}
				++format_begin;
				last = format_begin;
			}else if(*format_begin == '{')
			{
				buf.append(last, format_begin);

				++format_begin;
				if(format_begin == format_end)
				{
					amx_FormalError(errors::invalid_format, "unexpected end");
					return;
				}

				cell argi = aux::parse_num<cell>(format_begin, format_end);

				if(format_begin == format_end || *format_begin != ':')
				{
					amx_FormalError(errors::invalid_format, "expected ':'");
					return;
				}

				++format_begin;
				last = format_begin;

				auto lastspec = format_begin;
				while(format_begin != format_end && *format_begin != '}')
				{
					lastspec = format_begin;
					++format_begin;
				}
				if(format_begin == format_end)
				{
					amx_FormalError(errors::invalid_format, "unexpected end");
					return;
				}
				if(last == lastspec)
				{
					amx_FormalError(errors::invalid_format, "missing specifier");
					return;
				}

				if(argi < 0)
				{
					amx_FormalError(errors::invalid_format, "negative argument index");
					return;
				}else if(argi < argc)
				{
					add_format(buf, last, lastspec, argf(argi));
				}else if(argi > maxargn)
				{
					maxargn = argi;
				}

				++format_begin;
				last = format_begin;
			}else if(*format_begin == '}')
			{
				amx_FormalError(errors::invalid_format, "unexpected '}'");
				return;
			}else{
				++format_begin;
			}
		}
		buf.append(last, format_end);

		if(maxargn >= argc)
		{
			throw errors::end_of_arguments_error(argf(-1), maxargn + 1);
		}
	}
};

void strings::format(AMX *amx, strings::cell_string &buf, const cell *format, cell argc, cell *args)
{
	select_iterator<format_base>(format, buf, argc, [&](cell argi)
	{
		if(argi == -1)
		{
			return args;
		}
		cell *addr = amx_GetAddrSafe(amx, args[argi]);
		return addr;
	});
}

void strings::format(AMX *amx, strings::cell_string &buf, const cell_string &format, cell argc, cell *args)
{
	format_base<cell_string::const_iterator>()(format.begin(), format.end(), buf, argc, [&](cell argi)
	{
		if(argi == -1)
		{
			return args;
		}
		cell *addr = amx_GetAddrSafe(amx, args[argi]);
		return addr;
	});
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

static const std::basic_regex<cell, regex_traits> &get_cached(const cell_string &pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(pattern, options), std::forward_as_tuple(pattern, syntax_options)).first->second;
}

template <class Iter>
static const std::basic_regex<cell, regex_traits> &get_cached(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	if(pattern != nullptr)
	{
		return get_cached(*pattern, options, syntax_options);
	}
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(cell_string(pattern_begin, pattern_end), options), std::forward_as_tuple(pattern_begin, pattern_end, syntax_options)).first->second;
}

template <class Iter>
struct regex_search_base
{
	bool operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(options & cache_flag)
		{
			return std::regex_search(str, get_cached(pattern_begin, pattern_end, pattern, options, syntax_options), match_options);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			return std::regex_search(str, regex, match_options);
		}
	}
};

bool strings::regex_search(const cell_string &str, const cell *pattern, cell options)
{
	try{
		return select_iterator<regex_search_base>(pattern, str, nullptr, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell options)
{
	try{
		return regex_search_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class Iter>
struct regex_extract_base
{
	cell operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		std::match_results<cell_string::const_iterator> match;
		if(options & cache_flag)
		{
			if(!std::regex_search(str, match, get_cached(pattern_begin, pattern_end, pattern, options, syntax_options), match_options))
			{
				return 0;
			}
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			if(!std::regex_search(str, match, regex, match_options))
			{
				return 0;
			}
		}
		tag_ptr chartag = tags::find_tag(tags::tag_char);
		auto list = list_pool.add();
		for(auto &group : match)
		{
			list->push_back(dyn_object(&*group.first, group.length(), chartag));
		}
		return list_pool.get_id(list);
	}
};

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell options)
{
	try{
		return select_iterator<regex_extract_base>(pattern, str, nullptr, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell options)
{
	try{
		return regex_extract_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class PatternIter>
struct regex_replace_base
{
	template <class ReplacementIter>
	struct inner
	{
		void operator()(ReplacementIter replacement_begin, ReplacementIter replacement_end, PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, cell options) const
		{
			std::regex_constants::syntax_option_type syntax_options;
			std::regex_constants::match_flag_type match_options;
			regex_options(options, syntax_options, match_options);

			if(options & cache_flag)
			{
				std::regex_replace(std::back_inserter(target), str.cbegin(), str.cend(), get_cached(pattern_begin, pattern_end, pattern, options, syntax_options), cell_string(replacement_begin, replacement_end), match_options);
			}else{
				std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
				std::regex_replace(std::back_inserter(target), str.cbegin(), str.cend(), regex, cell_string(replacement_begin, replacement_end), match_options);
			}
		}
	};

	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell *replacement, cell options) const
	{
		select_iterator<inner>(replacement, pattern_begin, pattern_end, target, str, nullptr, options);
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell options)
{
	try{
		select_iterator<regex_replace_base>(pattern, target, str, replacement, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell options)
{
	try{
		regex_replace_base<cell_string::const_iterator>::inner<cell_string::const_iterator>()(replacement.begin(), replacement.end(), pattern.begin(), pattern.end(), target, str, &pattern, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}
