#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED

#include "strings.h"
#include "containers.h"
#include "tag_ops.h"
#include "errors.h"
#include "utils/contiguous_iterator.h"
#include "utils/systools.h"

#include <stack>
#include <cctype>
#include <cmath>
#include <sstream>
#include <type_traits>
#include <locale>
#include <bitset>
#include <ctime>
#include <iomanip>

namespace strings
{
	extern std::stack<std::weak_ptr<map_t>> format_env;

	void format(AMX *amx, strings::cell_string &str, const cell_string &format, cell argc, cell *args);
	void format(AMX *amx, strings::cell_string &str, const cell *format, cell argc, cell *args);

	namespace stream
	{
		inline void push_args(std::ostream &ostream)
		{

		}

		template <class Arg, class... Args>
		void push_args(std::ostream &ostream, Arg &&arg, Args&&... args)
		{
			ostream << std::forward<Arg>(arg);
			push_args(ostream, std::forward<Args>(args)...);
		}
	}

	template <class Obj, class... Args>
	cell_string to_string(const encoding &encoding, Obj &&obj, Args&&... args)
	{
		std::ostringstream ostream;
		ostream.imbue(encoding.locale);
		stream::push_args(ostream, std::forward<Args>(args)...);
		ostream << std::forward<Obj>(obj);
		return convert(ostream.str());
	}

	template <class Iter>
	struct num_parser
	{
		void *state;
		cell(*parse_func)(void *state, Iter &begin, Iter end);

		cell operator()(Iter &begin, Iter end) const
		{
			return parse_func(state, begin, end);
		}
	};

	inline bool is_letter(cell c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	inline bool is_digit(cell c)
	{
		return c >= '0' && c <= '9';
	}

	inline bool is_hex_digit(cell c)
	{
		return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	template <class Iter>
	bool append_implied_format(cell_string &buf, Iter begin, Iter end, const dyn_object &obj, const num_parser<Iter> &parse_num, const encoding &encoding)
	{
		return append_format(obj, strings::format_info<Iter>{obj.get_specifier(), begin, end, parse_num, buf, encoding});
	}

	template <class Iter>
	bool append_format(const dyn_object &obj, const strings::format_info<Iter> &info)
	{
		auto tag = obj.get_tag();
		if(obj.empty() || (obj.is_array() && !tag->inherits_from(tags::tag_char)))
		{
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(obj.to_string(info.encoding));
				return true;
			}
			return false;
		}

		const cell *data = obj.get_cell_addr(nullptr, 0);
		return tag->get_ops().format_base(tag, data, info);
	}
		
	template <class StringIter>
	struct append_simple
	{
		void operator()(StringIter begin, StringIter end, cell_string &buf) const
		{
			buf.append(begin, end);
		}
	};

	namespace impl
	{
		bool check_valid_time_format(std::string format);
	}

	template <class Iter>
	struct format_specific
	{
		static cell parse_num_simple(Iter &begin, Iter end)
		{
			if(begin == end)
			{
				return 0;
			}
			switch(*begin)
			{
				case '-':
				{
					++begin;
					auto oldbegin = begin;
					cell num = parse_num_simple(begin, end);
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

	private:
		template <class Type>
		static long double to_decimal(Type value, typename std::enable_if<std::is_integral<Type>::value, cell>::type precision)
		{
			return value * std::pow(10.0L, -std::abs(precision));
		}

		template <class Type>
		static typename std::enable_if<std::is_floating_point<Type>::value, Type>::type to_decimal(Type value, cell)
		{
			return value;
		}

	public:
		template <class Type, class... Args>
		static bool format_num(Type value, const format_info<Iter> &info, Args&&... args)
		{
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(to_string(info.encoding, value, std::forward<Args>(args)...));
				return true;
			}else if(*info.fmt_begin == '.')
			{
				++info.fmt_begin;
				if(info.fmt_begin == info.fmt_end)
				{
					return false;
				}
				cell precision = info.parse_num(info.fmt_begin, info.fmt_end);
				if(info.fmt_begin == info.fmt_end)
				{
					auto fvalue = to_decimal(value, precision);
					if(precision >= 0)
					{
						info.target.append(to_string(info.encoding, fvalue, std::setprecision(precision), std::fixed, std::forward<Args>(args)...));
					}else{
						info.target.append(to_string(info.encoding, fvalue, std::setprecision(-precision), std::defaultfloat, std::forward<Args>(args)...));
					}
					return true;
				}
			}else{
				char padding = static_cast<ucell>(*info.fmt_begin);
				++info.fmt_begin;
				if(info.fmt_begin == info.fmt_end)
				{
					return false;
				}
				cell width = info.parse_num(info.fmt_begin, info.fmt_end);
				if(width < 0)
				{
					return false;
				}
				if(info.fmt_begin == info.fmt_end)
				{
					info.target.append(to_string(info.encoding, value, std::setw(width), std::setfill(padding), std::forward<Args>(args)...));
					return true;
				}else if(*info.fmt_begin == '.')
				{
					++info.fmt_begin;
					if(info.fmt_begin == info.fmt_end)
					{
						return false;
					}
					cell precision = info.parse_num(info.fmt_begin, info.fmt_end);
					if(info.fmt_begin == info.fmt_end)
					{
						auto fvalue = to_decimal(value, precision);
						if(precision >= 0)
						{
							info.target.append(to_string(info.encoding, fvalue, std::setw(width), std::setfill(padding), std::setprecision(precision), std::fixed, std::forward<Args>(args)...));
						}else{
							info.target.append(to_string(info.encoding, fvalue, std::setw(width), std::setfill(padding), std::setprecision(-precision), std::defaultfloat, std::forward<Args>(args)...));
						}
						return true;
					}
				}
			}
			return false;
		}
		
	private:
		template <bool International>
		static long double adjust_digits(long double value, cell num_digits, const std::locale &locale)
		{
			const auto &punct = std::use_facet<std::moneypunct<char, International>>(locale);
			num_digits -= punct.frac_digits();
			return value * std::pow(10.0L, -num_digits);
		}

		static long double adjust_digits(long double value, cell num_digits, bool international, const std::locale &locale)
		{
			return international ? adjust_digits<true>(value, num_digits, locale) : adjust_digits<false>(value, num_digits, locale);
		}

	public:
		static bool format_money(long double value, const format_info<Iter> &info)
		{
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(to_string(info.encoding, std::put_money(value, true), std::showbase));
				return true;
			}
			bool international = true;
			if(*info.fmt_begin == '$')
			{
				international = false;
				++info.fmt_begin;
				if(info.fmt_begin == info.fmt_end)
				{
					info.target.append(to_string(info.encoding, std::put_money(value, international), std::showbase));
					return true;
				}
			}
			if(*info.fmt_begin == '.')
			{
				++info.fmt_begin;
				if(info.fmt_begin == info.fmt_end)
				{
					return false;
				}
				cell precision = info.parse_num(info.fmt_begin, info.fmt_end);
				if(info.fmt_begin == info.fmt_end)
				{
					value = adjust_digits(value, precision, international, info.encoding.locale);
					info.target.append(to_string(info.encoding, std::put_money(value, international), std::showbase));
					return true;
				}
				return false;
			}
			char padding = static_cast<ucell>(*info.fmt_begin);
			++info.fmt_begin;
			cell width = info.parse_num(info.fmt_begin, info.fmt_end);
			if(width < 0)
			{
				return false;
			}
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(to_string(info.encoding, std::put_money(value, international), std::showbase, std::setw(width), std::setfill(padding)));
				return true;
			}else if(*info.fmt_begin == '.')
			{
				++info.fmt_begin;
				if(info.fmt_begin == info.fmt_end)
				{
					return false;
				}
				cell precision = info.parse_num(info.fmt_begin, info.fmt_end);
				if(info.fmt_begin == info.fmt_end)
				{
					value = adjust_digits(value, precision, international, info.encoding.locale);
					info.target.append(to_string(info.encoding, std::put_money(value, international), std::showbase, std::setw(width), std::setfill(padding)));
					return true;
				}
			}
			return false;
		}
		
		static bool format_bits(cell value, const format_info<Iter> &info)
		{
			std::bitset<sizeof(cell) * 8> bits(value);
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(bits.to_string<cell>());
				return true;
			}
			cell zero = *info.fmt_begin;
			++info.fmt_begin;
			if(info.fmt_begin == info.fmt_end)
			{
				return false;
			}
			cell one = *info.fmt_begin;
			++info.fmt_begin;
			if(info.fmt_begin == info.fmt_end)
			{
				info.target.append(bits.to_string<cell>(zero, one));
				return true;
			}else if(*info.fmt_begin == '.')
			{
				++info.fmt_begin;
				cell limit = info.parse_num(info.fmt_begin, info.fmt_end);
				if(info.fmt_begin == info.fmt_end && limit > 0)
				{
					cell_string val(bits.to_string<cell>(zero, one));
					if(val.size() > static_cast<size_t>(limit))
					{
						info.target.append(val.begin() + val.size() - limit, val.end());
					}else{
						info.target.append(val);
					}
					return true;
				}
			}
			return false;
		}
		
		static bool format_time(cell value, const format_info<Iter> &info)
		{
			std::time_t time;
			if(*arg != -1)
			{
				time = static_cast<std::time_t>(*arg);
			}else{
				time = std::time(nullptr);
			}
			if(info.fmt_begin == info.fmt_end)
			{
				std::tm value = aux::localtime(time);
				info.target.append(to_string(info.encoding, std::put_time(&value, "%c")));
				return true;
			}
			bool offset_found = false;
			cell timezone_offset;
			Iter begin_after_escaped = info.fmt_end;

			Iter format_end = info.fmt_end;
			do{
				--format_end;

				char c = *format_end;
				bool negative = c == '-';
				if(c == '+' || negative)
				{
					// potential UTC offset
					Iter offset_begin = format_end;
					++offset_begin;
					if(offset_begin != info.fmt_end)
					{
						// data after
						timezone_offset = 60 * info.parse_num(offset_begin, info.fmt_end);
						offset_found = offset_begin == info.fmt_end;
						if(!offset_found)
						{
							// there is more data
							if(*offset_begin == ':' && ++offset_begin != info.fmt_end)
							{
								timezone_offset += info.parse_num(offset_begin, info.fmt_end);
								if(offset_begin == info.fmt_end)
								{
									// minutes and end found
									offset_found = true;
								}
							}
						}
					}
					if(negative)
					{
						timezone_offset = -timezone_offset;
					}
					if(offset_found && format_end != info.fmt_begin)
					{
						// ensure not escaped
						Iter percent_it = format_end;
						--percent_it;
						bool escaped = false;
						do{
							if(*percent_it == '%')
							{
								escaped = !escaped;
							}else{
								break;
							}
							--percent_it;
						}while(percent_it != info.fmt_begin);
						if(escaped)
						{
							// preceded by escaping %
							offset_found = false;
							// store the operator
							begin_after_escaped = format_end;
							// strip the %
							--format_end;
							break;
						}
					}
					if(!offset_found)
					{
						// revert
						format_end = info.fmt_end;
					}
					break;
				}
			}while(format_end != info.fmt_begin);

			// before escaped suffix
			auto format_initial_length = format_end - info.fmt_begin;
			std::string format(format_initial_length + (info.fmt_end - begin_after_escaped), '\0');

			// narrow before and after % using ctype<cell>
			const auto &ctype = std::use_facet<std::ctype<cell>>(info.encoding.locale);
			aux::make_contiguous(info.fmt_begin, format_end, [&](const cell *p_begin, const cell *p_end)
			{
				ctype.narrow(p_begin, p_end, info.encoding.unknown_char, &format[0]);
				return nullptr;
			});
			aux::make_contiguous(begin_after_escaped, info.fmt_end, [&](const cell *p_begin, const cell *p_end)
			{
				ctype.narrow(p_begin, p_end, info.encoding.unknown_char, &format[format_initial_length]);
				return nullptr;
			});

			if(!check_valid_time_format(format))
			{
				return false;
			}

			std::tm value;
			if(offset_found)
			{
				// add seconds to time
				timezone_offset *= 60;
				time += timezone_offset;
				value = aux::gmtime(time);
#ifndef _MSC_VER
				if(timezone_offset != 0)
				{
					// and timezone offset if present
					value.tm_gmtoff = timezone_offset;
					value.tm_zone = nullptr;
				}
#endif
			}else{
				value = aux::localtime(time);
			}

			info.target.append(to_string(info.encoding, std::put_time(&value, format.c_str())));
			return true;
		}
		
		template <class StringIter>
		struct append
		{
		private:
			void append_convert(StringIter begin, StringIter end, cell_string &buf, const encoding &encoding) const
			{
				if(encoding.locale != std::locale())
				{
					change_encoding(begin, end, default_encoding(), buf, encoding);
				}else{
					buf.append(begin, end);
				}
			}

		public:
			bool operator()(StringIter begin, StringIter end, Iter param_begin, Iter param_end, cell_string &buf, const num_parser<Iter> &parse_num, const encoding &encoding) const
			{
				auto appender = [&](StringIter sub_begin, StringIter sub_end)
				{
					append_convert(sub_begin, sub_end, buf, encoding);
				};

				if(param_begin == param_end)
				{
					appender(begin, end);
					return true;
				}else{
					if(*param_begin == '.')
					{
						++param_begin;
						cell max = parse_num(param_begin, param_end);
						if(param_begin != param_end || max < 0)
						{
							return false;
						}
						cell length = std::distance(begin, end);
						auto new_end = begin + std::min(max, length);
						appender(begin, new_end);
						return true;
					}else{
						cell padding = *param_begin;
						++param_begin;
						cell width = parse_num(param_begin, param_end);
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
							appender(begin, end);
							return true;
						}else if(*param_begin == '.')
						{
							++param_begin;
							cell max = parse_num(param_begin, param_end);
							if(param_begin != param_end || max < 0)
							{
								return false;
							}
							auto new_end = begin + max;
							if(new_end <= end)
							{
								appender(begin, new_end);
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
								appender(begin, end);
							}
							return true;
						}
						return false;
					}
				}
			}
		};

		template <class QueryIter>
		struct add_query
		{
			bool operator()(QueryIter begin, QueryIter end, Iter escape_begin, Iter escape_end, cell_string &buf, const num_parser<Iter> &parse_num, const encoding &encoding) const
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
				append<QueryIter> appender;
				auto last = begin;
				while(begin != end)
				{
					if((!custom && (*begin == '\'' || *begin == '\\')) || std::find(escape_begin, escape_end, *begin) != escape_end)
					{
						appender(last, begin, escape_end, escape_end, buf, parse_num, encoding);
						buf.append(1, escape_char);
						buf.append(1, *begin);
						++begin;
						last = begin;
					}else{
						++begin;
					}
				}
				appender(last, end, escape_end, escape_end, buf, parse_num, encoding);
				return true;
			}
		};
	};
}

#endif
