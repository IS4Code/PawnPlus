#include "format.h"
#include "errors.h"
#include "modules/parser.h"
#include "modules/variants.h"
#include "modules/tag_ops.h"

#include <cctype>
#include <sstream>
#include <bitset>
#include <iomanip>

using namespace strings;

std::stack<std::weak_ptr<map_t>> strings::format_env;

namespace strings
{
	template <class Iter>
	struct format_state
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
			while(begin != end && std::isdigit(c = *begin))
			{
				val = (val * 10) + (c - '0');
				++begin;
			}
			return val;
		}

		num_parser<Iter> get_num_parser()
		{
			return num_parser<Iter>{this, [](void *state, Iter &begin, Iter end)
			{
				return static_cast<format_state<Iter>*>(state)->parse_num(begin, end);
			}};
		}

		void add_format(cell_string &buf, Iter begin, Iter end, cell type, const cell *arg)
		{
			switch(type)
			{
				case 'P':
				{
					if(append_format(buf, begin, end, dyn_object(amx, arg[0], arg[1]), get_num_parser()))
					{
						return;
					}
				}
				break;
				default:
				{
					bool is_string;
					if(auto ops = tag_operations::from_specifier(type, is_string))
					{
						if(is_string)
						{
							cell val = strings::create(arg, false, false);
							if(ops->format_base(nullptr, &val, type, begin, end, get_num_parser(), buf))
							{
								return;
							}
						}else{
							if(ops->format_base(nullptr, arg, type, begin, end, get_num_parser(), buf))
							{
								return;
							}
						}
						break;
					}
					amx_FormalError(errors::invalid_format, "unknown specifier");
				}
				break;
			}
			amx_FormalError(errors::invalid_format, "invalid value or specifier parameter");
		}

		void operator()(Iter format_begin, Iter format_end, AMX *amx, strings::cell_string &buf, cell argc, const cell *args)
		{
			auto flen = format_end - format_begin;

			if(argc == 1 && flen == 2 && *format_begin == '%')
			{
				auto test = format_begin;
				++test;
				if(*test == 's')
				{
					// short-circuiting "%s" form
					select_iterator<append_simple>(amx_GetAddrSafe(amx, args[0]), buf);
					return;
				}
			}

			this->amx = amx;
			this->argc = argc;
			this->args = args;

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

					const auto brace_begin = format_begin;

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
						const auto brace_end = std::find(format_begin, format_end, '}');
						if(brace_end != format_end && brace_end - brace_begin == 7)
						{
							if(std::all_of(std::next(brace_begin), brace_end, [](cell c) {return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }))
							{
								format_begin = last = std::next(brace_end);
								buf.append(brace_begin, format_begin);
								continue;
							}
						}
						format_begin = std::next(brace_begin);
						auto expr = expression_parser<Iter>(parser_options::all).parse_partial(amx, format_begin, brace_end, ':');
						auto lock = format_env.size() > 0 ? format_env.top().lock() : std::shared_ptr<map_t>();
						expression::exec_info info(amx, lock.get(), true);

						if(format_begin == brace_end)
						{
							buf.append(expr->execute({}, info).to_string());
						}else{
							++format_begin;
							if(format_begin == brace_end)
							{
								amx_FormalError(errors::invalid_format, "missing specifier");
								return;
							}
							last = format_begin;
							format_begin = std::prev(brace_end);
							append_format(buf, *format_begin, last, format_begin, expr->execute({}, info), get_num_parser());
						}
						
						format_begin = last = std::next(brace_end);
					}else{
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
					}
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
}

void strings::format(AMX *amx, strings::cell_string &buf, const cell *format, cell argc, cell *args)
{
	select_iterator<format_state>(format, amx, buf, argc, args);
}

void strings::format(AMX *amx, strings::cell_string &buf, const cell_string &format, cell argc, cell *args)
{
	format_state<cell_string::const_iterator>()(format.begin(), format.end(), amx, buf, argc, args);
}
