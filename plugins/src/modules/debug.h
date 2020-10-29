#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "amxinfo.h"
#include "sdk/amx/amxdbg.h"

namespace debug
{
	void init();
	void clear_file();
	std::shared_ptr<AMX_DBG> create_last(std::unique_ptr<char[]> &name);
}

#endif
