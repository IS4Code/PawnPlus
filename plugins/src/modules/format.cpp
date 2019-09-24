#include "format.h"
#include "errors.h"
#include "modules/parser.h"
#include "modules/variants.h"

#include <cctype>
#include <sstream>
#include <bitset>
#include <iomanip>

using namespace strings;

std::stack<std::weak_ptr<map_t>> strings::format_env;

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
					if(begin == end)
					{
						if(precision >= 0)
						{
							buf.append(convert(aux::to_string(val, std::setprecision(precision), std::fixed)));
						}else{
							buf.append(convert(aux::to_string(val, std::setprecision(-precision), std::defaultfloat)));
						}
						return;
					}
				}else if(begin == end)
				{
					buf.append(convert(aux::to_string(val)));
					return;
				}else{
					char padding = static_cast<ucell>(*begin);
					++begin;
					cell width = parse_num(begin, end);
					if(width < 0)
					{
						break;
					}
					if(begin == end)
					{
						buf.append(convert(aux::to_string(val, std::setw(width), std::setfill(padding))));
						return;
					}else if(*begin == '.')
					{
						++begin;
						cell precision = parse_num(begin, end);
						if(begin == end)
						{
							if(precision >= 0)
							{
								buf.append(convert(aux::to_string(val, std::setw(width), std::setfill(padding), std::setprecision(precision), std::fixed)));
							}else{
								buf.append(convert(aux::to_string(val, std::setw(width), std::setfill(padding), std::setprecision(-precision), std::defaultfloat)));
							}
							return;
						}
						return;
					}
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
		buf.reserve(buf.size() + flen + 8 * argc);

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
					auto expr = expression_parser<Iter>(parser_options::all).parse_simple(amx, std::next(color_begin), color_end);
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
