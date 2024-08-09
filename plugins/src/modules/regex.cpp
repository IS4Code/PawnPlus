#include "regex.h"
#include "errors.h"
#include "format.h"
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

constexpr const cell syntax_mask = 7;
constexpr const cell pattern_mask = 255;
constexpr const cell percent_escaped_flag = 128;
constexpr const cell no_prev_avail_flag = 32768;
constexpr const cell cache_flag = 4194304;
constexpr const cell cache_addr_flag = 8388608;
constexpr const cell lex_syntax = 6;

template <class Base>
class cached_value : public Base
{
	bool initialized = false;

public:
	template <class... Args>
	bool update(Args&&... args)
	{
		if(!initialized)
		{
			Base::init(false, std::forward<Args>(args)...);
			initialized = true;
			return true;
		}
		return Base::update(std::forward<Args>(args)...);
	}
};

template <class Base>
class cached_addr : public Base
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
			// reset to initial state and re-init
			Base::reset();
			Base::init(false, std::forward<Args>(args)...);
			return true;
		}
		if(!Base::update(std::forward<Args>(args)...))
		{
			return false;
		}
		this->mem_handle = std::move(mem_handle);
		return true;
	}
};

template <class Iter>
encoding parse_encoding_override(Iter &begin, Iter end);

#include "regex_std.h"
#include "regex_lex.h"

static void regex_options(cell options, std::regex_constants::syntax_option_type &syntax, std::regex_constants::match_flag_type &match)
{
	switch(options & syntax_mask)
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
		case lex_syntax:
			syntax = {};
			break;
		default:
			amx_FormalError(errors::out_of_range, "options");
			break;
	}
	options &= ~syntax_mask;
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

template <class Iter, class Receiver>
auto get_regex_state(Iter pattern_begin, Iter pattern_end, const regex_info &info, Receiver receiver) -> decltype(receiver(std::declval<match_state<cell_string::const_iterator>&&>()))
{
	using str_iterator = cell_string::const_iterator;

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
	if((info.options & syntax_mask) == lex_syntax)
	{
		return get_lex(pattern_begin, pattern_end, info, syntax_options, match_options, std::move(receiver));
	}else{
		return get_regex(pattern_begin, pattern_end, info, syntax_options, match_options, std::move(receiver));
	}
}

template <class Iter>
struct regex_search_base
{
	bool operator()(Iter pattern_begin, Iter pattern_end, const regex_info &info) const
	{
		return get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
		{
			if(!state.search())
			{
				return false;
			}
			info.found(state.match[0].second);
			return true;
		});
	}
};

bool strings::regex_search(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return select_iterator<regex_search_base>(pattern, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

bool strings::regex_search(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return regex_search_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

template <class Iter>
struct regex_extract_base
{
	cell operator()(Iter pattern_begin, Iter pattern_end, const regex_info &info) const
	{
		return get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
		{
			if(!state.search())
			{
				return 0;
			}
			info.found(state.match[0].second);
			tag_ptr chartag = tags::find_tag(tags::tag_char);
			auto list = list_pool.add();
			for(auto &group : state.match)
			{
				dyn_object obj(&*group.first, group.length() + 1, chartag);
				*(obj.end() - 1) = 0;
				list->push_back(std::move(obj));
			}
			return list_pool.get_id(list);
		});
	}
};

cell strings::regex_extract(const cell_string &str, const cell *pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return select_iterator<regex_extract_base>(pattern, regex_info{str, nullptr, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

cell strings::regex_extract(const cell_string &str, const cell_string &pattern, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		return regex_extract_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

template <class SubIter>
struct replace_sub_match_base
{
	template <class Iter>
	struct inner
	{
		void operator()(Iter begin, Iter end, cell_string &target, cell escape_char, SubIter sbegin, SubIter send) const
		{
			auto last = begin;
			while(begin != end)
			{
				if(*begin == '$' || *begin == escape_char)
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

template <class RegexState, class RegexGroup>
static bool replace_move_next(cell_string &target, RegexState &state, const RegexGroup &group)
{
	state.options |= std::regex_constants::match_prev_avail;
	state.begin = group.second;
	if(group.first == group.second)
	{
		// zero-length group - always move by one character
		if(state.begin == state.end)
		{
			// at end - cannot move
			return false;
		}
		// append the skipped character
		target.append(1, *state.begin);
		++state.begin;
	}
	return true;
}

template <class ReplacementIter, class RegexState>
void replace_str(cell_string &target, RegexState &state, ReplacementIter replacement_begin, ReplacementIter replacement_end)
{
	using group_iterator = typename RegexState::match_iterator;

	while(state.search())
	{
		const auto &group = state.match[0];
		target.append(state.begin, group.first);
		typename replace_sub_match_base<group_iterator>::template inner<ReplacementIter>()(replacement_begin, replacement_end, target, state.escape_char(), std::next(state.match.cbegin()), state.match.cend());
		if(!replace_move_next(target, state, group))
		{
			break;
		}
	}
	target.append(state.begin, state.end);
}

template <class PatternIter>
struct regex_replace_base
{
	template <class ReplacementIter>
	struct inner
	{
		void operator()(ReplacementIter replacement_begin, ReplacementIter replacement_end, PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const regex_info &info) const
		{
			get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
			{
				target.append(info.string.cbegin(), state.begin);
				replace_str(target, state, replacement_begin, replacement_end);
				info.found(state.begin);
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
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		typename regex_replace_base<cell_string::const_iterator>::template inner<cell_string::const_iterator>()(replacement.begin(), replacement.end(), pattern.begin(), pattern.end(), target, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

template <class RegexState>
void replace_list(cell_string &target, RegexState &state, const list_t &replacement)
{
	using group_iterator = typename RegexState::match_iterator;

	while(state.search())
	{
		const auto &group = state.match[0];
		target.append(state.begin, group.first);
		size_t index = 0;
		for(auto it = std::next(state.match.cbegin()); it != state.match.cend();)
		{
			index++;
			const auto &capture = *it;
			if(capture.matched && index <= replacement.size())
			{
				const dyn_object &repl = replacement[index - 1];

				auto begin = it;
				++it;
				auto end = std::find_if(it, state.match.cend(), [](const auto &match) {return !match.matched; });
				it = end;

				if(repl.get_tag()->inherits_from(tags::tag_string))
				{
					cell value = repl.get_cell(0);
					cell_string *repl_str;
					if(strings::pool.get_by_id(value, repl_str))
					{
						typename replace_sub_match_base<group_iterator>::template inner<cell_string::const_iterator>()(repl_str->cbegin(), repl_str->cend(), target, state.escape_char(), begin, end);
						continue;
					}
				}else if(repl.get_tag()->inherits_from(tags::tag_char) && repl.is_array())
				{
					select_iterator<replace_sub_match_base<group_iterator>::template inner>(repl.begin(), target, state.escape_char(), begin, end);
					continue;
				}
				cell_string str = repl.to_string(strings::default_encoding());
				typename replace_sub_match_base<group_iterator>::template inner<cell_string::const_iterator>()(str.cbegin(), str.cend(), target, state.escape_char(), begin, end);
			}else{
				++it;
			}
		}
		if(!replace_move_next(target, state, group))
		{
			break;
		}
	}
	target.append(state.begin, state.end);
}

template <class PatternIter>
struct regex_replace_list_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, const list_t &replacement, const regex_info &info) const
	{
		get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
		{
			target.append(info.string.cbegin(), state.begin);
			replace_list(target, state, replacement);
			info.found(state.begin);
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
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const list_t &replacement, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_list_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, replacement, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

template <class RegexState>
void replace_func(cell_string &target, RegexState &state, AMX *amx, int replacement_index, const char *format, cell *params, size_t numargs)
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

	while(state.search())
	{
		const auto &group = state.match[0];
		target.append(state.begin, group.first);

		amx::guard guard(amx);

		for(int i = state.match.size() - 1; i >= 0; i--)
		{
			const auto &capture = state.match[i];

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

		if(!replace_move_next(target, state, group))
		{
			break;
		}
	}
	target.append(state.begin, state.end);
}

template <class PatternIter>
struct regex_replace_func_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, AMX *amx, int replacement_index, const regex_info &info, const char *format, cell *params, size_t numargs) const
	{
		get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
		{
			target.append(info.string.cbegin(), state.begin);
			replace_func(target, state, amx, replacement_index, format, params, numargs);
			info.found(state.begin);
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
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, int replacement_index, cell *pos, cell options, const char *format, cell *params, size_t numargs, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_func_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, amx, replacement_index, regex_info{str, &pattern, pos, options, std::move(mem_handle)}, format, params, numargs);
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

template <class RegexState>
void replace_expr(cell_string &target, RegexState &state, AMX *amx, const expression &expr)
{
	expression::exec_info info(amx);

	std::vector<dyn_object> args;
	args.resize(state.regex.mark_count() + 1);
	expression::args_type ref_args;
	for(const auto &arg : args)
	{
		ref_args.push_back(std::cref(arg));
	}

	while(state.search())
	{
		auto group = state.match[0];
		target.append(state.begin, group.first);

		for(size_t i = 0; i < state.match.size(); i++)
		{
			const auto &capture = state.match[i];
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

		if(!replace_move_next(target, state, group))
		{
			break;
		}
	}
	target.append(state.begin, state.end);
}

template <class PatternIter>
struct regex_replace_expr_base
{
	void operator()(PatternIter pattern_begin, PatternIter pattern_end, cell_string &target, AMX *amx, const expression &expr, const regex_info &info) const
	{
		get_regex_state(pattern_begin, pattern_end, info, [&](auto &&state)
		{
			target.append(info.string.cbegin(), state.begin);
			replace_expr(target, state, amx, expr);
			info.found(state.begin);
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
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
	}
}

void strings::regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, AMX *amx, const expression &expr, cell *pos, cell options, std::weak_ptr<void> mem_handle)
{
	try{
		regex_replace_expr_base<cell_string::const_iterator>()(pattern.begin(), pattern.end(), target, amx, expr, regex_info{str, &pattern, pos, options, std::move(mem_handle)});
	}catch(const std::regex_error &err)
	{
		amx_FormalError("%s (%s)", err.what(), get_error(err.code()));
	}catch(const lex::lex_error &err)
	{
		amx_FormalError(err.what(), err.first_arg());
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

bool strings::impl::check_valid_time_format(std::string format)
{
	static std::regex valid_pattern = ([]()
	{
		std::regex result;
		result.imbue(std::locale::classic());
		result.assign("^(?:[^%\\0]+|%(?:E[YyCcxX]|0[ymUWVdewuHIMS]|[%ntYyCGgbhBmUWVjdeaAwuHIMScxXDFrRTpzZ]))*$", std::regex_constants::ECMAScript | std::regex_constants::optimize);
		return result;
	})();

	return std::regex_match(format, valid_pattern);
}
