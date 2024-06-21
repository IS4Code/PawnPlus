#include "int_string.h"

std::locale::id impl::int_ctype<std::int32_t>::id;
std::locale::id impl::char_ctype<char16_t>::id;
std::locale::id impl::char_ctype<char32_t>::id;
