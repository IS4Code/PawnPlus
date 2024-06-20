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
		const std::ctype<char> *base;

	public:
		void set_base(const std::ctype<char> &base_facet)
		{
			this->base = &base_facet;
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
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return false;
			}
			return base->is(mask, static_cast<unsigned char>(ch));
		}

		char narrow(CharType ch, char dflt = '\0') const
		{
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return dflt;
			}
			return static_cast<unsigned char>(ch);
		}

		const char *narrow(const CharType *first, const CharType *last, char dflt, char *dest) const
		{
			return std::transform(first, last, dest, [&](CharType c) { return narrow(c, dflt); });
		}

		CharType widen(char c) const
		{
			return static_cast<unsigned char>(c);
		}

		const CharType *widen(const char *first, const char *last, CharType *dest) const
		{
			return std::transform(first, last, dest, [&](char c) { return widen(c); });
		}

		CharType tolower(CharType ch) const
		{
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return ch;
			}
			return static_cast<unsigned char>(base->tolower(static_cast<unsigned char>(ch)));
		}

		const CharType *tolower(CharType *first, const CharType *last) const
		{
			return std::transform(first, const_cast<CharType*>(last), first, [&](CharType c) { return tolower(c); });
		}

		CharType toupper(CharType ch) const
		{
			if(ch < 0 || ch > std::numeric_limits<unsigned char>::max())
			{
				return ch;
			}
			return static_cast<unsigned char>(base->toupper(static_cast<unsigned char>(ch)));
		}

		const CharType *toupper(CharType *first, const CharType *last) const
		{
			return std::transform(first, const_cast<CharType*>(last), first, [&](CharType c) { return tolower(c); });
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
