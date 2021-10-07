#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED

#include "strings.h"
#include "containers.h"
#include "tag_ops.h"
#include "errors.h"

#include <stack>
#include <cctype>
#include <sstream>

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
	cell_string to_string(Obj &&obj, Args&&... args)
	{
		std::ostringstream ostream;
		ostream.imbue(std::locale());
		stream::push_args(ostream, std::forward<Args>(args)...);
		ostream << std::forward<Obj>(obj);
		return convert(ostream.str());
	}

	template <class Iter>
	struct format_state;

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

	template <class Iter>
	bool append_format(cell_string &buf, Iter begin, Iter end, const dyn_object &obj, num_parser<Iter> &&parse_num)
	{
		return append_format(buf, obj.get_specifier(), begin, end, obj, std::move(parse_num));
	}

	template <class Iter>
	bool append_format(cell_string &buf, cell specifier, Iter begin, Iter end, const dyn_object &obj, num_parser<Iter> &&parse_num)
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
		return tag->get_ops().format_base(tag, data, specifier, begin, end, std::move(parse_num), buf);
	}
		
	template <class StringIter>
	struct append_simple
	{
		void operator()(StringIter begin, StringIter end, cell_string &buf) const
		{
			buf.append(begin, end);
		}
	};

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
			while(begin != end && std::isdigit(c = *begin))
			{
				val = (val * 10) + (c - '0');
				++begin;
			}
			return val;
		}

		template <class QueryIter>
		struct add_query
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
		struct append
		{
			bool operator()(StringIter begin, StringIter end, Iter param_begin, Iter param_end, num_parser<Iter> &&parse_num, cell_string &buf) const
			{
				if(param_begin == param_end)
				{
					buf.append(begin, end);
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
							buf.append(begin, end);
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
	};
}

#endif
