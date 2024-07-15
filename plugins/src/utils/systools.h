#ifndef SYSTOOLS_H_INCLUDED
#define SYSTOOLS_H_INCLUDED

#include <cstddef>
#include <ctime>

namespace aux
{
	std::size_t stackspace();
	std::tm localtime(const std::time_t &time);
	std::tm gmtime(const std::time_t &time);
}

#endif
