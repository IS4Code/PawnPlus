#ifndef INT_STRING_H_INCLUDED
#define INT_STRING_H_INCLUDED

#include <locale>
#include <type_traits>
#include <algorithm>

namespace impl
{
	template <class CharType>
	class ctype : public std::locale::facet
	{
		const std::ctype_base *base_ptr;
		enum
		{
			base_char,
			base_wchar_t,
			base_char16_t,
			base_char32_t
		} base_type;

		template <class Func>
		auto get_base(Func func) const -> decltype(func(*static_cast<const std::ctype<char>*>(base_ptr)))
		{
			switch(base_type)
			{
				case base_char:
					return func(*static_cast<const std::ctype<char>*>(base_ptr));
				case base_wchar_t:
					return func(*static_cast<const std::ctype<wchar_t>*>(base_ptr));
				case base_char16_t:
					return func(*static_cast<const std::ctype<char16_t>*>(base_ptr));
				case base_char32_t:
					return func(*static_cast<const std::ctype<char32_t>*>(base_ptr));
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
	public:
		void set_base(const std::ctype<char> &base_facet)
		{
			base_ptr = &base_facet;
			base_type = base_char;
		}

		void set_base(const std::ctype<wchar_t> &base_facet)
		{
			base_ptr = &base_facet;
			base_type = base_wchar_t;
		}

		void set_base(const std::ctype<char16_t> &base_facet)
		{
			base_ptr = &base_facet;
			base_type = base_char16_t;
		}

		void set_base(const std::ctype<char32_t> &base_facet)
		{
			base_ptr = &base_facet;
			base_type = base_char32_t;
		}

		typedef CharType char_type;

		enum
		{
			alnum = std::ctype_base::alnum,
			alpha = std::ctype_base::alpha,
			cntrl = std::ctype_base::cntrl,
			digit = std::ctype_base::digit,
			graph = std::ctype_base::graph,
			lower = std::ctype_base::lower,
			print = std::ctype_base::print,
			punct = std::ctype_base::punct,
			space = std::ctype_base::space,
			upper = std::ctype_base::upper,
			xdigit = std::ctype_base::xdigit
		};
		typedef std::ctype_base::mask mask;

		static std::locale::id id;

		bool is(mask mask, CharType ch) const
		{
			return get_base([&](const auto &base)
			{
				using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

				if(!char_in_range<unsigned_char>(ch))
				{
					return false;
				}
				return base.is(mask, static_cast<unsigned_char>(ch));
			});
		}

		template <class BaseCharType>
		char narrow(CharType ch, const std::ctype<BaseCharType> &base, char dflt = '\0') const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			if(!char_in_range<unsigned_char>(ch))
			{
				return dflt;
			}
			return base.narrow(static_cast<unsigned_char>(ch), dflt);
		}

		char narrow(CharType ch, char dflt = '\0') const
		{
			return get_base([&](const auto &base)
			{
				return narrow(ch, base, dflt);
			});
		}

		const char *narrow(const CharType *first, const CharType *last, char dflt, char *dest) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, last, dest, [&](CharType c) { return narrow(c, base, dflt); });
			});
		}

		template <class BaseCharType>
		CharType widen(char c, const std::ctype<BaseCharType> &base) const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			return static_cast<unsigned_char>(base.widen(c));
		}

		CharType widen(char c) const
		{
			return get_base([&](const auto &base)
			{
				return widen(c, base);
			});
		}

		const CharType *widen(const char *first, const char *last, CharType *dest) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, last, dest, [&](char c) { return widen(c, base); });
			});
		}

		template <class BaseCharType>
		CharType tolower(CharType ch, const std::ctype<BaseCharType> &base) const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			if(!char_in_range<unsigned_char>(ch))
			{
				return ch;
			}
			return static_cast<unsigned_char>(base.tolower(static_cast<unsigned_char>(ch)));
		}

		CharType tolower(CharType ch) const
		{
			return get_base([&](const auto &base)
			{
				return tolower(ch, base);
			});
		}

		const CharType *tolower(CharType *first, const CharType *last) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, const_cast<CharType*>(last), first, [&](CharType c) { return tolower(c, base); });
			});
		}

		template <class BaseCharType>
		CharType toupper(CharType ch, const std::ctype<BaseCharType> &base) const
		{
			using unsigned_char = unsigned_ctype_char_type<decltype(base)>;

			if(!char_in_range<unsigned_char>(ch))
			{
				return ch;
			}
			return static_cast<unsigned_char>(base.toupper(static_cast<unsigned_char>(ch)));
		}

		CharType toupper(CharType ch) const
		{
			return get_base([&](const auto &base)
			{
				return toupper(ch, base);
			});
		}

		const CharType *toupper(CharType *first, const CharType *last) const
		{
			return get_base([&](const auto &base)
			{
				return std::transform(first, const_cast<CharType*>(last), first, [&](CharType c) { return toupper(c, base); });
			});
		}
	};
}

namespace std
{
	template <>
	class ctype<std::int32_t> : public ::impl::ctype<std::int32_t>
	{

	};
}

#endif
