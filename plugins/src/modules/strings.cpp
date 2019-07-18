#include "strings.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include "modules/parser.h"
#include "objects/stored_param.h"

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
#include <functional>

using namespace strings;

std::stack<std::weak_ptr<map_t>> strings::format_env;

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
		ostream.imbue(std::locale());
		push_args(ostream, std::forward<Args>(args)...);
		ostream << std::forward<Obj>(obj);
		return ostream.str();
	}
}

template <class Iter>
struct format_base
{
	AMX *amx;
	cell argn = -1;
	cell maxargn = -1;
	cell argc;
	const cell *args;

	const cell *get_arg(cell argi) const
	{
		return amx_GetAddrSafe(amx, args[argi]);
	}

	cell parse_num(Iter &begin, Iter end)
	{
		if(begin == end)
		{
			return 0;
		}
		switch(*begin)
		{
			case '^':
			{
				++begin;
				argn++;
				return argn;
			}
			break;
			case '*':
			{
				++begin;
				argn++;
				if(argn >= argc)
				{
					throw errors::end_of_arguments_error(args, argn + 1);
				}
				return *get_arg(argn);
			}
			break;
			case '@':
			{
				++begin;
				auto oldbegin = begin;
				cell argi = parse_num(begin, end);
				if(argi < 0)
				{
					amx_LogicError(errors::invalid_format, "negative argument index");
				}
				if(begin == oldbegin)
				{
					amx_FormalError(errors::invalid_format, "invalid specifier parameter");
				}
				if(argi >= argc)
				{
					throw errors::end_of_arguments_error(args, argi + 1);
				}
				return *get_arg(argi);
			}
			break;
			case '-':
			{
				++begin;
				auto oldbegin = begin;
				cell num = parse_num(begin, end);
				if(begin != oldbegin)
				{
					return -num;
				}
				amx_FormalError(errors::invalid_format, "invalid specifier parameter");
			}
			break;
		}
		cell val = 0;
		cell c;
		while(std::isdigit(c = *begin) && begin != end)
		{
			val = (val * 10) + (c - '0');
			++begin;
		}
		return val;
	}

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
		bool operator()(StringIter begin, StringIter end, Iter param_begin, Iter param_end, format_base<Iter> &base, cell_string &buf) const
		{
			if(param_begin == param_end)
			{
				buf.append(begin, end);
				return true;
			}else{
				if(*param_begin == '.')
				{
					++param_begin;
					cell max = base.parse_num(param_begin, param_end);
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
					cell width = base.parse_num(param_begin, param_end);
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
						cell max = base.parse_num(param_begin, param_end);
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

	bool add_format(cell_string &buf, Iter begin, Iter end, const dyn_object &obj)
	{
		auto tag = obj.get_tag();
		if(obj.empty() || (obj.is_array() && !tag->inherits_from(tags::tag_char)))
		{
			if(begin == end)
			{
				buf.append(obj.to_string());
				return true;
			}
			return false;
		}

		const cell *data = obj.get_cell_addr(nullptr, 0);
		if(tag->inherits_from(tags::tag_signed) || tag->uid == tags::tag_cell)
		{
			add_format(buf, begin, end, 'i', data);
			return true;
		}else if(tag->inherits_from(tags::tag_unsigned))
		{
			add_format(buf, begin, end, 'u', data);
			return true;
		}else if(tag->inherits_from(tags::tag_char))
		{
			if(obj.is_array())
			{
				add_format(buf, begin, end, 's', data);
			}else{
				add_format(buf, begin, end, 'c', data);
			}
			return true;
		}else if(tag->inherits_from(tags::tag_float))
		{
			add_format(buf, begin, end, 'f', data);
			return true;
		}
		if(begin == end)
		{
			buf.append(obj.to_string());
			return true;
		}
		return false;
	}

	void add_format(cell_string &buf, Iter begin, Iter end, cell type, const cell *arg)
	{
		ptrdiff_t flen = end - begin;
		switch(type)
		{
			case 's':
			{
				if(select_iterator<append_base>(arg, begin, end, *this, buf))
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
					if(select_iterator<append_base>(str, begin, end, *this, buf))
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
					cell width = parse_num(begin, end);
					if(begin == end && width > 0)
					{
						buf.append(convert(aux::to_string(*arg, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(aux::to_string(*arg)));
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
					cell width = parse_num(begin, end);
					if(begin == end && width > 0)
					{
						buf.append(convert(aux::to_string(val, std::setw(width), std::setfill(padding))));
						return;
					}
				}else{
					buf.append(convert(aux::to_string(val)));
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
					cell width = parse_num(begin, end);
					if(begin == end && width > 0)
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
					cell width = parse_num(begin, end);
					if(begin == end && width > 0)
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
					cell precision = parse_num(begin, end);
					if(begin == end && precision >= 0)
					{
						buf.append(convert(aux::to_string(val, std::setprecision(precision), std::fixed)));
						return;
					}
				}else if(begin == end)
				{
					buf.append(convert(aux::to_string(val)));
					return;
				}
			}
			break;
			case 'c':
			{
				if(begin == end)
				{
					buf.append(1, *arg);
					return;
				}else{
					cell count = parse_num(begin, end);
					if(begin == end && count > 0)
					{
						buf.append(count, *arg);
						return;
					}
				}
			}
			break;
			case 'b':
			{
				std::bitset<sizeof(cell) * 8> bits(*arg);
				if(begin != end)
				{
					cell zero = *begin;
					++begin;
					if(begin != end)
					{
						cell one = *begin;
						++begin;
						if(begin == end)
						{
							buf.append(bits.to_string<cell>(zero, one));
							return;
						}else if(*begin == '.')
						{
							++begin;
							cell limit = parse_num(begin, end);
							if(begin == end && limit > 0)
							{
								cell_string val(bits.to_string<cell>(zero, one));
								if(val.size() > static_cast<size_t>(limit))
								{
									buf.append(val.begin() + val.size() - limit, val.end());
								}else{
									buf.append(val);
								}
								return;
							}
						}
					}
				}else{
					buf.append(bits.to_string<cell>());
					return;
				}
			}
			break;
			case 'V':
			{
				dyn_object *var;
				if(variants::pool.get_by_id(*arg, var))
				{
					if(add_format(buf, begin, end, *var))
					{
						return;
					}
				}else if(var != nullptr)
				{
					amx_LogicError(errors::pointer_invalid, "variant", *arg);
					return;
				}else if(begin == end)
				{
					return;
				}
			}
			break;
			case 'P':
			{
				if(add_format(buf, begin, end, dyn_object(amx, arg[0], arg[1])))
				{
					return;
				}
			}
			break;
			default:
			{
				amx_FormalError(errors::invalid_format, "invalid specifier");
			}
			break;
		}
		amx_FormalError(errors::invalid_format, "invalid specifier parameter");
	}

	void operator()(Iter format_begin, Iter format_end, AMX *amx, strings::cell_string &buf, cell argc, const cell *args)
	{
		this->amx = amx;
		this->argc = argc;
		this->args = args;

		auto flen = format_end - format_begin;
		buf.reserve(flen + 8 * argc);

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
					auto last2 = last;
					cell argi;
					if(pos_found && (argi = parse_num(last2, pos_end)) >= 0 && last2 == pos_end)
					{
						if(argi < argc)
						{
							++pos_end;

							add_format(buf, pos_end, format_begin, *format_begin, get_arg(argi));
						}else if(argi > maxargn)
						{
							maxargn = argi;
						}
					}else{
						argn++;
						if(argn < argc)
						{
							add_format(buf, last2, format_begin, *format_begin, get_arg(argn));
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

				auto color_begin = format_begin;

				++format_begin;
				if(format_begin == format_end)
				{
					amx_FormalError(errors::invalid_format, "unexpected end");
					return;
				}

				cell argi = parse_num(format_begin, format_end);

				if(format_begin == format_end)
				{
					amx_FormalError(errors::invalid_format, "expected ':'");
					return;
				}
				if(*format_begin != ':')
				{
					auto color_end = std::find(format_begin, format_end, '}');
					if(color_end != format_end && color_end - color_begin == 7)
					{
						if(std::all_of(std::next(color_begin), color_end, [](cell c) {return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }))
						{
							++color_end;
							buf.append(color_begin, color_end);
							format_begin = last = color_end;
							continue;
						}
					}
					auto expr = expression_parser<Iter>().parse_simple(amx, std::next(color_begin), color_end);
					auto lock = format_env.size() > 0 ? format_env.top().lock() : std::shared_ptr<map_t>();
					buf.append(expr->execute({}, expression::exec_info(amx, lock.get(), true)).to_string());
					
					++color_end;
					format_begin = last = color_end;
					continue;
				}

				last = format_begin;
				auto lastspec = format_begin;
				++format_begin;

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
				++last;

				if(argi < 0)
				{
					amx_FormalError(errors::invalid_format, "negative argument index");
					return;
				}else if(argi < argc)
				{
					add_format(buf, last, lastspec, *lastspec, get_arg(argi));
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
			throw errors::end_of_arguments_error(args, maxargn + 1);
		}
	}
};

void strings::format(AMX *amx, strings::cell_string &buf, const cell *format, cell argc, cell *args)
{
	select_iterator<format_base>(format, amx, buf, argc, args);
}

void strings::format(AMX *amx, strings::cell_string &buf, const cell_string &format, cell argc, cell *args)
{
	format_base<cell_string::const_iterator>()(format.begin(), format.end(), amx, buf, argc, args);
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
inline void hash_combine(size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

const std::ctype<char> *base_facet;

template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct tuple_hash
{
	void operator()(size_t &seed, const Tuple &tuple) const
	{
		tuple_hash<Tuple, Index - 1>()(seed, tuple);
		hash_combine(seed, std::get<Index>(tuple));
	}
};

template <class Tuple>
struct tuple_hash<Tuple, 0>
{
	void operator()(size_t &seed, const Tuple &tuple) const
	{
		seed = std::hash<typename std::tuple_element<0, Tuple>::type>()(std::get<0>(tuple));
	}
};

namespace std
{
	template <class Type1, class Type2>
	struct hash<std::pair<Type1, Type2>>
	{
		size_t operator()(const std::pair<Type1, Type2> &obj) const
		{
			size_t seed = std::hash<Type1>()(obj.first);
			hash_combine(seed, obj.second);
			return seed;
		}
	};

	template <class... Type>
	struct hash<std::tuple<Type...>>
	{
		size_t operator()(const std::tuple<Type...> &obj) const
		{
			size_t seed;
			tuple_hash<std::tuple<Type...>>()(seed, obj);
			return seed;
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

void strings::set_locale(const std::locale &loc, cell category)
{
	auto cat = get_category(category);
	if(cat == std::locale::all)
	{
		std::locale::global(custom_locale = std::locale(loc, &cell_ctype));
	}else{
		std::locale::global(custom_locale = std::locale(std::locale(custom_locale, loc, cat), &cell_ctype));
	}
	custom_locale_name = loc.name();
	base_facet = &std::use_facet<std::ctype<char>>(custom_locale);
}

const std::string &strings::locale_name()
{
	return custom_locale_name;
}

cell strings::to_lower(cell c)
{
	if(c < 0 || c > std::numeric_limits<unsigned char>::max())
	{
		return c;
	}
	return static_cast<unsigned char>(base_facet->tolower(static_cast<unsigned char>(c)));
}

cell strings::to_upper(cell c)
{
	if(c < 0 || c > std::numeric_limits<unsigned char>::max())
	{
		return c;
	}
	return static_cast<unsigned char>(base_facet->toupper(static_cast<unsigned char>(c)));
}

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
constexpr const cell cache_addr_flag = 4194304 | 8388608;

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
static const std::basic_regex<cell, regex_traits> &get_cached_addr(Iter pattern_begin, Iter pattern_end, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	static std::unordered_map<std::tuple<const void*, const void*, cell>, std::basic_regex<cell, regex_traits>> regex_cache;
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(&*pattern_begin, &*pattern_end, options), std::forward_as_tuple(pattern_begin, pattern_end, syntax_options)).first->second;
}

template <class Iter>
struct regex_search_base
{
	bool operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		std::match_results<cell_string::const_iterator> match;
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return false;
			}
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return false;
			}
		}
		*pos = match[0].second - str.cbegin();
		return true;
	}
};

bool strings::regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options)
{
	try{
		return select_iterator<regex_search_base>(pattern, str, nullptr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options)
{
	try{
		return regex_search_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class Iter>
struct regex_extract_base
{
	cell operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		std::match_results<cell_string::const_iterator> match;
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return 0;
			}
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return 0;
			}
		}
		*pos = match[0].second - str.cbegin();
		tag_ptr chartag = tags::find_tag(tags::tag_char);
		auto list = list_pool.add();
		for(auto &group : match)
		{
			dyn_object obj(&*group.first, group.length() + 1, chartag);
			*(obj.end() - 1) = 0;
			list->push_back(std::move(obj));
		}
		return list_pool.get_id(list);
	}
};

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options)
{
	try{
		return select_iterator<regex_extract_base>(pattern, str, nullptr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options)
{
	try{
		return regex_extract_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class SubIter>
struct replace_sub_match_base
{
	template <class Iter>
	struct inner
	{
		void operator()(Iter begin, Iter end, cell_string &target, SubIter sbegin, SubIter send) const
		{
			auto last = begin;
			while(begin != end)
			{
				if(*begin == '$' || *begin == '\\')
				{
					auto c = *begin;
					auto spec = begin;
					++begin;
					if(begin != end)
					{
						if(*begin == c)
						{
							target.append(last, begin);
							last = ++begin;
							continue;
						}else if(*begin >= '1' && *begin <= '9')
						{
							size_t num = *begin - '1';
							target.append(last, spec);
							auto siter = sbegin;
							while(num > 0)
							{
								if(siter != send)
								{
									++siter;
									num--;
								}else{
									break;
								}
							}
							if(siter != send)
							{
								target.append(siter->first, siter->second);
							}
							last = ++begin;
							continue;
						}
					}
				}else{
					++begin;
				}
			}
			target.append(last, begin);
		}
	};
};

template <class StringIter, class ReplacementIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, ReplacementIter replacement_begin, ReplacementIter replacement_end, std::regex_constants::match_flag_type match_options)
{
	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);
		typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<ReplacementIter>()(replacement_begin, replacement_end, target, std::next(result.cbegin()), result.cend());
		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_base
{
	template <class ReplacementIter>
	struct inner
	{
		void operator()(ReplacementIter replacement_begin, ReplacementIter replacement_end, PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
		{
			std::regex_constants::syntax_option_type syntax_options;
			std::regex_constants::match_flag_type match_options;
			regex_options(options, syntax_options, match_options);

			if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
			{
				amx_LogicError(errors::out_of_range, "pos");
			}else if(*pos > 0)
			{
				match_options |= std::regex_constants::match_prev_avail;
			}
			auto begin = str.cbegin() + *pos;
			target.append(str.cbegin(), begin);
			if(options & cache_flag)
			{
				const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
				replace(target, begin, str.cend(), regex, replacement_begin, replacement_end, match_options);
			}else{
				std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
				replace(target, begin, str.cend(), regex, replacement_begin, replacement_end, match_options);
			}
			*pos = begin - str.cbegin();
		}
	};

	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell *replacement, cell *pos, cell options) const
	{
		select_iterator<inner>(replacement, pattern_begin, pattern_end, target, str, nullptr, pos, options);
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell *pos, cell options)
{
	try{
		select_iterator<regex_replace_base>(pattern, target, str, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options)
{
	try{
		typename regex_replace_base<cell_string::const_iterator>::template inner<cell_string::const_iterator>()(replacement.begin(), replacement.end(), pattern.begin(), pattern.end(), target, str, &pattern, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, const list_t &replacement, std::regex_constants::match_flag_type match_options)
{
	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);
		size_t index = 0;
		for(auto it = std::next(result.cbegin()); it != result.cend();)
		{
			index++;
			const auto &capture = *it;
			if(capture.matched && index <= replacement.size())
			{
				const dyn_object &repl = replacement[index - 1];

				auto begin = it;
				++it;
				auto end = std::find_if(it, result.cend(), [](const std::sub_match<StringIter> &match) {return !match.matched; });
				it = end;

				if(repl.get_tag()->inherits_from(tags::tag_string))
				{
					cell value = repl.get_cell(0);
					cell_string *repl_str;
					if(strings::pool.get_by_id(value, repl_str))
					{
						typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<cell_string::const_iterator>()(repl_str->cbegin(), repl_str->cend(), target, begin, end);
						continue;
					}
				}else if(repl.get_tag()->inherits_from(tags::tag_char) && repl.is_array())
				{
					select_iterator<replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner>(repl.begin(), target, begin, end);
					continue;
				}
				cell_string str = repl.to_string();
				typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<cell_string::const_iterator>()(str.cbegin(), str.cend(), target, begin, end);
			}else{
				++it;
			}
		}
		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_list_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, const list_t &replacement, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		target.append(str.cbegin(), begin);
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			replace(target, begin, str.cend(), regex, replacement, match_options);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			replace(target, begin, str.cend(), regex, replacement, match_options);
		}
		*pos = begin - str.cbegin();
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const list_t &replacement, cell *pos, cell options)
{
	try{
		select_iterator<regex_replace_list_base>(pattern, target, str, nullptr, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options)
{
	try{
		regex_replace_list_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, str, &pattern, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, AMX *amx, int replacement_index, std::regex_constants::match_flag_type match_options, const char *format, cell *params, size_t numargs)
{
	std::vector<stored_param> arg_values;
	if(format != nullptr)
	{
		size_t argi = -1;
		size_t len = std::strlen(format);
		for(size_t i = 0; i < len; i++)
		{
			arg_values.push_back(stored_param::create(amx, format[i], params, argi, numargs));
		}
	}

	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);

		cell reset_hea, *addr;
		amx_Allot(amx, 0, &reset_hea, &addr);

		for(int i = result.size() - 1; i >= 0; i--)
		{
			const auto &capture = result[i];

			amx_Push(amx, capture.length());

			cell val;
			if(capture.matched)
			{
				auto len = capture.length();
				amx_Allot(amx, len + 1, &val, &addr);
				std::copy(capture.first, capture.second, addr);
				addr[len] = 0;
			}else{
				amx_Allot(amx, 1, &val, &addr);
				*addr = 0;
			}
			amx_Push(amx, val);
		}

		for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
		{
			it->push(amx, 0);
		}

		cell retval = 0;
		if(amx_Exec(amx, &retval, replacement_index) != AMX_ERR_NONE)
		{
			return;
		}
		if(retval != 0)
		{
			cell_string *repl;
			if(!strings::pool.get_by_id(retval, repl))
			{
				amx_LogicError(errors::pointer_invalid, "string", retval);
				return;
			}
			target.append(repl->cbegin(), repl->cend());
		}

		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_func_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		target.append(str.cbegin(), begin);
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			replace(target, begin, str.cend(), regex, amx, replacement_index, match_options, format, params, numargs);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			replace(target, begin, str.cend(), regex, amx, replacement_index, match_options, format, params, numargs);
		}
		*pos = begin - str.cbegin();
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs)
{
	try{
		select_iterator<regex_replace_func_base>(pattern, target, str, nullptr, amx, replacement_index, pos, options, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs)
{
	try{
		regex_replace_func_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, str, &pattern, amx, replacement_index, pos, options, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}
