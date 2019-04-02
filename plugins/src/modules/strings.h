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
		class aligned_char_iterator
		{
			std::intptr_t pos;

			aligned_char_iterator(std::intptr_t pos) : pos(pos)
			{

			}

		public:
			typedef typename std::conditional<std::is_const<Elem>::value, const unsigned char, unsigned char>::type char_type;
			
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

	void set_locale(const std::locale &loc, cell category);
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
