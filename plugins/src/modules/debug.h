#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "amxinfo.h"
#include "sdk/amx/amxdbg.h"

namespace debug
{
	void init();
	AMX_DBG *create_last(std::unique_ptr<char[]> &name);

	struct info : public amx::extra
	{
		AMX_DBG *dbg;

		info(AMX *amx) : amx::extra(amx)
		{

		}

		virtual ~info() override
		{
			if(dbg)
			{
				dbg_FreeInfo(dbg);
				delete dbg;
				dbg = nullptr;
			}
		}
	};
}

#endif
