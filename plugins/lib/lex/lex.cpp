// MIT License
//
// Copyright (c) 2020 PG1003
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "lex.h"
#include <cctype>

const char *pg::lex::detail::error_text(pg::lex::error_type code) noexcept
{
    switch(code)
    {
		case pg::lex::pattern_too_complex:
			return "pattern too complex";
		case pg::lex::pattern_ends_with_percent:
			return "malformed pattern (ends with '%c')";
		case pg::lex::pattern_missing_closing_bracket:
			return "malformed pattern (missing ']')";
		case pg::lex::balanced_no_arguments:
			return "malformed pattern (missing arguments to '%cb')";
		case pg::lex::frontier_no_open_bracket:
			return "missing '[' after '%cf' in pattern";
		case pg::lex::capture_too_many:
			return "too many captures";
		case pg::lex::capture_invalid_pattern:
			return "invalid pattern capture";
		case pg::lex::capture_invalid_index:
			return "invalid capture index";
		case pg::lex::capture_not_finished:
			return "unfinished capture";
		case pg::lex::capture_out_of_range:
			return "capture out of range";
		case pg::lex::percent_invalid_use_in_replacement:
			return "invalid use of '%%' in replacement string";
		default:
			return "lex error";
    }
}
