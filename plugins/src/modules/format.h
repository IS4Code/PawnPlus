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

	template <class Iter>
	bool append_format(cell_string &buf, Iter begin, Iter end, const dyn_object &obj, const num_parser<Iter> &parse_num, const encoding &encoding)
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
