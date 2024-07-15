#include "strings.h"

#include <stddef.h>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <memory>
#include <unordered_map>
#include <iterator>
#include <limits>
#include <codecvt>
#ifdef _WIN32
#include <locale.h>
#endif

using namespace strings;

object_pool<cell_string> strings::pool;

cell strings::null_value1[1] = {0};
cell strings::null_value2[2] = {0, 1};

cell strings::create(const cell *addr, bool truncate, bool fixnulls)
{
	auto &ptr = pool.emplace(convert(addr));
	if(truncate || fixnulls)
	{
		for(auto &c : *ptr)
		{
			if(truncate)
			{
				c &= 0xFF;
			}
			if(fixnulls && c == 0)
			{
				c = 0x00FFFF00;
			}
		}
	}
	return pool.get_id(ptr);
}

cell strings::create(const cell *addr, size_t length, bool packed, bool truncate, bool fixnulls)
{
	auto &ptr = pool.emplace(convert(addr, length, packed));
	if(truncate || fixnulls)
	{
		for(auto &c : *ptr)
		{
			if(truncate)
			{
				c &= 0xFF;
			}
			if(fixnulls && c == 0)
			{
				c = 0x00FFFF00;
			}
		}
	}
	return pool.get_id(ptr);
}

cell_string strings::convert(const cell *str)
{
	if(!str)
	{
		return {};
	}
	int len;
	amx_StrLen(str, &len);
	return convert(str, len, static_cast<ucell>(*str) > UNPACKEDMAX);
}

cell_string strings::convert(const cell *str, size_t length, bool packed)
{
	if(!str)
	{
		return {};
	}
	if(!packed)
	{
		return cell_string(str, str + length);
	}else if(reinterpret_cast<std::intptr_t>(str) % sizeof(cell) == 0)
	{
		aligned_const_char_iterator it(str);
		return cell_string(it, it + length);
	}else{
		unaligned_const_char_iterator it(str);
		return cell_string(it, it + length);
	}
}

cell_string strings::convert(const std::string &str)
{
	size_t len = str.size();
	cell_string cstr(len, '\0');
	for(size_t i = 0; i < len; i++)
	{
		cstr[i] = static_cast<unsigned char>(str[i]);
	}
	return cstr;
}

cell strings::create(const std::string &str)
{
	return pool.get_id(pool.emplace(convert(str)));
}

bool strings::clamp_range(const cell_string &str, cell &start, cell &end)
{
	clamp_pos(str, start);
	clamp_pos(str, end);

	return start <= end;
}

bool strings::clamp_pos(const cell_string &str, cell &pos)
{
	size_t size = str.size();
	if(pos < 0) pos += size;
	if(static_cast<size_t>(pos) >= size)
	{
		pos = size;
		return false;
	}
	return true;
}


template <class Locale>
using flags_type_of = typename std::remove_reference<decltype((std::declval<encoding_info<Locale>>().flags))>::type;

template <class Locale>
using flags_int_type_of = typename std::underlying_type<flags_type_of<Locale>>::type;

template <class Locale>
flags_type_of<Locale> operator~(flags_type_of<Locale> a)
{
	return (flags_type_of<Locale>)(~(flags_int_type_of<Locale>)a);
}
template <class Locale>
flags_type_of<Locale> operator|(flags_type_of<Locale> a, flags_type_of<Locale> b)
{
	return (flags_type_of<Locale>)((flags_int_type_of<Locale>)a | (flags_int_type_of<Locale>)b);
}
template <class Locale>
flags_type_of<Locale> operator&(flags_type_of<Locale> a, flags_type_of<Locale> b)
{
	return (flags_type_of<Locale>)((flags_int_type_of<Locale>)a & (flags_int_type_of<Locale>)b);
}
template <class Locale>
flags_type_of<Locale> operator^(flags_type_of<Locale> a, flags_type_of<Locale> b)
{
	return (flags_type_of<Locale>)((flags_int_type_of<Locale>)a ^ (flags_int_type_of<Locale>)b);
}
flags_type_of<const char*>& operator|=(flags_type_of<const char*>& a, flags_type_of<const char*> b)
{
	return (flags_type_of<const char*>&)((flags_int_type_of<const char*>&)a |= (flags_int_type_of<const char*>)b);
}
template <class Locale>
flags_type_of<Locale>& operator&=(flags_type_of<Locale>& a, flags_type_of<Locale> b)
{
	return (flags_type_of<Locale>&)((flags_int_type_of<Locale>&)a &= (flags_int_type_of<Locale>)b);
}
template <class Locale>
flags_type_of<Locale>& operator^=(flags_type_of<Locale>& a, flags_type_of<Locale> b)
{
	return (flags_type_of<Locale>&)((flags_int_type_of<Locale>&)a ^= (flags_int_type_of<Locale>)b);
}

namespace
{
	std::locale custom_locale;
	std::string custom_locale_name;

	std::locale::category get_category(cell category)
	{
		if(category == -1) return std::locale::all;
		std::locale::category cat = std::locale::none;
		if(category & 1)
		{
			cat |= std::locale::collate;
		}
		if(category & 2)
		{
			cat |= std::locale::ctype;
		}
		if(category & 4)
		{
			cat |= std::locale::monetary;
		}
		if(category & 8)
		{
			cat |= std::locale::numeric;
		}
		if(category & 16)
		{
			cat |= std::locale::time;
		}
		if(category & 32)
		{
			cat |= std::locale::messages;
		}
		return cat;
	}

	char *split_locale(char *str)
	{
		auto pos = std::strchr(str, '|');
		if(pos)
		{
			*pos = '\0';
			++pos;
		}
		return pos;
	}

#ifndef _WIN32
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#endif

	using encoding_data = encoding_info<const char*>;
	using encoding_type = decltype(encoding_data::type);
	using encoding_flags = decltype(encoding_data::flags);

	encoding_type parse_encoding_type(char *&spec)
	{
		auto pos = std::strchr(spec, '+');
		if(pos)
		{
			pos[0] = '\0';
		}

		auto found = [&](encoding_type type)
		{
			static char empty[1] = {'\0'};
			spec = pos ? pos + 1 : empty;
			return type;
		};

		if(!_stricmp(spec, "ansi"))
		{
			return found(encoding_data::ansi);
		}
		if(!_stricmp(spec, "unicode"))
		{
			return found(encoding_data::unicode);
		}
		if(!_strnicmp(spec, "utf", 3))
		{
			auto utf = spec + 3;
			if(utf[0] == '-')
			{
				++utf;
			}
			if(!_stricmp(utf, "8"))
			{
				return found(encoding_data::utf8);
			}
			if(!_stricmp(utf, "16"))
			{
				return found(encoding_data::utf16);
			}
			if(!_stricmp(utf, "32"))
			{
				return found(encoding_data::utf32);
			}
		}

		if(pos)
		{
			pos[0] = '+';
		}
		return encoding_data::ansi;
	}

	void parse_encoding_flags(encoding_data &data, char *spec, char *spec_end)
	{
		using it = std::reverse_iterator<char*>;
		it begin{spec_end}, end{spec};

		it start;
		while((start = std::find(begin, end, ';')) != end)
		{
			char *param = start.base();
			if(!_stricmp(param, "trunc"))
			{
				data.flags |= encoding_data::truncated_cells;
			}else if(!_stricmp(param, "ucs"))
			{
				data.flags |= encoding_data::unicode_ucs;
			}else if(!_stricmp(param, "bom"))
			{
				data.flags |= encoding_data::unicode_use_header;
			}else if(!_stricmp(param, "maxrange"))
			{
				data.flags |= encoding_data::unicode_extended;
			}else if(param + 10 == spec_end && !_strnicmp(param, "fallback=", 9))
			{
				data.unknown_char = param[9];
			}else if(!_stricmp(param, "native"))
			{
				data.flags |= encoding_data::unicode_native;
			}else{
				// no recognized param
				break;
			}
			*start = '\0';
			++start;
			spec_end = start.base();
		}
	}

	encoding_data parse_encoding(char *&spec)
	{
		encoding_data data{spec};
		parse_encoding_flags(data, spec, spec + std::strlen(spec));
		data.type = parse_encoding_type(const_cast<char*&>(data.locale));
		return data;
	}
}

template <>
void encoding_info<std::locale>::fill_from_locale();

encoding strings::find_encoding(char *&spec, bool default_if_empty)
{
	auto make_encoding = [&](const encoding_data &data) -> encoding
	{
		const char *locale = data.locale;
		if(locale[0] == '\0' && !default_if_empty)
		{
			encoding defaulted{std::locale(), data};
			defaulted.fill_from_locale();
			return defaulted;
		}
		if(locale[0] == '*' && locale[1] == '\0')
		{
			// special case to use the system default
			locale = "";
		}
		return {std::locale(locale), data};
	};

	if(!spec)
	{
		return make_encoding(encoding_data(""));
	}
	char *nextspec;
	while(nextspec = split_locale(spec))
	{
		auto data = parse_encoding(spec);
		try{
			return make_encoding(data);
		}catch(const std::runtime_error &)
		{

		}
		spec = nextspec;
	}
	return make_encoding(parse_encoding(spec));
}

encoding strings::default_encoding()
{
	static encoding_data empty(nullptr);
	encoding result{std::locale(), empty};
	result.fill_from_locale();
	return result;
}

template <class Facet, class... Args>
void add_facet(std::locale &target, Args&&... args)
{
	if(!std::has_facet<Facet>(target))
	{
		Facet::install(target, std::forward<Args>(args)...);
	}
}

template <class Facet>
void copy_facet(std::locale &target, const std::locale &source)
{
	if(std::has_facet<Facet>(source))
	{
		Facet::install(target, std::use_facet<Facet>(source));
	}
}

static std::locale merge_locale(const std::locale &base, const std::locale &specific, std::locale::category cat)
{
	std::locale result(base, specific, cat);
	copy_facet<std::ctype<char8_t>>(result, specific);
	copy_facet<std::ctype<char16_t>>(result, specific);
	copy_facet<std::ctype<char32_t>>(result, specific);
	copy_facet<std::ctype<cell>>(result, specific);
	return result;
}

template <class T>
struct false_dep
{
	static constexpr const bool value = false;
};

template <class Locale>
typename encoding_info<Locale>::template make_value<Locale>::type encoding_info<Locale>::install() const
{
	static_assert(false_dep<Locale>::value, "This function is not implemented.");
}

template <>
std::locale encoding_info<std::locale>::install() const
{
	if(unmodified)
	{
		return locale;
	}
	std::locale result = locale;
	bool treat_as_truncated = static_cast<bool>(flags & truncated_cells);
	switch(type)
	{
		case ansi:
			std::ctype<cell>::install<char>(result, treat_as_truncated);
			break;
		case unicode:
			std::ctype<cell>::install<wchar_t>(result, treat_as_truncated);
			break;
		case utf8:
			add_facet<std::ctype<char8_t>>(result);
			std::ctype<cell>::install<char8_t>(result, treat_as_truncated);
			break;
		case utf16:
			if(sizeof(char16_t) == sizeof(wchar_t))
			{
				std::ctype<cell>::install<wchar_t>(result, treat_as_truncated);
			}else{
				add_facet<std::ctype<char16_t>>(result, static_cast<bool>(flags & unicode_ucs));
				std::ctype<cell>::install<char16_t>(result, treat_as_truncated);
			}
			break;
		case utf32:
			if(sizeof(char32_t) == sizeof(wchar_t))
			{
				std::ctype<cell>::install<wchar_t>(result, treat_as_truncated);
			}else{
				add_facet<std::ctype<char32_t>>(result, static_cast<bool>(flags & unicode_ucs));
				std::ctype<cell>::install<char32_t>(result, treat_as_truncated);
			}
			break;
	}
	return result;
}

template <class Locale>
void encoding_info<Locale>::fill_from_locale()
{
	static_assert(false_dep<Locale>::value, "This function is not implemented.");
}

template <>
void encoding_info<std::locale>::fill_from_locale()
{
	unmodified = true;
	if(std::has_facet<std::ctype<cell>>(locale))
	{
		const auto &ctype = std::use_facet<std::ctype<cell>>(locale);
		auto original_type = unspecified;
		const auto &base_type = ctype.underlying_char_type();
		if(base_type == typeid(char))
		{
			original_type = ansi;
		}else if(base_type == typeid(wchar_t))
		{
			original_type = unicode;
		}else if(base_type == typeid(char8_t))
		{
			original_type = utf8;
		}else if(base_type == typeid(char16_t))
		{
			original_type = utf16;
		}else if(base_type == typeid(char32_t))
		{
			original_type = utf32;
		}
		if(type == unspecified)
		{
			type = original_type;
		}else if(type != original_type)
		{
			unmodified = false;
		}

		if(ctype.truncates_cells())
		{
			if(!(flags & truncated_cells))
			{
				flags = static_cast<decltype(flags)>(flags | truncated_cells);
			}
		}else if(flags & truncated_cells)
		{
			unmodified = false;
		}
	}else{
		unmodified = false;
	}

	auto utf_type = type;
	if(utf_type == unicode)
	{
		switch(sizeof(wchar_t))
		{
			case sizeof(char16_t):
				utf_type = utf16;
				break;
			case sizeof(char32_t):
				utf_type = utf32;
				break;
		}
	}
	auto receiver = [&](const auto &ctype)
	{
		if(ctype.get_flag())
		{
			if(!(flags & unicode_ucs))
			{
				flags = static_cast<decltype(flags)>(flags | unicode_ucs);
			}
		}else if(flags & unicode_ucs)
		{
			unmodified = false;
		}
	};
	switch(utf_type)
	{
		case utf16:
			if(std::has_facet<std::ctype<char16_t>>(locale))
			{
				receiver(std::use_facet<std::ctype<char16_t>>(locale));
			}
			break;
		case utf32:
			if(std::has_facet<std::ctype<char32_t>>(locale))
			{
				receiver(std::use_facet<std::ctype<char32_t>>(locale));
			}
			break;
	}
}

std::string get_c_locale_name(const char *lc_name, std::locale::category cat)
{
#ifdef _WIN32
	// get LC_ category
	int lc_cat;
	if((cat - 1) & cat)
	{
		// multiple categories
		lc_cat = LC_ALL;
	}else if(cat & std::locale::collate)
	{
		lc_cat = LC_COLLATE;
	}else if(cat & std::locale::ctype)
	{
		lc_cat = LC_CTYPE;
	}else if(cat & std::locale::monetary)
	{
		lc_cat = LC_MONETARY;
	}else if(cat & std::locale::numeric)
	{
		lc_cat = LC_NUMERIC;
	}else if(cat & std::locale::time)
	{
		lc_cat = LC_TIME;
	}else{
		lc_cat = LC_ALL;
	}

	// make thread-local config and backup current locale
	int threadconfig = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);

	// get current name
	std::string original_locale = setlocale(lc_cat, nullptr);

	// set to empty and get final name
	std::string name = setlocale(lc_cat, lc_name);

	// restore
	setlocale(lc_cat, original_locale.c_str());
	_configthreadlocale(threadconfig);

	if(name.empty() && lc_name[0] == '\0')
	{
		// still empty - fallback to special *
		return "*";
	}

	return name;
#else
	// unused
	if(lc_name[0] == '\0')
	{
		return "*";
	}
	return std::string();
#endif
}

void strings::set_encoding(const encoding &enc, cell category)
{
	auto set_global = [](std::locale loc)
	{
		std::locale::global(custom_locale = std::move(loc));
	};

	auto cat = get_category(category);
	if((cat & std::locale::all) == std::locale::all)
	{
		set_global(enc.install());
	}else if(cat & std::locale::ctype)
	{
		set_global(merge_locale(custom_locale, enc.install(), cat));
	}else {
		set_global(std::locale(custom_locale, enc.locale, cat));
	}
	custom_locale_name = enc.locale.name();
#ifdef _WIN32
	// translate the name through system
	std::string c_name = get_c_locale_name(custom_locale_name.c_str(), cat);
	if(!c_name.empty())
	{
		custom_locale_name = std::move(c_name);
	}
#endif
}

void strings::reset_locale()
{
	std::locale::global(std::locale::classic());
}

const std::string &strings::current_locale_name()
{
	return custom_locale_name;
}

std::string strings::get_locale_name(const encoding &enc, cell category)
{
	if(enc.locale == std::locale())
	{
		return custom_locale_name;
	}
	std::string name = enc.locale.name();
#ifdef _WIN32
	std::string c_name = get_c_locale_name(name.c_str(), get_category(category));
	if(!c_name.empty())
	{
		return c_name;
	}
#endif
	return name;
}

using ctype_transform = const cell*(*)(const std::ctype<cell> &obj, cell*, const cell*);

struct ctype_transform_helper
{
	static const cell *tolower(const std::ctype<cell> &obj, cell *begin, const cell *end)
	{
		return obj.tolower(begin, end);
	}

	static const cell *toupper(const std::ctype<cell> &obj, cell *begin, const cell *end)
	{
		return obj.toupper(begin, end);
	}
};

template <ctype_transform Transform>
void transform_string(cell_string &str, const encoding &enc)
{
	std::locale locale = enc.install();
	const auto &facet = std::use_facet<std::ctype<cell>>(locale);
	Transform(facet, &str[0], &str[str.size()]);
}

void strings::to_lower(cell_string &str, const encoding &enc)
{
	transform_string<&ctype_transform_helper::tolower>(str, enc);
}

void strings::to_upper(cell_string &str, const encoding &enc)
{
	transform_string<&ctype_transform_helper::toupper>(str, enc);
}

constexpr const size_t buffer_size = 16;

// Compatible character type for non-platform UTF-8 conversions
#ifdef __cpp_char8_t
using u8char = char8_t;
#else
using u8char = char;
#endif

// Calls encoder/decoder with a receiver for its outpu
template <template <class> class Coder, class Receiver, class... Args>
void call_coder(Receiver receiver, Args&&... args)
{
	return Coder<Receiver>()(std::forward<Receiver>(receiver), std::forward<Args>(args)...);
}

// Functor to use as a receiver to append output directly
struct output_appender
{
	cell_string &output;

	template <class Element>
	void operator()(const Element *input_begin, const Element *input_end) const
	{
		using unsigned_element = typename std::make_unsigned<Element>::type;

		ptrdiff_t size = input_end - input_begin;
		size_t pos = output.size();
		output.resize(pos + size, 0);
		std::copy(reinterpret_cast<const unsigned_element*>(input_begin), reinterpret_cast<const unsigned_element*>(input_end), &output[pos]);
	}
};

#ifdef _MSC_VER
// Workaround for MSC not defining std::codecvt::id
template <class CharType>
using make_char_t = typename std::make_signed<CharType>::type;

template <class CharType>
struct unmake_char_helper
{
	using type = CharType;
};

template <>
struct unmake_char_helper<make_char_t<char8_t>>
{
	using type = char8_t;
};

template <>
struct unmake_char_helper<make_char_t<char16_t>>
{
	using type = char16_t;
};

template <>
struct unmake_char_helper<make_char_t<char32_t>>
{
	using type = char32_t;
};

template <class CharType>
using unmake_char_t = typename unmake_char_helper<CharType>::type;
#else
template <class CharType>
using make_char_t = CharType;

template <class CharType>
using unmake_char_t = CharType;
#endif

template <class Type>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<Type>::type>::type;

// used to store the result of previous conversion
struct cvt_state
{
	std::mbstate_t state;
	std::codecvt_base::result last_result;

	cvt_state() : state(), last_result(std::codecvt_base::ok)
	{

	}
};

// The type of std::codecvt::in or std::codecvt::out
template <class Input, class Output>
using codecvt_process = std::codecvt_base::result(*)(const std::codecvt_base &obj, cvt_state&, const Input*, const Input*, const Input*&, Output*, Output*, Output*&);

// Repeatedly calls Conversion until all input is converted and received
template <class Input, class Output, codecvt_process<Input, Output> Conversion, class Receiver>
void encode_buffer(const std::codecvt_base &cvt, const encoding &enc, cvt_state &state, const Input *input_begin, const Input *input_end, Output *buffer_begin, Output *buffer_end, Receiver receiver)
{
	const Input *input_next = input_begin;
	Output *buffer_next = buffer_begin;

	// Convert until characters remain
	while(input_begin < input_end || input_end == nullptr)
	{
		std::codecvt_base::result result = Conversion(cvt, state, input_begin, input_end, input_next, buffer_begin, buffer_end, buffer_next);
		
		if(result == std::codecvt_base::noconv)
		{
			std::string msg = "std::codecvt: Conversion from ";
			msg += typeid(Input).name();
			msg += " to ";
			msg += typeid(Output).name();
			msg += " is not supported.";
			throw std::invalid_argument(msg);
		}

		receiver(
			static_cast<const Output*>(buffer_begin),
			static_cast<const Output*>(buffer_next)
		);

		if(input_next == nullptr && result != std::codecvt_base::ok)
		{
			result = std::codecvt_base::error;
		}

		switch(result)
		{
			case std::codecvt_base::error:
			{
				const Output unknown[1] = {static_cast<Output>(static_cast<unsigned char>(enc.unknown_char))};
				receiver(unknown, unknown + 1);
				if(input_next == nullptr)
				{
					return;
				}
				++input_next;
				state = {};
			}
			break;
			case std::codecvt_base::partial:
				if(input_next == input_end)
				{
					// non-final chunk - needs more input
					return;
				}
				// buffer full - continue
				break;
			case std::codecvt_base::ok:
				// all good
				break;
		}

		if(input_next == nullptr)
		{
			// processed final chunk
			return;
		}

		if(input_next == input_begin)
		{
			throw std::logic_error("std::codecvt: No characters were read from input.");
		}
		input_begin = input_next;
	}
}

template <class CodeCvt, class Input, class Output>
struct encode_buffer_helper;

template <class Input, class Output>
struct encode_buffer_helper<std::codecvt<Input, Output, std::mbstate_t>, Input, Output>
{
	static std::codecvt_base::result out(const std::codecvt_base &obj, cvt_state &state, const Input *input_begin, const Input *input_end, const Input *&input_next, Output *output_begin, Output *output_end, Output *&output_next)
	{
		const auto &cvt = static_cast<const std::codecvt<Input, Output, std::mbstate_t>&>(obj);
		if(input_end == nullptr)
		{
			// end of data - finalize std::mbstate_t
			auto result = cvt.unshift(state.state, output_begin, output_end, output_next);
			if(result == std::codecvt_base::noconv)
			{
				result = std::codecvt_base::ok;
			}
			input_next = input_end;
			return state.last_result = result;
		}
		return state.last_result = cvt.out(state.state, input_begin, input_end, input_next, output_begin, output_end, output_next);
	}
};

template <class Input, class Output>
struct encode_buffer_helper<std::codecvt<Output, Input, std::mbstate_t>, Input, Output>
{
	static std::codecvt_base::result in(const std::codecvt_base &obj, cvt_state &state, const Input *input_begin, const Input *input_end, const Input *&input_next, Output *output_begin, Output *output_end, Output *&output_next)
	{
		const auto &cvt = static_cast<const std::codecvt<Output, Input, std::mbstate_t>&>(obj);
		if(input_end == nullptr)
		{
			// nothing new to convert - report last result
			input_next = input_end;
			output_next = output_begin;
			return state.last_result;
		}
		return state.last_result = cvt.in(state.state, input_begin, input_end, input_next, output_begin, output_end, output_next);
	}
};

// Specializes encode_buffer for cvt::func
#define encode_buffer_f(from_type, to_type, cvt, func) \
	encode_buffer< \
		from_type, to_type, \
		&encode_buffer_helper<remove_cvref_t<decltype(cvt)>, from_type, to_type>::func \
	>

#define CODECVT_CLASS template <class, unsigned long, std::codecvt_mode> class

// std::codecvt cache
template <CODECVT_CLASS CodeCvtType, class CharType, unsigned long Maxcode, std::codecvt_mode Mode>
const std::codecvt<CharType, char, std::mbstate_t> &get_codecvt()
{
	static const CodeCvtType<CharType, Maxcode, Mode> cvt{};
	return cvt;
}

template <CODECVT_CLASS CodeCvtType, class CharType, unsigned long Maxcode>
const std::codecvt<CharType, char, std::mbstate_t> &get_codecvt(const encoding &enc)
{
	using mode = std::codecvt_mode;
	using mode_int = typename std::underlying_type<mode>::type;

	return (enc.flags & encoding::unicode_use_header) ?
		get_codecvt<CodeCvtType, CharType, Maxcode, static_cast<mode>(static_cast<mode_int>(mode::consume_header) | static_cast<mode_int>(mode::generate_header))>() :
		get_codecvt<CodeCvtType, CharType, Maxcode, mode{}>();
}

template <CODECVT_CLASS CodeCvtType, class CharType>
const std::codecvt<CharType, char, std::mbstate_t> &get_codecvt(const encoding &enc)
{
	return (enc.flags & encoding::unicode_extended) ?
		get_codecvt<CodeCvtType, CharType, std::numeric_limits<unsigned long>::max()>(enc) :
		get_codecvt<CodeCvtType, CharType, 0x10FFFF>(enc);
}

#ifdef _MSC_VER
template <class CharType, unsigned long Maxcode, std::codecvt_mode Mode>
class codecvt_native_utf8 : public std::codecvt<CharType, u8char, std::mbstate_t>
{
	using original_intern_type = unmake_char_t<CharType>;

	struct inner_codecvt : public std::codecvt<original_intern_type, u8char, std::mbstate_t>
	{
		inner_codecvt() : std::codecvt<original_intern_type, u8char, std::mbstate_t>(std::_Locinfo(), Maxcode, static_cast<std::_Codecvt_mode>(Mode), 0)
		{

		}
	} inner;

	static const original_intern_type *&reinterpret(const intern_type *&ptr)
	{
		return reinterpret_cast<const original_intern_type*&>(ptr);
	}

	static original_intern_type *&reinterpret(intern_type *&ptr)
	{
		return reinterpret_cast<original_intern_type*&>(ptr);
	}

public:
	codecvt_native_utf8()
	{

	}

protected:
	virtual result do_out(state_type &state, const intern_type *from, const intern_type *from_end, const intern_type *&from_next, u8char *to, u8char *to_end, u8char *&to_next) const override
	{
		return inner.out(state, reinterpret(from), reinterpret(from_end), reinterpret(from_next), to, to_end, to_next);
	}

	virtual result do_in(state_type &state, const u8char *from, const u8char *from_end, const u8char *&from_next, intern_type *to, intern_type *to_end, intern_type *&to_next) const override
	{
		return inner.in(state, from, from_end, from_next, reinterpret(to), reinterpret(to_end), reinterpret(to_next));
	}

	virtual result do_unshift(state_type &state, u8char *to, u8char *to_end, u8char *&to_next) const override
	{
		return inner.unshift(state, to, to_end, to_next);
	}

	virtual int do_encoding() const noexcept override
	{
		return inner.encoding();
	}

	virtual bool do_always_noconv() const noexcept override
	{
		return inner.always_noconv();
	}

	virtual int do_length(state_type &state, const u8char *from, const u8char *from_end, std::size_t max) const override
	{
		return inner.length(state, from, from_end, max);
	}

	virtual int do_max_length() const noexcept override
	{
		return inner.max_length();
	}
};

template <unsigned long Maxcode, std::codecvt_mode Mode>
class codecvt_native_utf8<wchar_t, Maxcode, Mode> : public std::codecvt<wchar_t, u8char, std::mbstate_t>
{
public:
	codecvt_native_utf8()
	{

	}
};
#endif

template <class CharType>
const std::codecvt<CharType, char, std::mbstate_t> &get_codecvt_utf8(const encoding &enc)
{
	if(!std::is_same<CharType, wchar_t>::value && enc.flags & encoding::unicode_native)
	{
		// conversion between UTF-8 and UTF-16/32 through the locale
#ifdef _MSC_VER
		// all have identical implementation
		return get_codecvt<codecvt_native_utf8, CharType>(enc);
#else
		// make_char_t is identity 
		return std::use_facet<std::codecvt<CharType, u8char, std::mbstate_t>>(enc.locale);
#endif
	}
	return (sizeof(CharType) == sizeof(char16_t)) != static_cast<bool>(enc.flags & encoding::unicode_ucs) ?
		// is 16-bit and not UCS mode, or is 32-bit and UCS mode
		get_codecvt<std::codecvt_utf8_utf16, CharType>(enc) :
		// is 16-bit and UCS mode, or is 32-bit and not UCS mode
		get_codecvt<std::codecvt_utf8, CharType>(enc);
}

template <template <class> class WCharDecoder>
struct encoder_from_wchar_t
{
	// Encodes from wchar_t decoder
	template <class U8CharReceiver>
	struct to_utf8
	{
		// Conversion to UTF-8
		template <class... Args>
		void operator()(U8CharReceiver receiver, const encoding &output_enc, Args&&... args) const
		{
			const auto &cvt = get_codecvt_utf8<wchar_t>(output_enc);
			char buffer[buffer_size];
			cvt_state state{};

			// Reads wchar_t
			return call_coder<WCharDecoder>([&](const wchar_t *input_begin, const wchar_t *input_end)
			{
				// Encodes wchar_t to UTF-8 (out)
				return encode_buffer_f(wchar_t, char, cvt, out)(cvt, output_enc, state, input_begin, input_end, buffer, buffer + buffer_size, [&](const char *begin, const char *end)
				{
					return receiver(
						reinterpret_cast<const u8char*>(begin),
						reinterpret_cast<const u8char*>(end)
					);
				});
			}, std::forward<Args>(args)...);
		}
	};

	template <class CharType>
	struct through_utf8
	{
		// Conversion to CharType (UTF-16 / UTF-32) from UTF-8
		using char_type = make_char_t<CharType>;

		template <class CharReceiver>
		struct to_char_type
		{
			template <class... Args>
			void operator()(CharReceiver receiver, const encoding &output_enc, Args&&... args) const
			{
				const auto &cvt = get_codecvt_utf8<char_type>(output_enc);
				char_type buffer[buffer_size];
				cvt_state state{};

				// Reads UTF-8
				return call_coder<to_utf8>([&](const u8char *input_begin, const u8char *input_end)
				{
					// Encodes UTF-8 to CharType (in)
					return encode_buffer_f(u8char, char_type, cvt, in)(cvt, output_enc, state, input_begin, input_end, buffer, buffer + buffer_size, [&](const char_type *begin, const char_type *end)
					{
						return receiver(
							reinterpret_cast<const CharType*>(begin),
							reinterpret_cast<const CharType*>(end)
						);
					});
				}, output_enc, std::forward<Args>(args)...);
			}
		};
	};

	template <class... Args>
	void operator()(cell_string &output, const encoding &output_enc, Args&&... args) const
	{
		switch(output_enc.type)
		{
			case encoding::utf8:
				// Append to output as UTF-8
				return call_coder<to_utf8>(output_appender{output}, output_enc, std::forward<Args>(args)...);
			case encoding::unicode:
				// Append to output directly
				return call_coder<WCharDecoder>(output_appender{output}, std::forward<Args>(args)...);
			case encoding::utf16:
				// Append to output as UTF-16
				return call_coder<through_utf8<char16_t>::template to_char_type>(output_appender{output}, output_enc, std::forward<Args>(args)...);
			case encoding::utf32:
				// Append to output as UTF-32
				return call_coder<through_utf8<char32_t>::template to_char_type>(output_appender{output}, output_enc, std::forward<Args>(args)...);
		}

		// Conversion to ANSI
		auto loc = output_enc.install();
		const auto &ctype = std::use_facet<std::ctype<cell>>(loc);
		const auto &cvt = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);

		char char_buffer[buffer_size];
		cvt_state state{};

		return call_coder<WCharDecoder>([&](const wchar_t *input_begin, const wchar_t *input_end)
		{
			encode_buffer_f(wchar_t, char, cvt, out)(cvt, output_enc, state, input_begin, input_end, char_buffer, char_buffer + buffer_size, [&](const char *begin, const char *end)
			{
				ptrdiff_t size = end - begin;
				size_t pos = output.size();
				output.resize(pos + size, 0);
				return ctype.widen(begin, end, &output[pos]);
			});
		}, std::forward<Args>(args)...);
	}
};

template <template <class> class U8CharDecoder>
struct encoder_from_utf8
{
	// Encodes from UTF-8 decoder
	template <class WCharReceiver>
	struct through_wchar_t
	{
		// Conversion to wchar_t
		template <class... Args>
		void operator()(WCharReceiver receiver, const encoding &output_enc, Args&&... args) const
		{
			const auto &cvt = get_codecvt_utf8<wchar_t>(output_enc);
			wchar_t buffer[buffer_size];
			cvt_state state{};

			// Reads UTF-8
			return call_coder<U8CharDecoder>([&](const u8char *input_begin, const u8char *input_end)
			{
				// Encodes UTF-8 to wchar_t (in)
				return encode_buffer_f(char, wchar_t, cvt, in)(cvt, output_enc, state, reinterpret_cast<const char*>(input_begin), reinterpret_cast<const char*>(input_end), buffer, buffer + buffer_size, [&](const wchar_t *begin, const wchar_t *end)
				{
					return receiver(begin, end);
				});
			}, std::forward<Args>(args)...);
		}
	};

	template <class CharType>
	struct using_char_type
	{
		// Conversion to CharType (UTF-16 / UTF-32)
		using char_type = make_char_t<CharType>;

		template <class CharReceiver>
		struct to_output
		{
			template <class... Args>
			void operator()(CharReceiver receiver, const encoding &output_enc, Args&&... args) const
			{
				const auto &cvt = get_codecvt_utf8<char_type>(output_enc);
				char_type buffer[buffer_size];
				cvt_state state{};

				// Reads UTF-8
				return call_coder<U8CharDecoder>([&](const u8char *input_begin, const u8char *input_end)
				{
					// Encodes UTF-8 to CharType (in)
					return encode_buffer_f(u8char, char_type, cvt, in)(cvt, output_enc, state, input_begin, input_end, buffer, buffer + buffer_size, [&](const char_type *begin, const char_type *end)
					{
						return receiver(begin, end);
					});
				}, std::forward<Args>(args)...);
			}
		};
	};

	template <class... Args>
	void operator()(cell_string &output, const encoding &output_enc, Args&&... args) const
	{
		switch(output_enc.type)
		{
			case encoding::utf8:
				// Append to output directly
				return call_coder<U8CharDecoder>(output_appender{output}, std::forward<Args>(args)...);
			case encoding::unicode:
				// Convert to wchar_t and append
				return call_coder<through_wchar_t>(output_appender{output}, output_enc, std::forward<Args>(args)...);
			case encoding::utf16:
				// Convert to UTF-16 and append
				return call_coder<using_char_type<char16_t>::template to_output>(output_appender{output}, output_enc, std::forward<Args>(args)...);
			case encoding::utf32:
				// Convert to UTF-32 and append
				return call_coder<using_char_type<char32_t>::template to_output>(output_appender{output}, output_enc, std::forward<Args>(args)...);
		}

		// Wrong initial choice of UTF-8 - should have been wchar_t
		return encoder_from_wchar_t<through_wchar_t>()(output, output_enc, output_enc, std::forward<Args>(args)...);
	}
};

template <class CharType>
void copy_narrow(const cell *begin, const cell *end, CharType *dest, const encoding &enc)
{
	using unsigned_char = typename std::make_unsigned<CharType>::type;

	if(enc.flags & encoding::truncated_cells)
	{
		std::copy(begin, end, reinterpret_cast<unsigned_char*>(dest));
	}else{
		std::transform(begin, end, dest, [&](cell c)
		{
			if(static_cast<ucell>(c) > std::numeric_limits<unsigned_char>::max())
			{
				c = static_cast<unsigned char>(enc.unknown_char);
			}
			return static_cast<unsigned_char>(c);
		});
	}
}

template <class CharType, class Receiver>
void mark_end(Receiver &receiver)
{
	receiver(static_cast<const CharType*>(nullptr), static_cast<const CharType*>(nullptr));
}

using const_cell_span = std::pair<const cell*, const cell*>;

template <class CharType>
struct decoder_to_char_type
{
	// Outputs CharType (UTF-16 / UTF-32) directly from input

	template <class Receiver>
	struct from_raw
	{
		void operator()(Receiver receiver, const_cell_span input, const encoding &input_enc) const
		{
			CharType char_buffer[buffer_size];

			auto remaining = input.second - input.first;
			auto pointer = input.first;
			while(remaining > buffer_size)
			{
				copy_narrow(pointer, pointer + buffer_size, char_buffer, input_enc);

				receiver(
					static_cast<const CharType*>(char_buffer),
					static_cast<const CharType*>(char_buffer + buffer_size)
				);

				pointer += buffer_size;
				remaining -= buffer_size;
			}
			if(remaining > 0)
			{
				copy_narrow(pointer, pointer + remaining, char_buffer, input_enc);

				receiver(
					static_cast<const CharType*>(char_buffer),
					static_cast<const CharType*>(char_buffer + remaining)
				);
			}
			mark_end<CharType>(receiver);
		}
	};
};

template <class Receiver>
struct decoder_from_ansi_to_wchar_t
{
	// Conversion of cell -> char -> wchar_t from input

	void operator()(Receiver receiver, const_cell_span input, const encoding &input_enc) const
	{
		auto loc = input_enc.install();
		const auto &ctype = std::use_facet<std::ctype<cell>>(loc);
		const auto &cvt = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);

		char char_buffer[buffer_size];
		wchar_t wchar_buffer[buffer_size];
		cvt_state state{};

		auto remaining = input.second - input.first;
		auto pointer = input.first;
		while(remaining > buffer_size)
		{
			ctype.narrow(pointer, pointer + buffer_size, input_enc.unknown_char, char_buffer);

			encode_buffer_f(char, wchar_t, cvt, in)(cvt, input_enc, state, char_buffer, char_buffer + buffer_size, wchar_buffer, wchar_buffer + buffer_size, [&](const wchar_t *begin, const wchar_t *end)
			{
				return receiver(begin, end);
			});

			pointer += buffer_size;
			remaining -= buffer_size;
		}
		if(remaining > 0)
		{
			ctype.narrow(pointer, pointer + remaining, input_enc.unknown_char, char_buffer);

			encode_buffer_f(char, wchar_t, cvt, in)(cvt, input_enc, state, char_buffer, char_buffer + remaining, wchar_buffer, wchar_buffer + buffer_size, [&](const wchar_t *begin, const wchar_t *end)
			{
				return receiver(begin, end);
			});
		}
		encode_buffer_f(char, wchar_t, cvt, in)(cvt, input_enc, state, nullptr, nullptr, wchar_buffer, wchar_buffer + buffer_size, [&](const wchar_t *begin, const wchar_t *end)
		{
			return receiver(begin, end);
		});
		mark_end<wchar_t>(receiver);
	}
};

template <template <class> class U8CharDecoder>
struct decoder_from_utf8_decoder
{
	// Uses a nested decoder for UTF-8

	template <class WCharReceiver>
	struct through_wchar_t
	{
		// Conversion to wchar_t

		template <class... Args>
		void operator()(WCharReceiver receiver, const encoding &input_enc, Args&&... args) const
		{
			const auto &cvt = get_codecvt_utf8<wchar_t>(input_enc);
			wchar_t wchar_buffer[buffer_size];
			cvt_state state{};

			// Read UTF-8
			return call_coder<U8CharDecoder>([&](const u8char *begin, const u8char *end)
			{
				// Encode UTF-8 to wchar_t
				encode_buffer_f(char, wchar_t, cvt, in)(cvt, input_enc, state, reinterpret_cast<const char*>(begin), reinterpret_cast<const char*>(end), wchar_buffer, wchar_buffer + buffer_size, [&](const wchar_t *begin, const wchar_t *end)
				{
					return receiver(begin, end);
				});
			}, std::forward<Args>(args)...);
		}
	};
};

template <class CharType>
struct decoder_from_char_type
{
	// Converts from CharType (UTF-16 / UTF-32)

	using char_type = make_char_t<CharType>;

	template <class U8CharReceiver>
	struct through_utf8
	{
		// Conversion to UTF-8

		void operator()(U8CharReceiver receiver, const_cell_span input, const encoding &input_enc) const
		{
			const auto &cvt = get_codecvt_utf8<char_type>(input_enc);
			char_type char_buffer[buffer_size];
			u8char utf8_buffer[buffer_size];
			cvt_state state{};

			auto remaining = input.second - input.first;
			auto pointer = input.first;
			while(remaining > buffer_size)
			{
				copy_narrow(pointer, pointer + buffer_size, char_buffer, input_enc);

				// Encode from CharType to UTF-8
				encode_buffer_f(char_type, u8char, cvt, out)(cvt, input_enc, state, char_buffer, char_buffer + buffer_size, utf8_buffer, utf8_buffer + buffer_size, [&](const u8char *begin, const u8char *end)
				{
					return receiver(begin, end);
				});

				pointer += buffer_size;
				remaining -= buffer_size;
			}
			if(remaining > 0)
			{
				copy_narrow(pointer, pointer + remaining, char_buffer, input_enc);

				// Encode from CharType to UTF-8
				encode_buffer_f(char_type, u8char, cvt, out)(cvt, input_enc, state, char_buffer, char_buffer + remaining, utf8_buffer, utf8_buffer + buffer_size, [&](const u8char *begin, const u8char *end)
				{
					return receiver(begin, end);
				});
			}
			encode_buffer_f(char_type, u8char, cvt, out)(cvt, input_enc, state, nullptr, nullptr, utf8_buffer, utf8_buffer + buffer_size, [&](const u8char *begin, const u8char *end)
			{
				return receiver(begin, end);
			});
			mark_end<u8char>(receiver);
		}
	};

	template <class WCharReceiver>
	struct through_wchar_t : decoder_from_utf8_decoder<through_utf8>::template through_wchar_t<WCharReceiver>
	{
		// Conversion to wchar_t (through UTF-8)
	};
};

const_cell_span get_span(const cell_string &str)
{
	const cell *begin = &str[0];
	return {begin, begin + str.size()};
}

void strings::change_encoding(const cell_string &input, const encoding &input_enc, cell_string &output, const encoding &output_enc)
{
	if(input_enc.is_unicode())
	{
		if(output_enc.is_unicode() && input_enc.char_size() == output_enc.char_size())
		{
			// No conversion involved
			output = input;
			return;
		}
		const_cell_span input_span = get_span(input);
		if(input_enc.type == encoding::unicode)
		{
			// Treat as raw wchar_t and convert directly
			return encoder_from_wchar_t<decoder_to_char_type<wchar_t>::template from_raw>()(output, output_enc, input_span, input_enc);
		}
		if(output_enc.is_unicode())
		{
			// Through as UTF-8
			switch(input_enc.type)
			{
				case encoding::utf8:
					return encoder_from_utf8<decoder_to_char_type<u8char>::template from_raw>()(output, output_enc, input_span, input_enc);
				case encoding::utf16:
					return encoder_from_utf8<decoder_from_char_type<char16_t>::template through_utf8>()(output, output_enc, input_span, input_enc);
				case encoding::utf32:
					return encoder_from_utf8<decoder_from_char_type<char32_t>::template through_utf8>()(output, output_enc, input_span, input_enc);
			}
		}else{
			// Through as wchar_t
			switch(input_enc.type)
			{
				case encoding::utf8:
					return encoder_from_wchar_t<decoder_from_utf8_decoder<decoder_to_char_type<u8char>::template from_raw>::template through_wchar_t>()(output, output_enc, input_enc, input_span, input_enc);
				case encoding::utf16:
					return encoder_from_wchar_t<decoder_from_char_type<char16_t>::template through_wchar_t>()(output, output_enc, input_enc, input_span, input_enc);
				case encoding::utf32:
					return encoder_from_wchar_t<decoder_from_char_type<char32_t>::template through_wchar_t>()(output, output_enc, input_enc, input_span, input_enc);
			}
		}
	}

	// Always through as wchar_t
	return encoder_from_wchar_t<decoder_from_ansi_to_wchar_t>()(output, output_enc, get_span(input), input_enc);
}

template <template <class> class U8CharDecoder>
struct counter_from_utf8
{
	template <class... Args>
	size_t operator()(const encoding &enc, Args&&... args) const
	{
		size_t result = 0;

		size_t bom_part = (enc.flags & encoding::unicode_use_header) ? 0 : -1;

		// Reads UTF-8
		call_coder<U8CharDecoder>([&](const u8char *input_begin, const u8char *input_end)
		{
			// Count UTF-8
			while(input_begin != input_end)
			{
				u8char c = *input_begin;
				++input_begin;

				switch(bom_part)
				{
					case 0:
						if(c == '\xEF')
						{
							bom_part++;
							continue;
						}
						bom_part = -1;
						break;
					case 1:
						if(c == '\xBB')
						{
							bom_part++;
							continue;
						}
						bom_part = -1;
						break;
					case 2:
						if(c == '\xBF')
						{
							bom_part++;
							continue;
						}
						bom_part = -1;
						break;
				}

				if(!(c & 0x80))
				{
					// Single-byte character
					++result;
					continue;
				}
				// Multi-byte sequence
				if(c & 0x40)
				{
					// Initial byte in sequence
					++result;
				}
			}
			return nullptr;
		}, std::forward<Args>(args)...);

		return result;
	}
};

template <template <class> class WCharDecoder>
struct counter_from_wchar_t
{
	template <class... Args>
	size_t operator()(const encoding &enc, Args&&... args) const
	{
		return counter_from_utf8<encoder_from_wchar_t<WCharDecoder>::template to_utf8>()(enc, enc, std::forward<Args>(args)...);
	}
};

template <template <class> class U16CharDecoder>
struct counter_from_utf16
{
	template <class... Args>
	size_t operator()(const encoding &enc, Args&&... args) const
	{
		size_t result = 0;

		// Reads UTF-16
		call_coder<U16CharDecoder>([&](const char16_t *input_begin, const char16_t *input_end)
		{
			// Count UTF-16
			while(input_begin != input_end)
			{
				char16_t c = *input_begin;

				if(c < 0xDC00 || c > 0xDFFF)
				{
					// Not a low surrogate
					++result;
				}
				
				++input_begin;
			}
			return nullptr;
		}, std::forward<Args>(args)...);

		return result;
	}
};

size_t strings::count_chars(const cell *begin, const cell *end, const encoding &enc)
{
	if(enc.is_unicode() && enc.char_size() == sizeof(char32_t) && (enc.flags & encoding::unicode_use_header))
	{
		// No conversion involved
		return end - begin;
	}

	const_cell_span input_span{begin, end};

	switch(enc.type)
	{
		case encoding::unicode:
			// Treat as raw wchar_t and convert directly
			return counter_from_wchar_t<decoder_to_char_type<wchar_t>::template from_raw>()(enc, input_span, enc);
		case encoding::utf8:
			// Through as UTF-8
			return counter_from_utf8<decoder_to_char_type<u8char>::template from_raw>()(enc, input_span, enc);
		case encoding::utf16:
			if(enc.flags & (encoding::unicode_ucs | encoding::unicode_use_header))
			{
				// Decode to UTF-8
				return counter_from_utf8<decoder_from_char_type<char16_t>::template through_utf8>()(enc, input_span, enc);
			}
			return counter_from_utf16<decoder_to_char_type<char16_t>::template from_raw>()(enc, input_span, enc);
		case encoding::utf32:
			// Decode
			return counter_from_utf8<decoder_from_char_type<char32_t>::template through_utf8>()(enc, input_span, enc);
		default:
			// Always through as wchar_t
			return counter_from_wchar_t<decoder_from_ansi_to_wchar_t>()(enc, input_span, enc);
	}
}
