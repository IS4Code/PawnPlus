#ifndef INT_REGEX_H_INCLUDED
#define INT_REGEX_H_INCLUDED

#include "int_string.h"

#include <string>
#include <regex>
#include <type_traits>
#include <algorithm>

namespace impl
{
	template <class CharType>
	class regex_traits
	{
		const ::impl::int_ctype<CharType> *char_ctype;
		std::locale base_locale;
		bool is_wide;
		union
		{
			std::regex_traits<char> char_traits;
			std::regex_traits<wchar_t> wchar_t_traits;
		};

		void cache_locale()
		{
			bool was_wide = is_wide;
			char_ctype = &std::use_facet<::impl::int_ctype<CharType>>(base_locale);
			is_wide = char_ctype->is_unicode();
			if(is_wide)
			{
				if(!was_wide)
				{
					char_traits.~regex_traits<char>();
					new (&wchar_t_traits) std::regex_traits<wchar_t>();
				}
				wchar_t_traits.imbue(base_locale);
			}else{
				if(was_wide)
				{
					wchar_t_traits.~regex_traits<wchar_t>();
					new (&char_traits) std::regex_traits<char>();
				}
				char_traits.imbue(base_locale);
			}
		}

		template <std::size_t Size>
		static std::basic_string<CharType> get_str(const char(&source)[Size])
		{
			std::basic_string<CharType> result(Size - 1, CharType());
			std::copy(std::begin(source), std::prev(std::end(source)), result.begin());
			return result;
		}

	public:
		typedef CharType char_type;
		typedef std::basic_string<CharType> string_type;
		typedef std::size_t size_type;
		typedef std::locale locale_type;
		typedef typename std::ctype<CharType>::mask char_class_type;

		typedef typename std::make_unsigned<CharType>::type _Uelem;
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

		regex_traits() : is_wide(false), char_traits()
		{
			cache_locale();
		}

		~regex_traits()
		{
			if(is_wide)
			{
				wchar_t_traits.~regex_traits<wchar_t>();
			}else{
				char_traits.~regex_traits<char>();
			}
		}

		int value(CharType ch, int base) const
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

		static size_type length(const CharType *str)
		{
			return std::char_traits<CharType>::length(str);
		}

		CharType translate(CharType ch) const
		{
			if(is_wide)
			{
				wchar_t n = char_ctype->narrow(ch, L'\0');
				if(n != L'\0')
				{
					return char_ctype->widen(wchar_t_traits.translate(ch));
				}
			}else{
				char n = char_ctype->narrow(ch, '\0');
				if(n != '\0')
				{
					return char_ctype->widen(char_traits.translate(ch));
				}
			}
			return ch;
		}

		CharType translate_nocase(CharType ch) const
		{
			return translate(char_ctype->tolower(ch));
		}

		template <class Iterator>
		string_type transform(Iterator first, Iterator last) const
		{
			return string_type(first, last);
		}

		template <class Iterator>
		string_type transform_primary(Iterator first, Iterator last) const
		{
			string_type res(first, last);
			std::transform(res.begin(), res.end(), res.begin(), [&](CharType c) { return tolower(c); });
			return res;
		}

		bool isctype(CharType ch, char_class_type ctype) const
		{
			if(ctype != static_cast<char_class_type>(-1))
			{
				return char_ctype->is(ctype, ch);
			}else{
				return ch == '_' || char_ctype->is(std::ctype_base::alnum, ch);
			}
		}

		template <class Iterator>
		char_class_type lookup_classname(Iterator first, Iterator last, bool icase = false) const
		{
			static std::unordered_map<string_type, char_class_type> map{
				{get_str("alnum"), std::ctype_base::alnum},
				{get_str("alpha"), std::ctype_base::alpha},
				{get_str("cntrl"), std::ctype_base::cntrl},
				{get_str("digit"), std::ctype_base::digit},
				{get_str("graph"), std::ctype_base::graph},
				{get_str("lower"), std::ctype_base::lower},
				{get_str("print"), std::ctype_base::print},
				{get_str("punct"), std::ctype_base::punct},
				{get_str("space"), std::ctype_base::space},
				{get_str("upper"), std::ctype_base::upper},
				{get_str("xdigit"), std::ctype_base::xdigit}
			};

			string_type key(first, last);
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

		template <class Iterator>
		string_type lookup_collatename(Iterator first, Iterator last) const
		{
			return string_type(first, last);
		}

		locale_type imbue(locale_type loc)
		{
			std::swap(base_locale, loc);
			cache_locale();
			return loc;
		}

		locale_type getloc() const
		{
			return base_locale;
		}
	};
}

namespace std
{
	template <>
	class regex_traits<std::int32_t> : public ::impl::regex_traits<std::int32_t>
	{

	};
}

#endif
