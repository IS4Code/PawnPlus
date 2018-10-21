#ifndef EXEC_H_INCLUDED
#define EXEC_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"

extern int maxRecursionLevel;

int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, amx::reset *reset, bool forked = false);

// Holds the original code of the program (i.e. before relocation)
struct amx_code_info : public amx::extra
{
	std::unique_ptr<unsigned char[]> code;

	amx_code_info(AMX *amx);
};

#endif

