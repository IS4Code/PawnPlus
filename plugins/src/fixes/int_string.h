#ifndef INT_STRING_H_INCLUDED
#define INT_STRING_H_INCLUDED

#include "char8_t.h"
#include <locale>
#include <type_traits>
#include <algorithm>
#include <tuple>
#include <utility>
#include <string>

namespace std
{
	template <>
	class ctype<std::int32_t>;

	template <>
	class ctype<char8_t>;

	template <>
	class ctype<char16_t>;

	template <>
	class ctype<char32_t>;
}

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

		typename std::add_lvalue_reference<const BaseCType>::type base() const
		{
			return *base_ptr;
		}

		template <class CharType>
		const std::ctype<CharType> &base_derived() const
		{
			return *static_cast<const std::ctype<CharType>*>(base_ptr);
		}
	};

	template <class CharType>
	struct transform_helper
	{
		static CharType tolower(const std::ctype<CharType> &obj, CharType ch)
		{
			return obj.tolower(ch);
		}

		static CharType toupper(const std::ctype<CharType> &obj, CharType ch)
		{
			return obj.toupper(ch);
		}
	};

	template <class CharType>
	struct ctype_convert;

	template <class CharType>
	class int_ctype : public wrapper_ctype<void>
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
					return func(base_derived<char>());
				case base_wchar_t:
					return func(base_derived<wchar_t>());
				case base_char8_t:
					return func(base_derived<char8_t>());
				case base_char16_t:
					return func(base_derived<char16_t>());
				case base_char32_t:
					return func(base_derived<char32_t>());
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
		using ctype_transform = BaseCharType(*)(const std::ctype<BaseCharType>&, BaseCharType);

		template <class BaseCharType>
		struct empty
		{

		};

		decltype(base_type) base_type_from_param(empty<char>&&)
		{
			return base_char;
		}

		decltype(base_type) base_type_from_param(empty<wchar_t>&&)
		{
			return base_wchar_t;
		}

		decltype(base_type) base_type_from_param(empty<char8_t>&&)
		{
			return base_char8_t;
		}

		decltype(base_type) base_type_from_param(empty<char16_t>&&)
		{
			return base_char16_t;
		}

		decltype(base_type) base_type_from_param(empty<char32_t>&&)
		{
			return base_char32_t;
		}

	protected:
		template <class BaseCharType>
		void set_base(const std::locale &locale)
		{
			wrapper_ctype::set_base<BaseCharType>(locale);
			base_type = base_type_from_param(empty<BaseCharType>());
		}

		void change_base(const std::locale &locale)
		{
			get_base([&](const auto &base)
			{
				wrapper_ctype::set_base<ctype_char_type<decltype(base)>>(locale);
				return nullptr;
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
				return static_cast<unsigned_char>(Transform(base, static_cast<unsigned_char>(ch)));
			}

			if(!treat_as_truncated)
			{
				return ch;
			}

			char_type excess = ch ^ static_cast<unsigned_char>(ch);
			char_type result = static_cast<unsigned_char>(Transform(base, static_cast<unsigned_char>(ch)));
			return result | excess;
		}

		template <class BaseCharType>
		char_type tolower(char_type ch, const std::ctype<BaseCharType> &base) const
		{
			return transform<BaseCharType, &transform_helper<BaseCharType>::tolower>(ch, base);
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
			return transform<BaseCharType, &transform_helper<BaseCharType>::toupper>(ch, base);
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
		static bool can_cast_to_wchar(char8_t ch, bool)
		{
			return ch <= '\x7F';
		}

		static bool can_cast_from_wchar(wchar_t ch, bool)
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
		static bool can_cast_to_wchar(char16_t ch, bool allow_surrogates)
		{
			return sizeof(char16_t) == sizeof(wchar_t) || allow_surrogates || !is_surrogate(ch);
		}

		static bool can_cast_from_wchar(wchar_t ch, bool)
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
		static bool can_cast_to_wchar(char32_t ch, bool)
		{
			return ch <= std::numeric_limits<wchar_t>::max();
		}

		static bool can_cast_from_wchar(wchar_t ch, bool allow_surrogates)
		{
			return sizeof(wchar_t) == sizeof(char32_t) || allow_surrogates || !is_surrogate(ch);
		}
	};

	template <class CharType, class Traits = wchar_traits<CharType>>
	class char_ctype : public wrapper_ctype<std::ctype<wchar_t>>
	{
		using ctype_transform = wchar_t(*)(const std::ctype<wchar_t>&, wchar_t);

		bool flag;

	protected:
		char_ctype() : flag(false)
		{

		}

		char_ctype(bool flag) : flag(flag)
		{

		}

	public:
		typedef CharType char_type;

		static std::locale::id id;

		bool is(mask mask, char_type ch) const
		{
			if(!Traits::can_cast_to_wchar(ch, flag))
			{
				return false;
			}
			return base().is(mask, static_cast<wchar_t>(ch));
		}

		char narrow(char_type ch, char dflt = '\0') const
		{
			if(!Traits::can_cast_to_wchar(ch, flag))
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

			if(!Traits::can_cast_from_wchar(result, flag))
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
				std::transform(first, last, dest, [&](char c) { return widen(c); });
				return last;
			}
		}

		template <ctype_transform Transform>
		char_type transform(char_type ch) const
		{
			if(!Traits::can_cast_to_wchar(ch, flag))
			{
				return ch;
			}

			auto result = Transform(base(), static_cast<wchar_t>(ch));

			if(!Traits::can_cast_from_wchar(result, flag))
			{
				return ch;
			}

			return static_cast<char_type>(result);
		}

		char_type tolower(char_type ch) const
		{
			return transform<&transform_helper<wchar_t>::tolower>(ch);
		}

		char_type toupper(char_type ch) const
		{
			return transform<&transform_helper<wchar_t>::toupper>(ch);
		}
	};
}

namespace std
{
	template <class Type>
	struct hash<std::basic_string<Type>>
	{
		size_t operator()(const std::basic_string<Type> &obj) const
		{
			std::hash<Type> hasher;
			size_t seed = 0;
			for(const auto &c : obj)
			{
				seed ^= hasher(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
			return seed;
		}
	};

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
