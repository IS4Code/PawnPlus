#ifndef CHAR8_T_H_INCLUDED
#define CHAR8_T_H_INCLUDED

#include <cstdint>
#include <limits>
#include <type_traits>
#include <string>

#ifndef __cpp_char8_t
struct alignas(unsigned char) char8_t
{
private:
	unsigned char value;

public:
	char8_t() noexcept
	{

	}

	constexpr char8_t(unsigned char value) noexcept : value(value)
	{

	}

	constexpr explicit char8_t(char value) noexcept : value(value)
	{

	}

	constexpr explicit char8_t(wchar_t value) noexcept : value(static_cast<unsigned char>(value))
	{

	}

	constexpr explicit operator unsigned char() const noexcept
	{
		return value;
	}

	constexpr explicit operator char() const noexcept
	{
		return value;
	}

	constexpr explicit operator wchar_t() const noexcept
	{
		return value;
	}

	char8_t(const char8_t &other) = default;
	char8_t(char8_t &&other) = default;
	char8_t &operator=(const char8_t &other) = default;
	char8_t &operator=(char8_t &&other) = default;

	constexpr bool operator==(char8_t c) const noexcept
	{
		return value == c.value;
	}

	constexpr bool operator!=(char8_t c) const noexcept
	{
		return value != c.value;
	}

	constexpr bool operator<(char8_t c) const noexcept
	{
		return value < c.value;
	}

	constexpr bool operator>(char8_t c) const noexcept
	{
		return value > c.value;
	}

	constexpr bool operator<=(char8_t c) const noexcept
	{
		return value <= c.value;
	}

	constexpr bool operator>=(char8_t c) const noexcept
	{
		return value >= c.value;
	}

	constexpr char8_t operator+(char8_t c) const noexcept
	{
		return value + c.value;
	}

	constexpr char8_t operator-(char8_t c) const noexcept
	{
		return value - c.value;
	}

	constexpr char8_t operator*(char8_t c) const noexcept
	{
		return value * c.value;
	}

	constexpr char8_t operator/(char8_t c) const noexcept
	{
		return value / c.value;
	}

	constexpr char8_t operator%(char8_t c) const noexcept
	{
		return value % c.value;
	}

	char8_t &operator+=(char8_t c) noexcept
	{
		value += c.value;
		return *this;
	}

	char8_t &operator-=(char8_t c) noexcept
	{
		value -= c.value;
		return *this;
	}

	char8_t &operator*=(char8_t c) noexcept
	{
		value *= c.value;
		return *this;
	}

	char8_t &operator/=(char8_t c) noexcept
	{
		value /= c.value;
		return *this;
	}

	char8_t &operator%=(char8_t c) noexcept
	{
		value %= c.value;
		return *this;
	}
};

namespace std
{
	template <>
	class numeric_limits<char8_t>
	{
	public:
		static constexpr char8_t min() noexcept
		{
			return 0;
		}

		static constexpr char8_t max() noexcept
		{
			return std::numeric_limits<unsigned char>::max();
		}

		static constexpr char8_t lowest() noexcept
		{
			return 0;
		}

		static constexpr bool is_modulo = true;
		static constexpr int digits = std::numeric_limits<unsigned char>::digits;
		static constexpr int digits10 = std::numeric_limits<unsigned char>::digits10;
	};

	template <>
	struct is_integral<char8_t> : public std::true_type
	{

	};

	template <>
	struct is_unsigned<char8_t> : public std::true_type
	{

	};

	template <>
	struct is_signed<char8_t> : public std::false_type
	{

	};

	template <>
	struct make_signed<char8_t>
	{
		using type = signed char;
	};

	template <>
	struct make_unsigned<char8_t>
	{
		using type = unsigned char;
	};

	template <>
	struct hash<char8_t>
	{
		hash<unsigned char> inner;

		std::size_t operator()(const char8_t &c) const
		{
			return inner(static_cast<unsigned char>(c));
		}
	};
}
#endif

#endif
