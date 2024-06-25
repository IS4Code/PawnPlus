#ifndef INT_STRING_H_INCLUDED
#define INT_STRING_H_INCLUDED

#include "char8_t.h"
#include <locale>
#include <type_traits>
#include <algorithm>
#include <tuple>
#include <utility>

namespace impl
{
	template <class BaseCType>
	class wrapper_ctype :
		public std::ctype_base,
		public std::conditional<
			std::is_base_of<std::locale::facet, std::ctype_base>::value,
			std::tuple<>, std::locale::facet
		>::type
	{
		const BaseCType *base_ptr;
		std::locale base_locale{std::locale::classic()};

	protected:
		wrapper_ctype() = default;

		wrapper_ctype(const wrapper_ctype<BaseCType> &obj) : base_ptr(obj.base_ptr), base_locale(obj.base_locale)
		{

		}

		wrapper_ctype(wrapper_ctype<BaseCType> &&obj) : base_ptr(obj.base_ptr), base_locale(std::move(obj.base_locale))
		{

		}

		template <class BaseCharType>
		void set_base(std::locale locale)
		{
			base_locale = std::move(locale);
			base_ptr = &std::use_facet<std::ctype<BaseCharType>>(base_locale);
		}

		const BaseCType &base() const
		{
			return *base_ptr;
		}
	};

	template <class CharType>
	class int_ctype : public wrapper_ctype<std::ctype_base>
	{
		enum
		{
			base_char,
			base_wchar_t,
			base_char8_t,
			base_char16_t,
			base_char32_t
		} base_type;
		bool treat_as_truncated;

		template <class Func>
		auto get_base(Func func) const -> decltype(func(std::declval<const std::ctype<char>>()))
		{
			switch(base_type)
			{
				case base_char:
					return func(static_cast<const std::ctype<char>&>(base()));
				case base_wchar_t:
					return func(static_cast<const std::ctype<wchar_t>&>(base()));
				case base_char8_t:
					return func(static_cast<const std::ctype<char8_t>&>(base()));
				case base_char16_t:
					return func(static_cast<const std::ctype<char16_t>&>(base()));
				case base_char32_t:
					return func(static_cast<const std::ctype<char32_t>&>(base()));
				default:
					throw std::logic_error("Unrecognized base character type.");
			}
		}

		template <class CType>
		using ctype_char_type = typename std::remove_cv<typename std::remove_reference<CType>::type>::type::char_type;

		template <class CType>
		using unsigned_ctype_char_type = typename std::make_unsigned<ctype_char_type<CType>>::type;

		template <class BaseCharType>
		static bool char_in_range(CharType ch)
		{
			static_assert(sizeof(BaseCharType) <= sizeof(CharType), "Underlying character type does not fit in a string element.");

			using unsigned_type = typename std::make_unsigned<CharType>::type;

			return static_cast<unsigned_type>(ch) <= std::numeric_limits<BaseCharType>::max();
		}

		template <class BaseCharType>
		using ctype_transform = BaseCharType(std::ctype<BaseCharType>::*)(BaseCharType) const;

	protected:
		template <class BaseCharType>
		void set_base(const std::locale &locale);

		template <>
		void set_base<char>(const std::locale &locale)
		{
			wrapper_ctype::set_base<char>(locale);
			base_type = base_char;
		}

		template <>
		void set_base<wchar_t>(const std::locale &locale)
		{
			wrapper_ctype::set_base<wchar_t>(locale);
			base_type = base_wchar_t;
		}

		template <>
		void set_base<char8_t>(const std::locale &locale)
		{
			wrapper_ctype::set_base<char8_t>(locale);
			base_type = base_char8_t;
		}

		template <>
		void set_base<char16_t>(const std::locale &locale)
		{
			wrapper_ctype::set_base<char16_t>(locale);
			base_type = base_char16_t;
		}

		template <>
		void set_base<char32_t>(const std::locale &locale)
		{
			wrapper_ctype::set_base<char32_t>(locale);
			base_type = base_char32_t;
		}

		void change_base(const std::locale &locale)
		{
			return get_base([&](const auto &base)
			{
				wrapper_ctype::set_base<ctype_char_type<decltype(base)>>(locale);
			});
		}

		int_ctype() : treat_as_truncated(false)
		{

		}

		int_ctype(bool treat_as_truncated) : treat_as_truncated(treat_as_truncated)
		{

		}

	public:
		typedef CharType char_type;

		static std::locale::id id;

		bool is(mask mask, char_type ch) const
		{
			return get_base([&](const auto &base)
			{
				using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

				if(treat_as_truncated || char_in_range<unsigned_char>(ch))
				{
					return base.is(mask, static_cast<unsigned_char>(ch));
				}
				return false;
			});
		}

		template <class BaseCharType>
		char narrow(char_type ch, const std::ctype<BaseCharType> &base, char dflt = '\0') const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			if(treat_as_truncated || char_in_range<unsigned_char>(ch))
			{
				return base.narrow(static_cast<unsigned_char>(ch), dflt);
			}
			return dflt;
		}

		char narrow(char_type ch, char dflt = '\0') const
		{
			return get_base([&](const auto &base)
			{
				return narrow(ch, base, dflt);
			});
		}

		const char_type *narrow(const char_type *first, const char_type *last, char dflt, char *dest) const
		{
			return get_base([&](const auto &base)
			{
				std::transform(first, last, dest, [&](char_type c) { return narrow(c, base, dflt); });
				return last;
			});
		}

		template <class BaseCharType>
		char_type widen(char c, const std::ctype<BaseCharType> &base) const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			return static_cast<unsigned_char>(base.widen(c));
		}

		char_type widen(char c) const
		{
			return get_base([&](const auto &base)
			{
				return widen(c, base);
			});
		}

		const char *widen(const char *first, const char *last, char_type *dest) const
		{
			return get_base([&](const auto &base)
			{
				std::transform(first, last, dest, [&](char c) { return widen(c, base); });
				return last;
			});
		}

		template <class BaseCharType, ctype_transform<BaseCharType> Transform>
		char_type transform(char_type ch, const std::ctype<BaseCharType> &base) const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			if(char_in_range<unsigned_char>(ch))
			{
				return static_cast<unsigned_char>((base.*Transform)(static_cast<unsigned_char>(ch)));
			}

			if(!treat_as_truncated)
			{
				return ch;
			}

			char_type excess = ch ^ static_cast<unsigned_char>(ch);
			char_type result = static_cast<unsigned_char>((base.*Transform)(static_cast<unsigned_char>(ch)));
			return result | excess;
		}

		template <class BaseCharType>
		char_type tolower(char_type ch, const std::ctype<BaseCharType> &base) const
		{
			return transform<BaseCharType, static_cast<ctype_transform<BaseCharType>>(&std::ctype<BaseCharType>::tolower)>(ch, base);
		}

		char_type tolower(char_type ch) const
		{
			return get_base([&](const auto &base)
			{
				return tolower(ch, base);
			});
		}

		const char_type *tolower(char_type *first, const char_type *last) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, const_cast<char_type*>(last), first, [&](char_type c) { return tolower(c, base); });
			});
		}

		template <class BaseCharType>
		char_type toupper(char_type ch, const std::ctype<BaseCharType> &base) const
		{
			return transform<BaseCharType, static_cast<ctype_transform<BaseCharType>>(&std::ctype<BaseCharType>::toupper)>(ch, base);
		}

		char_type toupper(char_type ch) const
		{
			return get_base([&](const auto &base)
			{
				return toupper(ch, base);
			});
		}

		const char_type *toupper(char_type *first, const char_type *last) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, const_cast<char_type*>(last), first, [&](char_type c) { return toupper(c, base); });
			});
		}
	};

	template <class CharType>
	class wchar_traits;

	template <>
	class wchar_traits<char8_t>
	{
	public:
		static bool can_cast_to_wchar(char8_t ch)
		{
			return ch <= '\x7F';
		}

		static bool can_cast_from_wchar(wchar_t ch)
		{
			return ch <= L'\x7F';
		}
	};

	template <>
	class wchar_traits<char16_t>
	{
		static bool is_surrogate(char16_t ch)
		{
			return ch >= u'\xD800' && ch <= u'\xDFFF';
		}

	public:
		static bool can_cast_to_wchar(char16_t ch)
		{
			return sizeof(char16_t) == sizeof(wchar_t) || !is_surrogate(ch);
		}

		static bool can_cast_from_wchar(wchar_t ch)
		{
			return ch <= std::numeric_limits<char16_t>::max();
		}
	};

	template <>
	class wchar_traits<char32_t>
	{
		static bool is_surrogate(wchar_t ch)
		{
			return ch >= L'\xD800' && ch <= L'\xDFFF';
		}

	public:
		static bool can_cast_to_wchar(char32_t ch)
		{
			return ch <= std::numeric_limits<wchar_t>::max();
		}

		static bool can_cast_from_wchar(wchar_t ch)
		{
			return sizeof(wchar_t) == sizeof(char32_t) || !is_surrogate(ch);
		}
	};

	template <class CharType, class Traits = wchar_traits<CharType>>
	class char_ctype : public wrapper_ctype<std::ctype<wchar_t>>
	{
		using ctype_transform = wchar_t(std::ctype<wchar_t>::*)(wchar_t) const;

	protected:
		char_ctype()
		{

		}

	public:
		typedef CharType char_type;

		static std::locale::id id;

		bool is(mask mask, char_type ch) const
		{
			if(!Traits::can_cast_to_wchar(ch))
			{
				return false;
			}
			return base().is(mask, static_cast<wchar_t>(ch));
		}

		char narrow(char_type ch, char dflt = '\0') const
		{
			if(!Traits::can_cast_to_wchar(ch))
			{
				return dflt;
			}
			return base().narrow(static_cast<wchar_t>(ch), dflt);
		}

		const char_type *narrow(const char_type *first, const char_type *last, char dflt, char *dest) const
		{
			if(sizeof(char_type) == sizeof(wchar_t))
			{
				return reinterpret_cast<const char_type*>(base().narrow(reinterpret_cast<const wchar_t*>(first), reinterpret_cast<const wchar_t*>(last), dflt, dest));
			}else{
				std::transform(first, last, dest, [&](char_type c) { return narrow(c, dflt); });
				return last;
			}
		}

		char_type widen(char c) const
		{
			auto result = base().widen(c);

			if(!Traits::can_cast_from_wchar(result))
			{
				return -1;
			}

			return static_cast<char_type>(result);
		}

		const char *widen(const char *first, const char *last, char_type *dest) const
		{
			if(sizeof(char_type) == sizeof(wchar_t))
			{
				return base().widen(first, last, reinterpret_cast<wchar_t*>(dest));
			}else{
				std::transform(first, last, dest, [&](char_type c) { return widen(c); });
				return last;
			}
		}

		template <ctype_transform Transform>
		char_type transform(char_type ch) const
		{
			if(!Traits::can_cast_to_wchar(ch))
			{
				return ch;
			}

			auto result = (base().*Transform)(static_cast<wchar_t>(ch));

			if(!Traits::can_cast_from_wchar(result))
			{
				return ch;
			}

			return static_cast<char_type>(result);
		}

		char_type tolower(char_type ch) const
		{
			return transform<static_cast<ctype_transform>(&std::ctype<wchar_t>::tolower)>(ch);
		}

		char_type toupper(char_type ch) const
		{
			return transform<static_cast<ctype_transform>(&std::ctype<wchar_t>::toupper)>(ch);
		}
	};
}

namespace std
{
	template <>
	class ctype<std::int32_t> : public ::impl::int_ctype<std::int32_t>
	{
		template <class... Args>
		ctype(Args&&... args) : int_ctype(std::forward<Args>(args)...)
		{

		}

	public:
		template <class BaseCharType, class... Args>
		static const ctype<char_type> &install(std::locale &locale, Args&&... args)
		{
			auto facet = new ctype<char_type>(std::forward<Args>(args)...);
			locale = std::locale(locale, facet);
			facet->set_base<BaseCharType>(locale);
			return *facet;
		}

		static const ctype<char_type> &install(std::locale &locale, const ctype<char_type> &obj)
		{
			auto facet = new ctype<char_type>(obj);
			locale = std::locale(locale, facet);
			facet->change_base(locale);
			return *facet;
		}
	};

	template <>
	class ctype<char8_t> : public ::impl::char_ctype<char8_t>
	{
		template <class... Args>
		ctype(Args&&... args) : char_ctype(std::forward<Args>(args)...)
		{

		}

	public:
		template <class... Args>
		static const ctype<char_type> &install(std::locale &locale, Args&&... args)
		{
			auto facet = new ctype<char_type>(std::forward<Args>(args)...);
			locale = std::locale(locale, facet);
			facet->set_base<wchar_t>(locale);
			return *facet;
		}
	};

	template <>
	class ctype<char16_t> : public ::impl::char_ctype<char16_t>
	{
		template <class... Args>
		ctype(Args&&... args) : char_ctype(std::forward<Args>(args)...)
		{

		}

	public:
		template <class... Args>
		static const ctype<char_type> &install(std::locale &locale, Args&&... args)
		{
			auto facet = new ctype<char_type>(std::forward<Args>(args)...);
			locale = std::locale(locale, facet);
			facet->set_base<wchar_t>(locale);
			return *facet;
		}
	};

	template <>
	class ctype<char32_t> : public ::impl::char_ctype<char32_t>
	{
		template <class... Args>
		ctype(Args&&... args) : char_ctype(std::forward<Args>(args)...)
		{

		}

	public:
		template <class... Args>
		static const ctype<char_type> &install(std::locale &locale, Args&&... args)
		{
			auto facet = new ctype<char_type>(std::forward<Args>(args)...);
			locale = std::locale(locale, facet);
			facet->set_base<wchar_t>(locale);
			return *facet;
		}
	};
}

#endif
