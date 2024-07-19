#include "regex.h"
#include "errors.h"
#include "modules/expressions.h"
#include "objects/stored_param.h"
#include "fixes/int_regex.h"

#include <regex>
#include <utility>

using namespace strings;

template <class T>
inline void hash_combine(size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct tuple_hash
{
	void operator()(size_t &seed, const Tuple &tuple) const
	{
		tuple_hash<Tuple, Index - 1>()(seed, tuple);
		hash_combine(seed, std::get<Index>(tuple));
	}
};

template <class Tuple>
struct tuple_hash<Tuple, 0>
{
	void operator()(size_t &seed, const Tuple &tuple) const
	{
		seed = std::hash<typename std::tuple_element<0, Tuple>::type>()(std::get<0>(tuple));
	}
};

namespace std
{
	template <>
	struct hash<std::regex_constants::error_type>
	{
		size_t operator()(const std::regex_constants::error_type &obj) const
		{
			return std::hash<size_t>()(obj);
		}
	};

	template <class Type1, class Type2>
	struct hash<std::pair<Type1, Type2>>
	{
		size_t operator()(const std::pair<Type1, Type2> &obj) const
		{
			size_t seed = std::hash<Type1>()(obj.first);
			hash_combine(seed, obj.second);
			return seed;
		}
	};

	template <class... Type>
	struct hash<std::tuple<Type...>>
	{
		size_t operator()(const std::tuple<Type...> &obj) const
		{
			size_t seed;
			tuple_hash<std::tuple<Type...>>()(seed, obj);
			return seed;
		}
	};
}

typedef std::basic_regex<cell> cell_regex;

constexpr const cell percent_escaped_flag = 128;
constexpr const cell no_prev_avail_flag = 32768;
constexpr const cell cache_flag = 4194304;
constexpr const cell cache_addr_flag = 8388608;

static void regex_options(cell options, std::regex_constants::syntax_option_type &syntax, std::regex_constants::match_flag_type &match)
{
	switch(options & 7)
	{
		case 0:
			syntax = std::regex_constants::ECMAScript;
			break;
		case 1:
			syntax = std::regex_constants::basic;
			break;
		case 2:
			syntax = std::regex_constants::extended;
			break;
		case 3:
			syntax = std::regex_constants::awk;
			break;
		case 4:
			syntax = std::regex_constants::grep;
			break;
		case 5:
			syntax = std::regex_constants::egrep;
			break;
		default:
			amx_FormalError(errors::out_of_range, "options");
			break;
	}
	options &= ~7;
	if(options & 8)
	{
		syntax |= std::regex_constants::icase;
	}
	if(options & 16)
	{
		syntax |= std::regex_constants::nosubs;
	}
	if(options & 32)
	{
		syntax |= std::regex_constants::optimize;
	}
	if(options & 64)
	{
		syntax |= std::regex_constants::collate;
	}

	match = {};
	if(options & 256)
	{
		match |= std::regex_constants::match_not_bol;
	}
	if(options & 512)
	{
		match |= std::regex_constants::match_not_eol;
	}
	if(options & 1024)
	{
		match |= std::regex_constants::match_not_bow;
	}
	if(options & 2048)
	{
		match |= std::regex_constants::match_not_eow;
	}
	if(options & 4096)
	{
		match |= std::regex_constants::match_any;
	}
	if(options & 8192)
	{
		match |= std::regex_constants::match_not_null;
	}
	if(options & 16384)
	{
		match |= std::regex_constants::match_continuous;
	}
	if(options & 65536)
	{
		match |= std::regex_constants::format_no_copy;
	}
	if(options & 131072)
	{
		match |= std::regex_constants::format_first_only;
	}
}

static const char *get_error(std::regex_constants::error_type code)
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

template <class Iter>
encoding parse_encoding_override(Iter &begin, Iter end)
{
	Iter it = begin, start;
	enum {
		at_start,
		found_parenthesis,
		found_question_mark,
		found_dollar,
		encoding_loop
	} state = at_start;
	while(it != end)
	{
		switch(state)
		{
			case at_start:
				if(*it != '(')
				{
					break;
				}
				++it;
				state = found_parenthesis;
				continue;
			case found_parenthesis:
				if(*it != '?')
				{
					break;
				}
				++it;
				state = found_question_mark;
				continue;
			case found_question_mark:
				if(*it != '$')
				{
					break;
				}
				++it;
				state = found_dollar;
				continue;
			case found_dollar:
				start = it;
				state = encoding_loop;
				goto loop;
			case encoding_loop:
			loop:
			{
				if(*it != ')')
				{
					++it;
					continue;
				}
				try{
					auto size = std::distance(start, it);
					encoding enc = aux::alloc_inline<char>(size + 1, [&](char *spec)
					{
						std::copy(start, it, reinterpret_cast<unsigned char*>(spec));
						spec[size] = '\0';
						return find_encoding(spec, false);
					});
					++it;
					begin = it;
					return enc;
				}catch(const std::runtime_error&)
				{
					break;
				}
			}
		}
		break;
	}
	char *spec = nullptr;
	return find_encoding(spec, false);
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

static std::unordered_map<std::pair<cell_string, cell>, regex_cached_value> regex_cache;

template <class Iter, class... KeyArgs>
static const cell_regex &get_cached_key(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options, KeyArgs&&... keyArgs)
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
static const cell_regex &get_cached(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
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
static const cell_regex &get_cached_addr(Iter pattern_begin, Iter pattern_end, cell options, std::regex_constants::syntax_option_type syntax_options, std::weak_ptr<void> &&mem_handle)
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

struct regex_info
{
	const cell_string &string;
	const cell_string *pattern;
	cell *pos;
	cell options;
	std::weak_ptr<void> &&mem_handle;

	cell_string::const_iterator begin() const
	{
		return string.cbegin() + *pos;
	}

	void found(cell_string::const_iterator it) const
	{
		*pos = it - string.cbegin();
	}
};

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

template <class Iter, class Receiver>
auto get_regex(Iter pattern_begin, Iter pattern_end, const regex_info &info, Receiver receiver) -> decltype(receiver(std::declval<cell_regex>(), std::declval<std::regex_constants::match_flag_type>(), std::declval<cell_string::const_iterator>(), std::declval<cell_string::const_iterator>()))
{
	std::regex_constants::syntax_option_type syntax_options;
	std::regex_constants::match_flag_type match_options;
	regex_options(info.options, syntax_options, match_options);

	if(*info.pos < 0 || static_cast<ucell>(*info.pos) > info.string.size())
	{
		amx_LogicError(errors::out_of_range, "pos");
	}else if(*info.pos > 0 && !(info.options & no_prev_avail_flag))
	{
		match_options |= std::regex_constants::match_prev_avail;
	}
	if(info.options & cache_flag)
	{
		const cell_regex &regex = info.options & cache_addr_flag
			? get_cached_addr(pattern_begin, pattern_end, info.options, syntax_options, std::move(info.mem_handle))
			: get_cached(pattern_begin, pattern_end, info.pattern, info.options, syntax_options);
		return receiver(regex, match_options, info.begin(), info.string.cend());
	}
	cell_regex regex = create_regex(pattern_begin, pattern_end, info.pattern, syntax_options, info.options & percent_escaped_flag);
	return receiver(regex, match_options, info.begin(), info.string.cend());
}

template <class Iter>
struct regex_search_base
{
	bool operator()(Iter pattern_begin, Iter pattern_end, const regex_info &info) const
	{
		std::match_results<cell_string::const_iterator> match;
		if(!get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
		{
			return std::regex_search(begin, end, match, regex, match_options);
		}))
		{
			return false;
		}
		info.found(match[0].second);
		return true;
	}
};

bool strings::regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return select_iterator<regex_search_base>(pattern, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return regex_search_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class Iter>
struct regex_extract_base
{
	cell operator()(Iter pattern_begin, Iter pattern_end, const regex_info &info) const
	{
		std::match_results<cell_string::const_iterator> match;
		if(!get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
		{
			return std::regex_search(begin, end, match, regex, match_options);
		}))
		{
			return 0;
		}
		info.found(match[0].second);
		tag_ptr chartag = tags::find_tag(tags::tag_char);
		auto list = list_pool.add();
		for(auto &group : match)
		{
			dyn_object obj(&*group.first, group.length() + 1, chartag);
			*(obj.end() - 1) = 0;
			list->push_back(std::move(obj));
		}
		return list_pool.get_id(list);
	}
};

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return select_iterator<regex_extract_base>(pattern, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return regex_extract_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class SubIter>
struct replace_sub_match_base
{
	template <class Iter>
	struct inner
	{
		void operator()(Iter begin, Iter end, cell_string &target, SubIter sbegin, SubIter send) const
		{
			auto last = begin;
			while(begin != end)
			{
				if(*begin == '$' || *begin == '\\')
				{
					auto c = *begin;
					auto spec = begin;
					++begin;
					if(begin != end)
					{
						if(*begin == c)
						{
							target.append(last, begin);
							last = ++begin;
							continue;
						}else if(*begin >= '1' && *begin <= '9')
						{
							size_t num = *begin - '1';
							target.append(last, spec);
							auto siter = sbegin;
							while(num > 0)
							{
								if(siter != send)
								{
									++siter;
									num--;
								}else{
									break;
								}
							}
							if(siter != send)
							{
								target.append(siter->first, siter->second);
							}
							last = ++begin;
							continue;
						}
					}
				}else{
					++begin;
				}
			}
			target.append(last, begin);
		}
	};
};

template <class StringIter, class ReplacementIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const cell_regex &regex, ReplacementIter replacement_begin, ReplacementIter replacement_end, std::regex_constants::match_flag_type match_options)
{
	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);
		typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<ReplacementIter>()(replacement_begin, replacement_end, target, std::next(result.cbegin()), result.cend());
		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_base
{
	template <class ReplacementIter>
	struct inner
	{
		void operator()(ReplacementIter replacement_begin, ReplacementIter replacement_end, PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const regex_info &info) const
		{
			get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
			{
				target.append(info.string.cbegin(), begin);
				replace(target, begin, end, regex, replacement_begin, replacement_end, match_options);
				info.found(begin);
				return nullptr;
			});
		}
	};

	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell *replacement, const regex_info &info) const
	{
		select_iterator<inner>(replacement, pattern_begin, pattern_end, target, info);
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		select_iterator<regex_replace_base>(pattern, target, replacement, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		typename regex_replace_base<cell_string::const_iterator>::template inner<cell_string::const_iterator>()(replacement.begin(), replacement.end(), pattern.begin(), pattern.end(), target, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const cell_regex &regex, const list_t &replacement, std::regex_constants::match_flag_type match_options)
{
	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);
		size_t index = 0;
		for(auto it = std::next(result.cbegin()); it != result.cend();)
		{
			index++;
			const auto &capture = *it;
			if(capture.matched && index <= replacement.size())
			{
				const dyn_object &repl = replacement[index - 1];

				auto begin = it;
				++it;
				auto end = std::find_if(it, result.cend(), [](const std::sub_match<StringIter> &match) {return !match.matched; });
				it = end;

				if(repl.get_tag()->inherits_from(tags::tag_string))
				{
					cell value = repl.get_cell(0);
					cell_string *repl_str;
					if(strings::pool.get_by_id(value, repl_str))
					{
						typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<cell_string::const_iterator>()(repl_str->cbegin(), repl_str->cend(), target, begin, end);
						continue;
					}
				}else if(repl.get_tag()->inherits_from(tags::tag_char) && repl.is_array())
				{
					select_iterator<replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner>(repl.begin(), target, begin, end);
					continue;
				}
				cell_string str = repl.to_string(strings::default_encoding());
				typename replace_sub_match_base<typename std::match_results<StringIter>::const_iterator>::template inner<cell_string::const_iterator>()(str.cbegin(), str.cend(), target, begin, end);
			}else{
				++it;
			}
		}
		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_list_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const list_t &replacement, const regex_info &info) const
	{
		get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
		{
			target.append(info.string.cbegin(), begin);
			replace(target, begin, end, regex, replacement, match_options);
			info.found(begin);
			return nullptr;
		});
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const list_t &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		select_iterator<regex_replace_list_base>(pattern, target, replacement, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_list_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, replacement, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const cell_regex &regex, AMX *amx, int replacement_index, std::regex_constants::match_flag_type match_options, const char *format, cell *params, size_t numargs)
{
	std::vector<stored_param> arg_values;
	if(format != nullptr)
	{
		size_t argi = -1;
		size_t len = std::strlen(format);
		for(size_t i = 0; i < len; i++)
		{
			arg_values.push_back(stored_param::create(amx, format[i], params, argi, numargs));
		}
	}

	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		const auto &group = result[0];
		target.append(begin, group.first);

		amx::guard guard(amx);

		for(int i = result.size() - 1; i >= 0; i--)
		{
			const auto &capture = result[i];

			amx_Push(amx, capture.length());

			cell val, *addr;
			if(capture.matched)
			{
				auto len = capture.length();
				amx_AllotSafe(amx, len + 1, &val, &addr);
				std::copy(capture.first, capture.second, addr);
				addr[len] = 0;
			}else{
				amx_AllotSafe(amx, 1, &val, &addr);
				*addr = 0;
			}
			amx_Push(amx, val);
		}

		for(auto it = arg_values.rbegin(); it != arg_values.rend(); it++)
		{
			it->push(amx, 0);
		}

		cell retval = 0;
		if(amx_Exec(amx, &retval, replacement_index) != AMX_ERR_NONE)
		{
			return;
		}
		if(retval != 0)
		{
			cell_string *repl;
			if(!strings::pool.get_by_id(retval, repl))
			{
				amx_LogicError(errors::pointer_invalid, "string", retval);
				return;
			}
			target.append(repl->cbegin(), repl->cend());
		}

		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_func_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, AMX *amx, int replacement_index, const regex_info &info, const char *format, cell *params, size_t numargs) const
	{
		get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
		{
			target.append(info.string.cbegin(), begin);
			replace(target, begin, end, regex, amx, replacement_index, match_options, format, params, numargs);
			info.found(begin);
			return nullptr;
		});
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs, std::weak_ptr<void> mem_handle)
{
	try{
		select_iterator<regex_replace_func_base>(pattern, target, amx, replacement_index, regex_info{str, nullptr, pos, options, std::move(mem_handle)}, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_func_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, amx, replacement_index, regex_info{str, &pattern, pos, options, std::move(mem_handle)}, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const cell_regex &regex, AMX *amx, const expression &expr, std::regex_constants::match_flag_type match_options)
{
	expression::exec_info info(amx);

	std::vector<dyn_object> args;
	args.resize(regex.mark_count() + 1);
	expression::args_type ref_args;
	for(const auto &arg : args)
	{
		ref_args.push_back(std::cref(arg));
	}

	std::match_results<StringIter> result;
	while(std::regex_search(begin, end, result, regex, match_options))
	{
		auto group = result[0];
		target.append(begin, group.first);

		for(size_t i = 0; i < result.size(); i++)
		{
			const auto &capture = result[i];
			if(capture.matched)
			{
				auto len = capture.length();
				args[i] = dyn_object(nullptr, len + 1, tags::find_tag(tags::tag_char));
				auto addr = args[i].begin();
				std::copy(capture.first, capture.second, addr);
				addr[len] = 0;
			}else{
				args[i] = dyn_object();
			}
		}

		amx::guard guard(amx);
		auto result = expr.execute(ref_args, info).to_string(strings::default_encoding());
		target.append(result.cbegin(), result.cend());

		if(group.second != begin)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		begin = group.second;
	}
	target.append(begin, end);
}

template <class PatternIter>
struct regex_replace_expr_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, AMX *amx, const expression &expr, const regex_info &info) const
	{
		get_regex(pattern_begin, pattern_end, info, [&](const cell_regex &regex, std::regex_constants::match_flag_type match_options, cell_string::const_iterator begin, cell_string::const_iterator end)
		{
			target.append(info.string.cbegin(), begin);
			replace(target, begin, end, regex, amx, expr, match_options);
			info.found(begin);
			return nullptr;
		});
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, const expression &expr, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		select_iterator<regex_replace_expr_base>(pattern, target, amx, expr, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, const expression &expr, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_expr_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, amx, expr, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

cell_string strings::collate_transform(const cell_string &str, bool primary, const encoding &enc)
{
	std::regex_traits<cell> traits;
	traits.imbue(enc.install());

	return primary
		? traits.transform_primary(str.begin(), str.end())
		: traits.transform(str.begin(), str.end());
}
