#include "regex.h"
#include "errors.h"
#include "modules/expressions.h"
#include "objects/stored_param.h"

#include <regex>

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

struct regex_traits
{
	typedef cell char_type;
	typedef cell_string string_type;
	typedef size_t size_type;
	typedef std::locale locale_type;
	typedef std::ctype_base::mask char_class_type;

	typedef ucell _Uelem;
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

	regex_traits()
	{
		cache_locale();
	}

	int value(cell ch, int base) const
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

	static size_type length(const cell *str)
	{
		return std::char_traits<cell>::length(str);
	}

	cell translate(cell ch) const
	{
		return ch;
	}

	cell translate_nocase(cell ch) const
	{
		return to_lower(ch);
	}

	template<class Iterator>
	string_type transform(Iterator first, Iterator last) const
	{
		return string_type(first, last);
	}

	template<class Iterator>
	string_type transform_primary(Iterator first, Iterator last) const
	{
		string_type res(first, last);
		std::transform(res.begin(), res.end(), res.begin(), to_lower);
		return res;
	}

	bool isctype(cell ch, std::ctype_base::mask ctype) const
	{
		if(ctype != static_cast<std::ctype_base::mask>(-1))
		{
			return _ctype->is(ctype, ch);
		}else{
			return ch == '_' || _ctype->is(std::ctype_base::alnum, ch);
		}
	}

	template<class Iterator>
	std::ctype_base::mask lookup_classname(Iterator first, Iterator last, bool icase = false) const
	{
		static std::unordered_map<cell_string, std::ctype_base::mask> map{
			{convert("alnum"), std::ctype_base::alnum},
			{convert("alpha"), std::ctype_base::alpha},
			{convert("cntrl"), std::ctype_base::cntrl},
			{convert("digit"), std::ctype_base::digit},
			{convert("graph"), std::ctype_base::graph},
			{convert("lower"), std::ctype_base::lower},
			{convert("print"), std::ctype_base::print},
			{convert("punct"), std::ctype_base::punct},
			{convert("space"), std::ctype_base::space},
			{convert("upper"), std::ctype_base::upper},
			{convert("xdigit"), std::ctype_base::xdigit}
		};

		cell_string key(first, last);
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

	template<class Iterator>
	string_type lookup_collatename(Iterator first, Iterator last) const
	{
		return string_type(first, last);
	}

	locale_type imbue(locale_type loc)
	{
		locale_type tmp = _locale;
		_locale = loc;
		cache_locale();
		return tmp;
	}

	locale_type getloc() const
	{
		return _locale;
	}

private:
	void cache_locale()
	{
		_ctype = &std::use_facet<std::ctype<cell>>(_locale);
	}

	const std::ctype<cell> *_ctype;
	locale_type _locale;
};


template<class BidirIt, class CharT = typename std::iterator_traits<BidirIt>::value_type, class Traits = std::regex_traits<CharT>>
static std::regex_iterator<BidirIt, CharT, Traits> make_regex_iterator(BidirIt a, BidirIt b, const std::basic_regex<CharT, Traits>& re, std::regex_constants::match_flag_type m = std::regex_constants::match_default)
{
	return {a, b, re, m};
}

constexpr const cell cache_flag = 4194304;
constexpr const cell cache_addr_flag = 4194304 | 8388608;

static void regex_options(cell options, std::regex_constants::syntax_option_type &syntax, std::regex_constants::match_flag_type &match)
{
	switch(options & 7)
	{
		default:
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

static std::unordered_map<std::pair<cell_string, cell>, std::basic_regex<cell, regex_traits>> regex_cache;

static const std::basic_regex<cell, regex_traits> &get_cached(const cell_string &pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(pattern, options), std::forward_as_tuple(pattern, syntax_options)).first->second;
}

template <class Iter>
static const std::basic_regex<cell, regex_traits> &get_cached(Iter pattern_begin, Iter pattern_end, const cell_string *pattern, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	if(pattern != nullptr)
	{
		return get_cached(*pattern, options, syntax_options);
	}
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(cell_string(pattern_begin, pattern_end), options), std::forward_as_tuple(pattern_begin, pattern_end, syntax_options)).first->second;
}

template <class Iter>
static const std::basic_regex<cell, regex_traits> &get_cached_addr(Iter pattern_begin, Iter pattern_end, cell options, std::regex_constants::syntax_option_type syntax_options)
{
	static std::unordered_map<std::tuple<const void*, const void*, cell>, std::basic_regex<cell, regex_traits>> regex_cache;
	options &= 255;
	return regex_cache.emplace(std::piecewise_construct, std::forward_as_tuple(&*pattern_begin, &*pattern_end, options), std::forward_as_tuple(pattern_begin, pattern_end, syntax_options)).first->second;
}

template <class Iter>
struct regex_search_base
{
	bool operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		std::match_results<cell_string::const_iterator> match;
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return false;
			}
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return false;
			}
		}
		*pos = match[0].second - str.cbegin();
		return true;
	}
};

bool strings::regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options)
{
	try{
		return select_iterator<regex_search_base>(pattern, str, nullptr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options)
{
	try{
		return regex_search_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

template <class Iter>
struct regex_extract_base
{
	cell operator()(Iter pattern_begin, Iter pattern_end, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		std::match_results<cell_string::const_iterator> match;
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return 0;
			}
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			if(!std::regex_search(begin, str.cend(), match, regex, match_options))
			{
				return 0;
			}
		}
		*pos = match[0].second - str.cbegin();
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

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options)
{
	try{
		return select_iterator<regex_extract_base>(pattern, str, nullptr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
		return 0;
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options)
{
	try{
		return regex_extract_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), str, &pattern, pos, options);
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
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, ReplacementIter replacement_begin, ReplacementIter replacement_end, std::regex_constants::match_flag_type match_options)
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
		void operator()(ReplacementIter replacement_begin, ReplacementIter replacement_end, PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, cell *pos, cell options) const
		{
			std::regex_constants::syntax_option_type syntax_options;
			std::regex_constants::match_flag_type match_options;
			regex_options(options, syntax_options, match_options);

			if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
			{
				amx_LogicError(errors::out_of_range, "pos");
			}else if(*pos > 0)
			{
				match_options |= std::regex_constants::match_prev_avail;
			}
			auto begin = str.cbegin() + *pos;
			target.append(str.cbegin(), begin);
			if(options & cache_flag)
			{
				const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
				replace(target, begin, str.cend(), regex, replacement_begin, replacement_end, match_options);
			}else{
				std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
				replace(target, begin, str.cend(), regex, replacement_begin, replacement_end, match_options);
			}
			*pos = begin - str.cbegin();
		}
	};

	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell *replacement, cell *pos, cell options) const
	{
		select_iterator<inner>(replacement, pattern_begin, pattern_end, target, str, nullptr, pos, options);
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell *pos, cell options)
{
	try{
		select_iterator<regex_replace_base>(pattern, target, str, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options)
{
	try{
		typename regex_replace_base<cell_string::const_iterator>::template inner<cell_string::const_iterator>()(replacement.begin(), replacement.end(), pattern.begin(), pattern.end(), target, str, &pattern, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, const list_t &replacement, std::regex_constants::match_flag_type match_options)
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
				cell_string str = repl.to_string();
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
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, const list_t &replacement, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		target.append(str.cbegin(), begin);
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			replace(target, begin, str.cend(), regex, replacement, match_options);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			replace(target, begin, str.cend(), regex, replacement, match_options);
		}
		*pos = begin - str.cbegin();
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const list_t &replacement, cell *pos, cell options)
{
	try{
		select_iterator<regex_replace_list_base>(pattern, target, str, nullptr, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options)
{
	try{
		regex_replace_list_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, str, &pattern, replacement, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, AMX *amx, int replacement_index, std::regex_constants::match_flag_type match_options, const char *format, cell *params, size_t numargs)
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
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		target.append(str.cbegin(), begin);
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			replace(target, begin, str.cend(), regex, amx, replacement_index, match_options, format, params, numargs);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			replace(target, begin, str.cend(), regex, amx, replacement_index, match_options, format, params, numargs);
		}
		*pos = begin - str.cbegin();
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs)
{
	try{
		select_iterator<regex_replace_func_base>(pattern, target, str, nullptr, amx, replacement_index, pos, options, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs)
{
	try{
		regex_replace_func_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, str, &pattern, amx, replacement_index, pos, options, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

template <class StringIter>
void replace(cell_string &target, StringIter &begin, StringIter end, const std::basic_regex<cell, regex_traits> &regex, AMX *amx, const expression &expr, std::regex_constants::match_flag_type match_options)
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
		auto result = expr.execute(ref_args, info).to_string();
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
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const cell_string &str, const cell_string *pattern, AMX *amx, const expression &expr, cell *pos, cell options) const
	{
		std::regex_constants::syntax_option_type syntax_options;
		std::regex_constants::match_flag_type match_options;
		regex_options(options, syntax_options, match_options);

		if(*pos < 0 || static_cast<ucell>(*pos) > str.size())
		{
			amx_LogicError(errors::out_of_range, "pos");
		}else if(*pos > 0)
		{
			match_options |= std::regex_constants::match_prev_avail;
		}
		auto begin = str.cbegin() + *pos;
		target.append(str.cbegin(), begin);
		if(options & cache_flag)
		{
			const std::basic_regex<cell, regex_traits> &regex = options & cache_addr_flag ? get_cached_addr(pattern_begin, pattern_end, options, syntax_options) : get_cached(pattern_begin, pattern_end, pattern, options, syntax_options);
			replace(target, begin, str.cend(), regex, amx, expr, match_options);
		}else{
			std::basic_regex<cell, regex_traits> regex(pattern_begin, pattern_end, syntax_options);
			replace(target, begin, str.cend(), regex, amx, expr, match_options);
		}
		*pos = begin - str.cbegin();
	}
};

void strings::regex_replace(cell_string &target, const cell_string &str, const cell *pattern, AMX *amx, const expression &expr, cell *pos, cell options)
{
	try{
		select_iterator<regex_replace_expr_base>(pattern, target, str, nullptr, amx, expr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, const expression &expr, cell *pos, cell options)
{
	try{
		regex_replace_expr_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, str, &pattern, amx, expr, pos, options);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}
}
