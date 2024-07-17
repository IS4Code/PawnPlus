#include "format.h"
#include "errors.h"
#include "modules/parser.h"
#include "modules/variants.h"
#include "modules/tag_ops.h"

#include <cctype>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <regex>

using namespace strings;

std::stack<std::weak_ptr<map_t>> strings::format_env;

namespace strings
{
	template <class Iter>
	class format_state
	{
		AMX *amx;
		cell argn = -1;
		cell maxargn = -1;
		cell argc;
		const cell *args;
		strings::encoding *encoding;

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
			while(begin != end && is_digit(c = *begin))
			{
				val = (val * 10) + (c - '0');
				++begin;
			}
			return val;
		}

		num_parser<Iter> get_num_parser()
		{
			return num_parser<Iter>{this, argn, [](void *state, Iter &begin, Iter end)
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
					if(append_implied_format(buf, begin, end, dyn_object(amx, arg[0], arg[1]), get_num_parser(), *encoding))
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
						strings::format_info<Iter> info{type, begin, end, get_num_parser(), buf, *encoding};
						if(is_string)
						{
							cell val = strings::create(arg, false, false);
							if(ops->format_base(nullptr, &val, info))
							{
								return;
							}
						}else{
							if(ops->format_base(nullptr, arg, info))
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

		template <class Iter>
		void parse_encoding_buffer(Iter begin, Iter end, char *spec, std::size_t size)
		{
			std::copy(begin, end, reinterpret_cast<unsigned char*>(spec));
			spec[size] = '\0';
			try{
				auto enc = find_encoding(spec, false);
				*encoding = enc.make_installed();
			}catch(const std::runtime_error &)
			{
				amx_LogicError(errors::locale_not_found, spec);
			}
		}

	public:
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
			strings::encoding encoding = strings::default_encoding();
			this->encoding = &encoding;

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
						while(format_begin != format_end && !is_letter(*format_begin))
						{
							if(*format_begin == '$' && !pos_found && format_begin != last)
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
						if(brace_end != format_end)
						{
							auto size = std::distance(brace_begin, brace_end);
							if(size == 7 && std::all_of(std::next(brace_begin), brace_end, [](cell c) {return is_hex_digit(c); }))
							{
								format_begin = last = std::next(brace_end);
								buf.append(brace_begin, format_begin);
								continue;
							}
							size -= 1;
							auto selector_begin = std::next(brace_begin);
							static const cell enc_selector[] = {'$', 'e', 'n', 'c', ':'};
							constexpr auto enc_size = std::extent<decltype(enc_selector)>::value;
							if(size >= enc_size && std::equal(std::begin(enc_selector), std::end(enc_selector), selector_begin))
							{
								format_begin = last = std::next(brace_end);
								std::advance(selector_begin, enc_size);
								size -= enc_size;
								if(size < 32)
								{
									char *ptr = static_cast<char*>(alloca((size + 1) * sizeof(char)));
									parse_encoding_buffer(selector_begin, brace_end, ptr, size);
								}else{
									auto memory = std::make_unique<char[]>(size + 1);
									auto ptr = memory.get();
									parse_encoding_buffer(selector_begin, brace_end, ptr, size);
								}
								continue;
							}
						}
						format_begin = std::next(brace_begin);
						auto expr = expression_parser<Iter>(parser_options::all).parse_partial(amx, format_begin, brace_end, ':');
						auto lock = format_env.size() > 0 ? format_env.top().lock() : std::shared_ptr<map_t>();
						expression::exec_info info(amx, lock.get(), true);

						if(format_begin == brace_end)
						{
							buf.append(expr->execute({}, info).to_string(encoding));
						}else{
							++format_begin;
							if(format_begin == brace_end)
							{
								amx_FormalError(errors::invalid_format, "missing specifier");
								return;
							}
							last = format_begin;
							format_begin = std::prev(brace_end);
							append_format(expr->execute({}, info), strings::format_info<Iter>{
								*format_begin, last, format_begin, get_num_parser(), buf, encoding
							});
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

bool strings::impl::check_valid_time_format(std::string format)
{
	static std::regex valid_pattern = ([]()
	{
		std::regex result;
		result.imbue(std::locale::classic());
		result.assign("^(?:[^%\\0]+|%(?:E[YyCcxX]|0[ymUWVdewuHIMS]|[%ntYyCGgbhBmUWVjdeaAwuHIMScxXDFrRTpzZ]))*$", std::regex_constants::ECMAScript | std::regex_constants::optimize);
		return result;
	})();

	return std::regex_match(format, valid_pattern);
}
