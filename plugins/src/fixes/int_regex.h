#ifndef INT_REGEX_H_INCLUDED
#define INT_REGEX_H_INCLUDED

#include "int_string.h"

#include <string>
#include <regex>
#include <type_traits>
#include <algorithm>
#include <utility>

namespace impl
{
	template <class CharType>
	class regex_traits
	{
		const ::impl::int_ctype<CharType> *base_int_ctype;
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
			base_int_ctype = &std::use_facet<::impl::int_ctype<CharType>>(base_locale);
			is_wide = base_int_ctype->is_unicode();
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

		template <class BaseCharType, std::size_t Size>
		static std::basic_string<CharType> get_str(const BaseCharType(&source)[Size])
		{
			using unsigned_narrow_char_type = typename std::make_unsigned<BaseCharType>::type;

			std::basic_string<CharType> result(Size - 1, CharType());
			std::transform(std::begin(source), std::prev(std::end(source)), result.begin(), [](BaseCharType c) { return static_cast<unsigned_narrow_char_type>(c); });
			return result;
		}

		template <class BaseCharType>
		static std::basic_string<CharType> get_str(const std::basic_string<BaseCharType> &source)
		{
			using unsigned_narrow_char_type = typename std::make_unsigned<BaseCharType>::type;

			std::basic_string<CharType> result(source.size(), CharType());
			std::transform(source.begin(), source.end(), result.begin(), [](BaseCharType c) { return static_cast<unsigned_narrow_char_type>(c); });
			return result;
		}
		
		template <class Func>
		auto get_traits(Func func) const -> decltype(func(std::declval<const std::regex_traits<char>>()))
		{
			if(is_wide)
			{
				return func(wchar_t_traits);
			}else{
				return func(char_traits);
			}
		}

		template <class Traits>
		using traits_char_type = typename std::remove_cv<typename std::remove_reference<Traits>::type>::type::char_type;

		template <class Iterator>
		using is_contiguous_iterator = std::integral_constant<bool,
			std::is_same<Iterator, CharType*>::value ||
			std::is_same<Iterator, const CharType*>::value ||
			std::is_same<Iterator, typename std::basic_string<CharType>::iterator>::value ||
			std::is_same<Iterator, typename std::basic_string<CharType>::const_iterator>::value ||
			std::is_same<Iterator, typename std::vector<CharType>::iterator>::value ||
			std::is_same<Iterator, typename std::vector<CharType>::const_iterator>::value
		>;

		template <class Receiver>
		static auto make_contiguous(const CharType *begin, const CharType *end, Receiver receiver) -> decltype(receiver(std::declval<const CharType*>(), std::declval<const CharType*>()))
		{
			return receiver(begin, end);
		}

		template <class Iterator, class Receiver>
		static auto make_contiguous(Iterator begin, typename std::enable_if<is_contiguous_iterator<Iterator>::value, Iterator>::type end, Receiver receiver) -> decltype(receiver(std::declval<const CharType*>(), std::declval<const CharType*>()))
		{
			auto ptr = &*begin;
			return receiver(ptr, ptr + (end - begin));
		}

		template <class Iterator, class Receiver>
		static auto make_contiguous(Iterator begin, typename std::enable_if<!is_contiguous_iterator<Iterator>::value, Iterator>::type end, Receiver receiver) -> decltype(receiver(std::declval<const CharType*>(), std::declval<const CharType*>()))
		{
			std::basic_string<CharType> str{begin, end};
			auto ptr = &str[0];
			return receiver(ptr, ptr + str.size());
		}

		template <class Iterator, class BaseCharType, class Receiver>
		auto narrow(Iterator begin, Iterator end, const std::regex_traits<BaseCharType> &traits, Receiver receiver) const -> decltype(receiver(std::declval<const BaseCharType*>(), std::declval<const BaseCharType*>()))
		{
			return make_contiguous(begin, end, [&](const CharType *begin, const CharType *end)
			{
				auto size = end - begin;

				if(size < 32)
				{
					BaseCharType *ptr = static_cast<BaseCharType*>(alloca(size * sizeof(BaseCharType)));
					base_int_ctype->narrow(begin, end, BaseCharType(), ptr);
					return receiver(ptr, ptr + size);
				}else{
					auto memory = std::make_unique<BaseCharType[]>(size);
					auto ptr = memory.get();
					base_int_ctype->narrow(begin, end, BaseCharType(), ptr);
					return receiver(ptr, ptr + size);
				}
			});
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
			if('0' <= ch && ch <= '9')
			{
				int result = ch - '0';
				return result < base ? result : -1;
			}

			if(base == 16)
			{
				if('a' <= ch && ch <= 'f')
				{
					return ch - 'a' + 10;
				}

				if('A' <= ch && ch <= 'F')
				{
					return ch - 'A' + 10;
				}
			}

			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				narrow_char_type n = base_int_ctype->narrow(ch, narrow_char_type{});
				if(n != narrow_char_type{})
				{
					return traits.value(ch, base);
				}
				return -1;
			});
		}

		static size_type length(const CharType *str)
		{
			return std::char_traits<CharType>::length(str);
		}

		CharType translate(CharType ch) const
		{
			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				narrow_char_type n = base_int_ctype->narrow(ch, narrow_char_type{});
				if(n != narrow_char_type{})
				{
					return base_int_ctype->widen(traits.translate(ch));
				}
				return ch;
			});
		}

		CharType translate_nocase(CharType ch) const
		{
			return translate(base_int_ctype->tolower(ch));
		}

		template <class Iterator>
		string_type transform(Iterator first, Iterator last) const
		{
			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				return narrow(first, last, traits, [&](const narrow_char_type *begin, const narrow_char_type *end)
				{
					return get_str(traits.transform(begin, end));
				});
			});
		}

		template <class Iterator>
		string_type transform_primary(Iterator first, Iterator last) const
		{
			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				return narrow(first, last, traits, [&](const narrow_char_type *begin, const narrow_char_type *end)
				{
					return get_str(traits.transform_primary(begin, end));
				});
			});
		}

		bool isctype(CharType ch, char_class_type ctype) const
		{
			if(ctype != static_cast<char_class_type>(-1))
			{
				return base_int_ctype->is(ctype, ch);
			}else{
				return ch == '_' || base_int_ctype->is(std::ctype_base::alnum, ch);
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
				{get_str("xdigit"), std::ctype_base::xdigit},
				{get_str("d"), std::ctype_base::digit},
				{get_str("s"), std::ctype_base::space},
				{get_str("w"), static_cast<char_class_type>(-1)}
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
			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				return narrow(first, last, traits, [&](const narrow_char_type *begin, const narrow_char_type *end)
				{
					return traits.lookup_classname(begin, end, icase);
				});
			});
		}

		template <class Iterator>
		string_type lookup_collatename(Iterator first, Iterator last) const
		{
			return get_traits([&](const auto &traits)
			{
				using narrow_char_type = traits_char_type<decltype(traits)>;

				return narrow(first, last, traits, [&](const narrow_char_type *begin, const narrow_char_type *end)
				{
					return get_str(traits.lookup_collatename(begin, end));
				});
			});
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
