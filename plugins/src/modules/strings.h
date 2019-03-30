#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include "objects/object_pool.h"
#include "sdk/amx/amx.h"
#include <string>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <locale>

namespace strings
{
	typedef std::basic_string<cell> cell_string;
	extern cell null_value1[1];
	extern cell null_value2[2];
	extern object_pool<cell_string> pool;

	namespace impl
	{
		template <class Elem>
		class basic_char_iterator
		{
			std::intptr_t pos;

			basic_char_iterator(std::intptr_t pos) : pos(pos)
			{

			}

		public:
			typedef typename std::conditional<std::is_const<Elem>::value, const unsigned char, unsigned char>::type char_type;
			
			basic_char_iterator(Elem *it) : pos(reinterpret_cast<std::intptr_t>(it))
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

			basic_char_iterator<Elem>& operator++()
			{
				++pos;
				return *this;
			}

			basic_char_iterator<Elem> operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			basic_char_iterator<Elem>& operator--()
			{
				--pos;
				return *this;
			}

			basic_char_iterator<Elem> operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}

			basic_char_iterator<Elem>& operator+=(std::ptrdiff_t offset)
			{
				pos += offset;
				return *this;
			}

			basic_char_iterator<Elem> operator+(std::ptrdiff_t offset) const
			{
				return basic_char_iterator<Elem>(pos + offset);
			}

			basic_char_iterator<Elem> operator-(std::ptrdiff_t offset) const
			{
				return basic_char_iterator<Elem>(pos - offset);
			}

			std::ptrdiff_t operator-(const basic_char_iterator<Elem> &it) const
			{
				return pos - it.pos;
			}

			bool operator==(const basic_char_iterator<Elem> &it) const
			{
				return pos == it.pos;
			}

			bool operator!=(const basic_char_iterator<Elem> &it) const
			{
				return pos != it.pos;
			}

			bool operator<(const basic_char_iterator<Elem> &it) const
			{
				return pos < it.pos;
			}

			bool operator<=(const basic_char_iterator<Elem> &it) const
			{
				return pos <= it.pos;
			}

			bool operator>(const basic_char_iterator<Elem> &it) const
			{
				return pos > it.pos;
			}

			bool operator>=(const basic_char_iterator<Elem> &it) const
			{
				return pos >= it.pos;
			}
		};
	}

	typedef impl::basic_char_iterator<const cell> const_char_iterator;
	typedef impl::basic_char_iterator<cell> char_iterator;

	template <template <class> class Func, class... Args>
	auto select_iterator(const cell *str, Args &&...args) -> decltype(Func<const cell*>()(static_cast<const cell*>(nullptr), static_cast<const cell*>(nullptr), std::forward<Args>(args)...))
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
		}else{
			const_char_iterator it(str);
			return Func<const_char_iterator>()(it, it + len, std::forward<Args>(args)...);
		}
	}
	
	template <template <class> class Func, class... Args>
	auto select_iterator(const cell_string *str, Args &&...args) -> decltype(Func<const cell*>()(static_cast<const cell*>(nullptr), static_cast<const cell*>(nullptr), std::forward<Args>(args)...))
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

	void format(AMX *amx, strings::cell_string &str, const cell_string &format, cell argc, cell *args);
	void format(AMX *amx, strings::cell_string &str, const cell *format, cell argc, cell *args);

	cell_string convert(const cell *str);
	cell_string convert(const std::string &str);
	bool clamp_range(const cell_string &str, cell &start, cell &end);
	bool clamp_pos(const cell_string &str, cell &pos);

	void set_locale(const std::locale &loc);
	const std::string &locale_name();

	cell to_lower(cell c);
	cell to_upper(cell c);

	bool regex_search(const cell_string &str, const cell *pattern, cell options);
	bool regex_search(const cell_string &str, const cell_string &pattern, cell options);
	cell regex_extract(const cell_string &str, const cell *pattern, cell options);
	cell regex_extract(const cell_string &str, const cell_string &pattern, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell *pattern, const cell *replacement, cell options);
	void regex_replace(cell_string &target, const cell_string &str, const cell_string &pattern, const cell_string &replacement, cell options);
}

namespace std
{
	template <class Elem>
	struct iterator_traits<strings::impl::basic_char_iterator<Elem>>
	{
		typedef typename strings::impl::basic_char_iterator<Elem>::char_type &reference;
		typedef typename strings::impl::basic_char_iterator<Elem>::char_type *pointer;
		typedef Elem value_type;
		typedef std::ptrdiff_t difference_type;
		typedef std::random_access_iterator_tag iterator_category;
	};
}

#endif
