#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include "objects/object_pool.h"
#include "sdk/amx/amx.h"
#include "fixes/int_string.h"
#include <string>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <locale>
#include <memory>
#include <algorithm>
#include <utility>

class expression;

namespace strings
{
	typedef std::basic_string<cell> cell_string;
	extern cell null_value1[1];
	extern cell null_value2[2];
	extern object_pool<cell_string> pool;

	namespace impl
	{
		template <class Elem>
		class aligned_char_iterator
		{
			std::intptr_t pos;

			aligned_char_iterator(std::intptr_t pos) : pos(pos)
			{

			}

		public:
			typedef typename std::conditional<std::is_const<Elem>::value, const unsigned char, unsigned char>::type char_type;

			aligned_char_iterator() = default;
			
			aligned_char_iterator(Elem *it) : pos(reinterpret_cast<std::intptr_t>(it))
			{
				if(pos % sizeof(Elem) != 0)
				{
					throw std::invalid_argument("argument must be aligned");
				}
			}

			char_type &operator*() const
			{
				return *reinterpret_cast<char_type*>(pos ^ (sizeof(Elem) - 1));
			}

			char_type *operator->() const
			{
				return reinterpret_cast<char_type*>(pos ^ (sizeof(Elem) - 1));
			}

			aligned_char_iterator<Elem>& operator++()
			{
				++pos;
				return *this;
			}

			aligned_char_iterator<Elem> operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			aligned_char_iterator<Elem>& operator--()
			{
				--pos;
				return *this;
			}

			aligned_char_iterator<Elem> operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			aligned_char_iterator<Elem>& operator+=(std::ptrdiff_t offset)
			{
				pos += offset;
				return *this;
			}

			aligned_char_iterator<Elem> operator+(std::ptrdiff_t offset) const
			{
				return aligned_char_iterator<Elem>(pos + offset);
			}

			aligned_char_iterator<Elem> operator-(std::ptrdiff_t offset) const
			{
				return aligned_char_iterator<Elem>(pos - offset);
			}

			std::ptrdiff_t operator-(const aligned_char_iterator<Elem> &it) const
			{
				return pos - it.pos;
			}

			bool operator==(const aligned_char_iterator<Elem> &it) const
			{
				return pos == it.pos;
			}

			bool operator!=(const aligned_char_iterator<Elem> &it) const
			{
				return pos != it.pos;
			}

			bool operator<(const aligned_char_iterator<Elem> &it) const
			{
				return pos < it.pos;
			}

			bool operator<=(const aligned_char_iterator<Elem> &it) const
			{
				return pos <= it.pos;
			}

			bool operator>(const aligned_char_iterator<Elem> &it) const
			{
				return pos > it.pos;
			}

			bool operator>=(const aligned_char_iterator<Elem> &it) const
			{
				return pos >= it.pos;
			}
		};
	}

	typedef impl::aligned_char_iterator<const cell> aligned_const_char_iterator;
	typedef impl::aligned_char_iterator<cell> aligned_char_iterator;

	namespace impl
	{
		template <class Elem>
		class unaligned_char_iterator
		{
		public:
			typedef typename std::conditional<std::is_const<Elem>::value, const unsigned char, unsigned char>::type char_type;

		private:
			char_type *base;
			std::ptrdiff_t pos;

			unaligned_char_iterator(char_type *base, std::ptrdiff_t pos) : base(base), pos(pos)
			{

			}
			
		public:
			unaligned_char_iterator() = default;

			unaligned_char_iterator(Elem *it) : base(reinterpret_cast<char_type*>(it)), pos(0)
			{

			}

			char_type &operator*() const
			{
				return base[pos ^ (sizeof(Elem) - 1)];
			}

			char_type *operator->() const
			{
				return &base[pos ^ (sizeof(Elem) - 1)];
			}

			unaligned_char_iterator<Elem>& operator++()
			{
				++pos;
				return *this;
			}

			unaligned_char_iterator<Elem> operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			unaligned_char_iterator<Elem>& operator--()
			{
				--pos;
				return *this;
			}

			unaligned_char_iterator<Elem> operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			unaligned_char_iterator<Elem>& operator+=(std::ptrdiff_t offset)
			{
				pos += offset;
				return *this;
			}

			unaligned_char_iterator<Elem> operator+(std::ptrdiff_t offset) const
			{
				return unaligned_char_iterator<Elem>(base, pos + offset);
			}

			unaligned_char_iterator<Elem> operator-(std::ptrdiff_t offset) const
			{
				return unaligned_char_iterator<Elem>(base, pos - offset);
			}

			std::ptrdiff_t operator-(const unaligned_char_iterator<Elem> &it) const
			{
				if(base != it.base)
				{
					throw std::logic_error("different bases");
				}
				return pos - it.pos;
			}

			bool operator==(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos == it.base + it.pos;
			}

			bool operator!=(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos != it.base + it.pos;
			}

			bool operator<(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos < it.base + it.pos;
			}

			bool operator<=(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos <= it.base + it.pos;
			}

			bool operator>(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos > it.base + it.pos;
			}

			bool operator>=(const unaligned_char_iterator<Elem> &it) const
			{
				return base + pos >= it.base + it.pos;
			}
		};
	}

	typedef impl::unaligned_char_iterator<const cell> unaligned_const_char_iterator;
	typedef impl::unaligned_char_iterator<cell> unaligned_char_iterator;

	template <template <class> class Func, class... Args>
	auto select_iterator(const cell *str, Args &&...args) -> decltype(Func<const cell*>()(std::declval<const cell*>(), std::declval<const cell*>(), std::forward<Args>(args)...))
	{
		if(str == nullptr)
		{
			return Func<const cell*>()(nullptr, nullptr, std::forward<Args>(args)...);
		}
		int len;
		amx_StrLen(str, &len);
		if(static_cast<ucell>(*str) <= UNPACKEDMAX)
		{
			return Func<const cell*>()(str, str + len, std::forward<Args>(args)...);
		}else if(reinterpret_cast<std::intptr_t>(str) % sizeof(cell) == 0)
		{
			aligned_const_char_iterator it(str);
			return Func<aligned_const_char_iterator>()(it, it + len, std::forward<Args>(args)...);
		}else{
			unaligned_const_char_iterator it(str);
			return Func<unaligned_const_char_iterator>()(it, it + len, std::forward<Args>(args)...);
		}
	}
	
	template <template <class> class Func, class... Args>
	auto select_iterator(const cell_string *str, Args &&...args) -> decltype(Func<const cell*>()(std::declval<const cell*>(), std::declval<const cell*>(), std::forward<Args>(args)...))
	{
		if(str == nullptr)
		{
			return Func<const cell*>()(nullptr, nullptr, std::forward<Args>(args)...);
		}
		return Func<cell_string::const_iterator>()(str->cbegin(), str->cend(), std::forward<Args>(args)...);
	}

	cell create(const cell *addr, bool truncate, bool fixnulls);
	cell create(const cell *addr, size_t length, bool packed, bool truncate, bool fixnulls);
	cell create(const std::string &str);

	cell_string convert(const cell *str);
	cell_string convert(const cell *str, size_t length, bool packed);
	cell_string convert(const std::string &str);
	bool clamp_range(const cell_string &str, cell &start, cell &end);
	bool clamp_pos(const cell_string &str, cell &pos);

	template <class Locale>
	struct encoding_info
	{
		Locale locale;
		enum : unsigned char {
			unspecified,
			ansi,
			unicode,
			utf8,
			utf16,
			utf32
		} type;
		enum : unsigned char {
			truncated_cells = 1,
			unicode_ucs = 2,
			unicode_extended = 4,
			unicode_use_header = 8,
			unicode_native = 16,

			unicode_flags = unicode_ucs | unicode_extended | unicode_use_header | unicode_native
		} flags;
		char unknown_char;

	private:
		bool unmodified;

		template <class LocaleRef>
		encoding_info(std::nullptr_t, LocaleRef &&locale) : locale(std::forward<LocaleRef>(locale)), type{}, unmodified{false}, flags{}, unknown_char{'?'}
		{

		}

		template <class LocaleRef, class OtherLocale>
		encoding_info(std::nullptr_t, LocaleRef &&locale, const encoding_info<OtherLocale> &enc) : locale(std::forward<LocaleRef>(locale)), type{static_cast<decltype(type)>(enc.type)}, unmodified{false}, flags{static_cast<decltype(flags)>(enc.flags)}, unknown_char{enc.unknown_char}
		{

		}

		template <class Type>
		struct make_value
		{
			using type = Type;
		};

		template <class Obj>
		struct make_value<Obj*>
		{
			using type = std::basic_string<Obj>;
		};

		template <class Obj>
		struct make_value<const Obj*>
		{
			using type = std::basic_string<Obj>;
		};

	public:
		encoding_info(const Locale &locale) : encoding_info(nullptr, locale)
		{

		}

		encoding_info(Locale &&locale) : encoding_info(nullptr, std::move(locale))
		{

		}

		template <class OtherLocale>
		encoding_info(const Locale &locale, const encoding_info<OtherLocale> &enc) : encoding_info(nullptr, locale, enc)
		{

		}

		template <class OtherLocale>
		encoding_info(Locale &&locale, const encoding_info<OtherLocale> &enc) : encoding_info(nullptr, std::move(locale), enc)
		{

		}

		typename make_value<Locale>::type install() const;

		void fill_from_locale();

		bool is_unicode() const
		{
			switch(type)
			{
				case encoding_info::unicode:
				case encoding_info::utf8:
				case encoding_info::utf16:
				case encoding_info::utf32:
					return true;
			}
			return false;
		}

		std::size_t char_size() const
		{
			switch(type)
			{
				case encoding_info::ansi:
					return sizeof(char);
				case encoding_info::unicode:
					return sizeof(wchar_t);
				case encoding_info::utf8:
					return sizeof(char8_t);
				case encoding_info::utf16:
					return sizeof(char16_t);
				case encoding_info::utf32:
					return sizeof(char32_t);
				default:
					return 0;
			}
		}
	};

	using encoding = encoding_info<std::locale>;

	encoding find_encoding(char *&spec, bool default_if_empty);
	void set_encoding(const encoding &enc, cell category);
	void reset_locale();
	const std::string &locale_name();

	void to_lower(cell_string &str, const encoding &enc);
	void to_upper(cell_string &str, const encoding &enc);
	void change_encoding(const cell_string &input, const encoding &input_enc, cell_string &output, const encoding &output_enc);
	size_t count_chars(const cell *begin, const cell *end, const encoding &enc);
}

namespace std
{
	template <class Elem>
	struct iterator_traits<strings::impl::aligned_char_iterator<Elem>>
	{
		typedef typename strings::impl::aligned_char_iterator<Elem>::char_type &reference;
		typedef typename strings::impl::aligned_char_iterator<Elem>::char_type *pointer;
		typedef Elem value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::random_access_iterator_tag iterator_category;
	};

	template <class Elem>
	struct iterator_traits<strings::impl::unaligned_char_iterator<Elem>>
	{
		typedef typename strings::impl::unaligned_char_iterator<Elem>::char_type &reference;
		typedef typename strings::impl::unaligned_char_iterator<Elem>::char_type *pointer;
		typedef Elem value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::random_access_iterator_tag iterator_category;
	};
}

#endif
