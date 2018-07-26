#ifndef EXEC_H_INCLUDED
#define EXEC_H_INCLUDED

#include "objects/reset.h"
#include "sdk/amx/amx.h"

int AMXAPI amx_ExecContext(AMX *amx, cell *retval, int index, bool restore, amx::reset *reset, bool forked = false);

// Holds the original code of the program (i.e. before relocation)
struct amx_code_info : public amx::extra
{
	std::unique_ptr<unsigned char[]> code;

	amx_code_info(AMX *amx) : amx::extra(amx)
	{
		auto amxhdr = (AMX_HEADER*)amx->base;
		code = std::make_unique<unsigned char[]>(amxhdr->size - amxhdr->cod);
		std::memcpy(code.get(), amx->base + amxhdr->cod, amxhdr->size - amxhdr->cod);
	}
};

#endif

