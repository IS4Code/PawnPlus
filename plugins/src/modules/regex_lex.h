#ifndef REGEX_LEX_H_INCLUDED
#define REGEX_LEX_H_INCLUDED

#include "regex.h"
#include "lex/lex.h"
#include <regex>
#include <locale>

using namespace pg;

struct regex_info;

namespace
{
	template <class StrIter, class PatIter, class Traits>
	struct lex_match_state
	{
		using iterator = StrIter;
		using match_type = lex::basic_match_result_iter<StrIter>;
		using match_iterator = typename match_type::const_iterator;

		const lex::pattern_iter<PatIter, Traits> &regex;
		std::regex_constants::match_flag_type options;
		bool nosubs;
		StrIter begin;
		const StrIter end;
		match_type &match;

		lex_match_state(const lex::pattern_iter<PatIter, Traits> &regex,
			std::regex_constants::match_flag_type options,
			bool nosubs,
			StrIter begin,
			StrIter end,
			match_type &match) : regex(regex), options(options), nosubs(nosubs), begin(begin), end(end), match(match)
		{

		}

		bool search() const
		{
			if((options & std::regex_constants::match_not_null) && begin == end)
			{
				// reject empty
				return false;
			}

			if((options & std::regex_constants::match_not_bol) && regex.anchor)
			{
				// beginning rejected
				return false;
			}

			if((options & std::regex_constants::match_not_eol) && regex.begin != regex.end && *std::prev(regex.end) == '$')
			{
				// end rejected
				return false;
			}

			bool result = lex::search(
				begin, end, match, regex,
				options & std::regex_constants::match_continuous,
				options & std::regex_constants::match_prev_avail,
				options & std::regex_constants::match_not_bow,
				options & std::regex_constants::match_not_eow
			);
			if(result && nosubs)
			{
				match.truncate();
			}
			return result;
		}

		constexpr typename Traits::char_type escape_char() const noexcept
		{
			return Traits::escape_char;
		}
	};

	template <class Iter>
	using it_value_type = typename std::iterator_traits<Iter>::value_type;

	template <typename PatIter, it_value_type<PatIter> EscapeChar>
	struct lex_traits_base
	{
		using char_type = typename std::iterator_traits<PatIter>::value_type;
		using locale_type = typename std::regex_traits<char_type>::locale_type;
		using char_class_type = typename std::regex_traits<char_type>::char_class_type;

		enum : char_type
		{
			escape_char = EscapeChar
		};

		std::regex_traits<char_type> regex_traits;

		template <typename StrIter, bool Icase>
		struct state_base
		{
			using str_char_type = typename std::iterator_traits<StrIter>::value_type;

			static_assert(std::is_same<char_type, str_char_type>::value, "Comparing strings of different types!");

			const std::regex_traits<char_type> &regex_traits;
			const locale_type locale;
			const std::ctype<char_type> &ctype;

			state_base(const lex_traits_base &traits)
				: regex_traits(traits.regex_traits)
				, locale(regex_traits.getloc())
				, ctype(std::use_facet<std::ctype<char_type>>(locale))
			{

			}

			bool match_class(str_char_type c, char_type cl) const
			{
				bool neg = ctype.is(std::ctype_base::upper, cl);
				if(neg)
				{
					cl = ctype.tolower(cl);
				}
				char_class_type test = regex_traits.lookup_classname(&cl, &cl + 1, Icase);
				return regex_traits.isctype(c, test) != neg;
			}
		};

		locale_type imbue(locale_type loc)
		{
			return regex_traits.imbue(loc);
		}
	};

	template <typename PatIter, it_value_type<PatIter> EscapeChar, bool Icase, bool Collate>
	struct lex_traits : public lex_traits_base<PatIter, EscapeChar>
	{
		template <typename StrIter>
		struct state : public state_base<StrIter, Icase>
		{
			state(const lex_traits &traits) : state_base(traits)
			{

			}

			bool match_range(str_char_type c, char_type min, char_type max) const
			{
				if(!Collate)
				{
					if(min <= c && c <= max)
					{
						return true;
					}
					if(!Icase)
					{
						return false;
					}
					str_char_type c2 = ctype.tolower(c);
					if(min <= c2 && c2 <= max)
					{
						return true;
					}else if(c != c2)
					{
						// was upper already, toupper(c) should not do anything
						// unless toupper(tolower(c)) != toupper(c)
						return false;
					}
					c2 = ctype.toupper(c);
					return min <= c2 && c2 <= max;
				}else{
					std::basic_string<char_type> minkey = regex_traits.transform(&min, &min + 1);
					std::basic_string<char_type> maxkey = regex_traits.transform(&max, &max + 1);
					std::basic_string<str_char_type> ckey = regex_traits.transform(&c, &c + 1);
					if(minkey <= ckey && ckey <= maxkey)
					{
						return true;
					}
					if(!Icase)
					{
						return false;
					}
					str_char_type c2 = ctype.tolower(c);
					ckey = regex_traits.transform(&c2, &c2 + 1);
					if(minkey <= ckey && ckey <= maxkey)
					{
						return true;
					}else if(c != c2)
					{
						return false;
					}
					c2 = ctype.toupper(c);
					ckey = regex_traits.transform(&c2, &c2 + 1);
					return minkey <= ckey && ckey <= maxkey;
				}
			}

			bool match_char(str_char_type c, char_type d) const
			{
				if(!Collate)
				{
					if(!Icase)
					{
						return c == d;
					}else{
						return ctype.tolower(c) == ctype.tolower(d);
					}
				}else{
					if(!Icase)
					{
						return regex_traits.translate(c) == regex_traits.translate(d);
					}else{
						return regex_traits.translate_nocase(c) == regex_traits.translate_nocase(d);
					}
				}
			}
		};

		template <typename StrIter>
		state<StrIter> get_state() const &
		{
			return state<StrIter>(*this);
		}
	};

	template <class StrIter, class PatIter>
	using default_lex_match_state = lex_match_state<StrIter, PatIter, lex_traits<PatIter, '\\', false, false>>&&;

	template <class StrIter, class PatIter, it_value_type<PatIter> EscapeChar, bool Icase, bool Collate, class Receiver, class... Args>
	auto get_lex_traits_final(Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		using traits = lex_traits<PatIter, EscapeChar, Icase, Collate>;
		lex::pattern_iter<PatIter, traits> pattern(pattern_begin, pattern_end);
		return receiver(lex_match_state<StrIter, PatIter, traits>(pattern, std::forward<Args>(args)...));
	}

	template <class StrIter, class PatIter, it_value_type<PatIter> EscapeChar, bool Icase, class Receiver, class... Args>
	auto get_lex_traits_collate(std::regex_constants::syntax_option_type syntax_options, Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		if(syntax_options & std::regex_constants::collate)
		{
			return get_lex_traits_final<StrIter, PatIter, EscapeChar, Icase, true>(std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}else{
			return get_lex_traits_final<StrIter, PatIter, EscapeChar, Icase, false>(std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}
	}

	template <class StrIter, class PatIter, it_value_type<PatIter> EscapeChar, class Receiver, class... Args>
	auto get_lex_traits_icase(std::regex_constants::syntax_option_type syntax_options, Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		if(syntax_options & std::regex_constants::icase)
		{
			return get_lex_traits_collate<StrIter, PatIter, EscapeChar, true>(syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}else{
			return get_lex_traits_collate<StrIter, PatIter, EscapeChar, false>(syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}
	}

	template <class StrIter, class PatIter, class Receiver, class... Args>
	auto get_lex_traits_percent(bool percent_escaped, std::regex_constants::syntax_option_type syntax_options, Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		if(percent_escaped)
		{
			return get_lex_traits_icase<StrIter, PatIter, '%'>(syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}else{
			return get_lex_traits_icase<StrIter, PatIter, '\\'>(syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}
	}

	template <class Iter, class Receiver>
	auto get_lex(Iter pattern_begin, Iter pattern_end, const regex_info &info, std::regex_constants::syntax_option_type syntax_options, std::regex_constants::match_flag_type match_options, Receiver &&receiver) -> decltype(receiver(std::declval<default_lex_match_state<strings::cell_string::const_iterator, Iter>>()))
	{
		using str_iterator = strings::cell_string::const_iterator;

		bool nosubs = syntax_options & std::regex_constants::nosubs;

		lex::basic_match_result_iter<str_iterator> match;

		bool percent_escaped = info.options & percent_escaped_flag;
		return get_lex_traits_percent<str_iterator, Iter>(percent_escaped, syntax_options, std::move(receiver), pattern_begin, pattern_end, match_options, nosubs, info.begin(), info.string.cend(), match);
	}
}

#endif
