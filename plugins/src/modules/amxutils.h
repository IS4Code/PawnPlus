#ifndef AMXUTILS_H_INCLUDED
#define AMXUTILS_H_INCLUDED

#include "amxinfo.h"
#include "sdk/amx/amx.h"

struct fork_info_extra : public amx::extra
{
	cell result_address = -1;
	cell error_address = -1;

	fork_info_extra(AMX *amx) : amx::extra(amx)
	{

	}
};

#endif
