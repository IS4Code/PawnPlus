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

	template <typename PatCharT, typename StrCharT>
	struct lex_traits_state;

	template <typename PatCharT, PatCharT EscapeChar>
	struct lex_traits
	{
		using char_type = PatCharT;
		using locale_type = typename std::regex_traits<char_type>::locale_type;

		enum : char_type
		{
			escape_char = EscapeChar
		};

		template <typename StrCharT>
		using state = lex_traits_state<PatCharT, StrCharT>;

		std::regex_traits<char_type> regex_traits;
		std::regex_constants::syntax_option_type syntax_options;

		lex_traits() : syntax_options()
		{

		}

		lex_traits(std::regex_constants::syntax_option_type syntax_options) : syntax_options(syntax_options)
		{

		}

		lex_traits(const lex_traits &other) : syntax_options(other.syntax_options)
		{
			regex_traits.imbue(other.regex_traits.getloc());
		}

		void configure(std::regex_constants::syntax_option_type syntax_options)
		{
			this->syntax_options = syntax_options & std::regex_constants::icase;
		}

		lex_traits &operator=(const lex_traits &other)
		{
			regex_traits.imbue(other.regex_traits.getloc());
			syntax_options = other.syntax_options;
			return *this;
		}

		template <typename StrCharT>
		lex_traits_state<PatCharT, StrCharT> get_state() const &
		{
			return lex_traits_state<PatCharT, StrCharT>(*this);
		}

		locale_type imbue(locale_type loc)
		{
			return regex_traits.imbue(loc);
		}
	};
	
	template <typename PatCharT, typename StrCharT>
	struct lex_traits_state
	{
		using char_type = PatCharT;
		using str_char_type = StrCharT;
		using locale_type = typename std::regex_traits<char_type>::locale_type;
		using char_class_type = typename std::regex_traits<char_type>::char_class_type;

		static_assert(std::is_same<char_type, str_char_type>::value, "Comparing strings of different types!");

		const std::regex_traits<char_type> &regex_traits;
		const locale_type locale;
		const std::ctype<char_type> &ctype;
		const bool icase;
		const bool collate;

		template <PatCharT EscapeChar>
		lex_traits_state(const lex_traits<PatCharT, EscapeChar> &traits)
			: regex_traits(traits.regex_traits)
			, locale(regex_traits.getloc())
			, ctype(std::use_facet<std::ctype<char_type>>(locale))
			, icase(traits.syntax_options & std::regex_constants::icase)
			, collate(traits.syntax_options & std::regex_constants::collate)
		{

		}

		bool match_class(str_char_type c, char_type cl) const
		{
			bool neg = ctype.is(std::ctype_base::upper, cl);
			char_type cll = neg ? ctype.tolower(cl) : cl;
			char_class_type test = regex_traits.lookup_classname(&cll, &cll + 1, icase);
			if(test == char_class_type())
			{
				return c == cl;
			}
			return regex_traits.isctype(c, test) != neg;
		}

		bool match_range(str_char_type c, char_type min, char_type max) const
		{
			if(!collate)
			{
				if(min <= c && c <= max)
				{
					return true;
				}
				if(!icase)
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
				if(!icase)
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
			if(!collate)
			{
				if(!icase)
				{
					return c == d;
				}else{
					return ctype.tolower(c) == ctype.tolower(d);
				}
			}else{
				if(!icase)
				{
					return regex_traits.translate(c) == regex_traits.translate(d);
				}else{
					return regex_traits.translate_nocase(c) == regex_traits.translate_nocase(d);
				}
			}
		}
	};

	template <class Iter, class Traits>
	void configure_lex(lex::pattern_iter<Iter, Traits> &pattern, std::regex_constants::syntax_option_type syntax_options)
	{
		pattern.traits.configure(syntax_options);
	}

	template <class StrIter, class PatIter>
	using default_lex_match_state = lex_match_state<StrIter, PatIter, lex_traits<typename std::iterator_traits<PatIter>::value_type, '\\'>>&&;

	template <class Iter, class Traits>
	class lex_cached
	{
		lex::pattern_iter<Iter, Traits> pattern;
		bool depends_on_global_locale = false;
		std::locale global_locale;

	protected:
		void reset()
		{
			global_locale = std::locale();
			depends_on_global_locale = false;
		}

		template <class... Args>
		bool update(Args&&... args)
		{
			if(!depends_on_global_locale)
			{
				return false;
			}
			std::locale new_global;
			if(global_locale == new_global)
			{
				return false;
			}
			global_locale = std::move(new_global);
			init(true, std::forward<Args>(args)...);
			return true;
		}

		template <class... Args>
		void init(bool updating, Iter pattern_begin, Iter pattern_end, Args&&... args)
		{
			encoding enc = parse_encoding_override(pattern_begin, pattern_end);

			if(!updating)
			{
				pattern = lex::pattern_iter<Iter, Traits>(pattern_begin, pattern_end, std::forward<Args>(args)...);
			}else{
				configure_lex(pattern, std::forward<Args>(args)...);
			}

			if(updating || enc.is_modified())
			{
				// has custom locale or modified properties, or default locale changed
				pattern.imbue(enc.install());
			}

			depends_on_global_locale = enc.locale == global_locale;
		}

	public:
		const lex::pattern_iter<Iter, Traits> &get_pattern() const
		{
			return pattern;
		}
	};

	template <class Iter, class Traits>
	using lex_cached_value = cached_value<lex_cached<Iter, Traits>>;

	struct unique_cell_string : public std::unique_ptr<const cell_string>
	{
		template <class... Args>
		unique_cell_string(Args&&... args) : unique_ptr(std::make_unique<cell_string>(std::forward<Args>(args)...))
		{

		}
	};
}

namespace std
{
	template <>
	struct hash<unique_cell_string>
	{
		size_t operator()(const unique_cell_string &obj) const
		{
			return obj == nullptr ? 0 : std::hash<cell_string>()(*obj);
		}
	};
}

namespace
{
	template <class Iter, class Traits>
	static std::unordered_map<std::pair<unique_cell_string, cell>, lex_cached_value<Iter, Traits>> &lex_cache()
	{
		static std::unordered_map<std::pair<unique_cell_string, cell>, lex_cached_value<Iter, Traits>> data;
		return data;
	}
	
	template <class Traits, class... KeyArgs>
	const lex::pattern_iter<cell_string::const_iterator, Traits> &get_lex_cached_key(cell options, std::regex_constants::syntax_option_type syntax_options, KeyArgs&&... keyArgs)
	{
		auto &cache = lex_cache<cell_string::const_iterator, Traits>();
		auto result = cache.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(
				std::piecewise_construct,
				std::forward_as_tuple(std::forward<KeyArgs>(keyArgs)...),
				std::forward_as_tuple(options & pattern_mask)
			),
			std::forward_as_tuple()
		).first;
		const auto &key = result->first.first;
		auto &cached = result->second;
		try{
			cached.update(key->begin(), key->end(), syntax_options);
		}catch(...)
		{
			cache.erase(result);
			throw;
		}
		return cached.get_pattern();
	}

	template <class Iter, class Traits>
	const lex::pattern_iter<cell_string::const_iterator, Traits> &get_lex_cached(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
	{
		if(pattern == nullptr)
		{
			// key from range
			return get_lex_cached_key<Traits>(options, syntax_options, pattern_begin, pattern_end);
		}else{
			// key from string
			return get_lex_cached_key<Traits>(options, syntax_options, *pattern);
		}
	}

	template <class Iter, class Traits>
	using lex_cached_addr = cached_addr<lex_cached<Iter, Traits>>;

	template <class Iter, class Traits>
	const lex::pattern_iter<Iter, Traits> &get_lex_cached_addr(Iter pattern_begin, Iter pattern_end, cell options, std::regex_constants::syntax_option_type syntax_options, std::weak_ptr<void> &&mem_handle)
	{
		static std::unordered_map<std::tuple<std::intptr_t, std::size_t, cell>, lex_cached_addr<Iter, Traits>> cache;
		auto result = cache.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(
				reinterpret_cast<std::intptr_t>(&*pattern_begin),
				std::distance(pattern_begin, pattern_end),
				options & pattern_mask
			),
			std::forward_as_tuple()
		).first;
		auto &cached = result->second;
		try{
			cached.update(std::move(mem_handle), pattern_begin, pattern_end, syntax_options);
		}catch(...)
		{
			cache.erase(result);
			throw;
		}
		return cached.get_pattern();
	}

	template <class Iter, class Traits, class... Args>
	lex::pattern_iter<Iter, Traits> create_lex(Iter pattern_begin, Iter pattern_end, Args&&... args)
	{
		encoding enc = parse_encoding_override(pattern_begin, pattern_end);
		if(enc.is_modified())
		{
			// has custom locale or modified properties
			lex::pattern_iter<Iter, Traits> pattern(pattern_begin, pattern_end, std::forward<Args>(args)...);
			pattern.imbue(enc.install());
			return pattern;
		}
		return lex::pattern_iter<Iter, Traits>(pattern_begin, pattern_end, std::forward<Args>(args)...);
	}

	template <class StrIter, class PatIter, typename std::iterator_traits<PatIter>::value_type EscapeChar, class Receiver, class... Args>
	auto get_lex_traits_final(const regex_info &info, std::regex_constants::syntax_option_type syntax_options, Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		using traits = lex_traits<typename std::iterator_traits<PatIter>::value_type, EscapeChar>;

		if(info.options & cache_flag)
		{
			if(info.options & cache_addr_flag)
			{
				const lex::pattern_iter<PatIter, traits> &pattern = get_lex_cached_addr<PatIter, traits>(pattern_begin, pattern_end, info.options, syntax_options, std::move(info.mem_handle));
				return receiver(lex_match_state<StrIter, PatIter, traits>(pattern, std::forward<Args>(args)...));
			}else{
				const lex::pattern_iter<cell_string::const_iterator, traits> &pattern = get_lex_cached<PatIter, traits>(pattern_begin, pattern_end, info.pattern, info.options, syntax_options);
				return receiver(lex_match_state<StrIter, cell_string::const_iterator, traits>(pattern, std::forward<Args>(args)...));
			}
		}
		lex::pattern_iter<PatIter, traits> pattern = create_lex<PatIter, traits>(pattern_begin, pattern_end, syntax_options);
		return receiver(lex_match_state<StrIter, PatIter, traits>(pattern, std::forward<Args>(args)...));
	}

	template <class StrIter, class PatIter, class Receiver, class... Args>
	auto get_lex_traits_percent(const regex_info &info, std::regex_constants::syntax_option_type syntax_options, Receiver &&receiver, PatIter pattern_begin, PatIter pattern_end, Args&&... args) -> decltype(receiver(std::declval<default_lex_match_state<StrIter, PatIter>>()))
	{
		if(info.options & percent_escaped_flag)
		{
			return get_lex_traits_final<StrIter, PatIter, '%'>(info, syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}else{
			return get_lex_traits_final<StrIter, PatIter, '\\'>(info, syntax_options, std::move(receiver), pattern_begin, pattern_end, std::forward<Args>(args)...);
		}
	}

	template <class Iter, class Receiver>
	auto get_lex(Iter pattern_begin, Iter pattern_end, const regex_info &info, std::regex_constants::syntax_option_type syntax_options, std::regex_constants::match_flag_type match_options, Receiver &&receiver) -> decltype(receiver(std::declval<default_lex_match_state<strings::cell_string::const_iterator, Iter>>()))
	{
		using str_iterator = strings::cell_string::const_iterator;

		bool nosubs = syntax_options & std::regex_constants::nosubs;

		lex::basic_match_result_iter<str_iterator> match;

		return get_lex_traits_percent<str_iterator, Iter>(info, syntax_options, std::move(receiver), pattern_begin, pattern_end, match_options, nosubs, info.begin(), info.string.cend(), match);
	}
}

#endif
