#ifndef REGEX_STD_H_INCLUDED
#define REGEX_STD_H_INCLUDED

#include "regex.h"
#include <regex>
#include <locale>

typedef std::basic_regex<cell> cell_regex;

namespace
{
	const char *get_error(std::regex_constants::error_type code)
	{
		static std::unordered_map<std::regex_constants::error_type, const char *> map{
#ifdef _MSC_VER
			{std::regex_constants::error_parse, "parse"},
			{std::regex_constants::error_syntax, "syntax"},
#endif
			{std::regex_constants::error_collate, "collate"},
			{std::regex_constants::error_ctype, "ctype"},
			{std::regex_constants::error_escape, "escape"},
			{std::regex_constants::error_backref, "backref"},
			{std::regex_constants::error_brack, "brack"},
			{std::regex_constants::error_paren, "paren"},
			{std::regex_constants::error_brace, "brace"},
			{std::regex_constants::error_badbrace, "badbrace"},
			{std::regex_constants::error_range, "range"},
			{std::regex_constants::error_space, "space"},
			{std::regex_constants::error_badrepeat, "badrepeat"},
			{std::regex_constants::error_complexity, "complexity"},
			{std::regex_constants::error_stack, "stack"}
		};
		auto it = map.find(code);
		if(it != map.end())
		{
			return it->second;
		}
		return "unknown";
	}

	template <class Iter, typename std::iterator_traits<Iter>::value_type Backslash = '\\', typename std::iterator_traits<Iter>::value_type Percent = '%'>
	class percent_escaped
	{
		Iter inner;
		mutable enum : char {
			unobserved,
			at_percent,
			at_backslash,
			after_percent,
			after_backslash,
			normal
		} state;

	private:
		using deref_type = typename std::remove_reference<typename std::iterator_traits<Iter>::reference>::type;

		void check_state() const
		{
			switch(state)
			{
				case unobserved:
					switch(*inner)
					{
						case Backslash:
							state = at_backslash;
							break;
						case Percent:
							state = at_percent;
							break;
						default:
							state = normal;
							break;
					}
					break;
				case after_percent:
					state = normal;
					break;
			}
		}

	public:
		using value_type = typename std::iterator_traits<Iter>::value_type;
		using reference = const deref_type&;
		using pointer = const deref_type*;
		using difference_type = typename std::iterator_traits<Iter>::difference_type;

		percent_escaped(Iter inner) : inner(inner), state(unobserved)
		{

		}

		reference operator*() const
		{
			check_state();
			if(state == at_percent)
			{
				static const deref_type backslashRef = Backslash;
				return backslashRef;
			}
			return *inner;
		}

		pointer operator->() const
		{
			return &this->operator*();
		}

		percent_escaped& operator++()
		{
			check_state();
			switch(state)
			{
				case at_backslash:
					state = after_backslash;
					break;
				case at_percent:
					++inner;
					state = after_percent;
					break;
				default:
					++inner;
					state = unobserved;
					break;
			}
			return *this;
		}

		percent_escaped operator++(int)
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}

		bool operator==(const percent_escaped &it) const
		{
			return inner == it.inner && ((state == after_backslash) == (it.state == after_backslash));
		}

		bool operator!=(const percent_escaped &it) const
		{
			return !(*this == it);
		}
	};

	namespace std
	{
		template <class Iter, typename std::iterator_traits<Iter>::value_type Backslash, typename std::iterator_traits<Iter>::value_type Percent>
		struct iterator_traits<percent_escaped<Iter, Backslash, Percent>>
		{
			using reference = typename percent_escaped<Iter, Backslash, Percent>::reference;
			using pointer = typename percent_escaped<Iter, Backslash, Percent>::pointer;
			using value_type = typename percent_escaped<Iter, Backslash, Percent>::value_type;
			using difference_type = typename percent_escaped<Iter, Backslash, Percent>::difference_type;
			using iterator_category =  std::input_iterator_tag;
		};
	}

	template <class Iter>
	void configure_regex(cell_regex &regex, Iter pattern_begin, Iter pattern_end, Iter orig_begin, const cell_string *pattern, std::regex_constants::syntax_option_type syntax_options, bool use_percent_escaped)
	{
		if(use_percent_escaped)
		{
			std::string msg;
			for(const auto &c : cell_string(percent_escaped<Iter>(pattern_begin), percent_escaped<Iter>(pattern_end)))
			{
				msg.append(1, static_cast<unsigned char>(c));
			}
			logprintf("%s", msg.c_str());
			regex.assign(percent_escaped<Iter>(pattern_begin), percent_escaped<Iter>(pattern_end), syntax_options);
		}else if(pattern != nullptr && pattern_begin == orig_begin)
		{
			// construct from string if available and equivalent
			regex.assign(*pattern, syntax_options);
		}else{
			// construct from range
			regex.assign(pattern_begin, pattern_end, syntax_options);
		}
	}

	template <class Iter>
	cell_regex construct_regex(Iter pattern_begin, Iter pattern_end, Iter orig_begin, const cell_string *pattern, std::regex_constants::syntax_option_type syntax_options, bool use_percent_escaped)
	{
		if(use_percent_escaped)
		{
			return cell_regex(percent_escaped<Iter>(pattern_begin), percent_escaped<Iter>(pattern_end), syntax_options);
		}else if(pattern != nullptr && pattern_begin == orig_begin)
		{
			// construct from string if available and equivalent
			return cell_regex(*pattern, syntax_options);
		}else{
			// construct from range
			return cell_regex(pattern_begin, pattern_end, syntax_options);
		}
	}

	class regex_cached
	{
		cell_regex regex;
		bool depends_on_global_locale = false;
		std::locale global_locale;

	protected:
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

		template <class Iter, class... Args>
		void init(bool reimbue, Iter pattern_begin, Iter pattern_end, Args&&... args)
		{
			Iter orig_begin = pattern_begin;
			encoding enc = parse_encoding_override(pattern_begin, pattern_end);
			if(reimbue || enc.is_modified())
			{
				// has custom locale or modified properties, or default locale changed
				regex.imbue(enc.install());
			}

			depends_on_global_locale = enc.locale == global_locale;

			configure_regex(regex, pattern_begin, pattern_end, orig_begin, std::forward<Args>(args)...);
		}

	public:
		const cell_regex &get_regex() const
		{
			return regex;
		}
	};

	class regex_cached_value : public regex_cached
	{
		bool initialized = false;

	public:
		template <class... Args>
		bool update(Args&&... args)
		{
			if(!initialized)
			{
				regex_cached::init(false, std::forward<Args>(args)...);
				initialized = true;
				return true;
			}
			return regex_cached::update(std::forward<Args>(args)...);
		}
	};

	std::unordered_map<std::pair<cell_string, cell>, regex_cached_value> regex_cache;

	template <class Iter, class... KeyArgs>
	const cell_regex &get_cached_key(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options, KeyArgs&&... keyArgs)
	{
		auto result = regex_cache.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(
				std::piecewise_construct,
				std::forward_as_tuple(std::forward<KeyArgs>(keyArgs)...),
				std::forward_as_tuple(options & 255)
			),
			std::forward_as_tuple()
		).first;
		auto &cached = result->second;
		try{
			cached.update(pattern_begin, pattern_end, pattern, syntax_options, options & percent_escaped_flag);
		}catch(...)
		{
			regex_cache.erase(result);
			throw;
		}
		return cached.get_regex();
	}

	template <class Iter>
	const cell_regex &get_cached(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
	{
		if(pattern == nullptr)
		{
			// key from range
			return get_cached_key(pattern_begin, pattern_end, pattern, options, syntax_options, pattern_begin, pattern_end);
		}else{
			// key from string
			return get_cached_key(pattern_begin, pattern_end, pattern, options, syntax_options, *pattern);
		}
	}

	class regex_cached_addr : public regex_cached
	{
		std::weak_ptr<void> mem_handle;

	public:
		template <class... Args>
		bool update(std::weak_ptr<void> &&mem_handle, Args&&... args)
		{
			if(this->mem_handle.owner_before(mem_handle) || mem_handle.owner_before(this->mem_handle))
			{
				// different address
				this->mem_handle = std::move(mem_handle);
				// try updating the global locale first
				if(!regex_cached::update(std::forward<Args>(args)...))
				{
					// but init anyway if locales match
					init(false, std::forward<Args>(args)...);
				}
				return true;
			}
			if(!regex_cached::update(std::forward<Args>(args)...))
			{
				return false;
			}
			this->mem_handle = std::move(mem_handle);
			return true;
		}
	};

	template <class Iter>
	const cell_regex &get_cached_addr(Iter pattern_begin, Iter pattern_end, cell options, std::regex_constants::syntax_option_type syntax_options, std::weak_ptr<void> &&mem_handle)
	{
		static std::unordered_map<std::tuple<std::intptr_t, std::size_t, cell>, regex_cached_addr> regex_cache;
		auto result = regex_cache.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(
				reinterpret_cast<std::intptr_t>(&*pattern_begin),
				pattern_end - pattern_begin,
				options & 255
			),
			std::forward_as_tuple()
		).first;
		auto &cached = result->second;
		try{
			cached.update(std::move(mem_handle), pattern_begin, pattern_end, nullptr, syntax_options, options & percent_escaped_flag);
		}catch(...)
		{
			regex_cache.erase(result);
			throw;
		}
		return cached.get_regex();
	}

	template <class Iter, class... Args>
	cell_regex create_regex(Iter pattern_begin, Iter pattern_end, Args&&... args)
	{
		Iter orig_begin = pattern_begin;
		encoding enc = parse_encoding_override(pattern_begin, pattern_end);
		if(enc.is_modified())
		{
			// has custom locale or modified properties
			cell_regex regex;
			regex.imbue(enc.install());
			configure_regex(regex, pattern_begin, pattern_end, orig_begin, std::forward<Args>(args)...);
			return regex;
		}
		return construct_regex(pattern_begin, pattern_end, orig_begin, std::forward<Args>(args)...);
	}

	template <class Iter>
	struct match_state
	{
		using iterator = Iter;
		using match_type = std::match_results<Iter>;
		using match_iterator = typename match_type::const_iterator;

		const cell_regex &regex;
		std::regex_constants::match_flag_type options;
		Iter begin;
		const Iter end;
		const cell esc_char;
		match_type &match;

		match_state(const cell_regex &regex,
			std::regex_constants::match_flag_type options,
			bool percent_escaped,
			Iter begin,
			Iter end,
			match_type &match) : regex(regex), options(options), esc_char(percent_escaped ? '%' : '\\'), begin(begin), end(end), match(match)
		{

		}

		bool search() const
		{
			return std::regex_search(begin, end, match, regex, options);
		}

		cell escape_char() const noexcept
		{
			return esc_char;
		}
	};
}

#endif
