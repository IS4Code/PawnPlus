#include "int_string.h"

template <class CharType>
std::locale::id impl::int_ctype<CharType>::id;

template <class CharType, class Traits>
std::locale::id impl::char_ctype<CharType, Traits>::id;

template class impl::int_ctype<std::int32_t>;

template class impl::char_ctype<char8_t>;

template class impl::char_ctype<char16_t>;

template class impl::char_ctype<char32_t>;
