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

class amx_var_info
{
	amx::handle _amx;
	cell _addr;
	cell _size;

public:
	amx_var_info(AMX *amx, cell addr, cell size) : _amx(amx::load(amx)), _addr(addr), _size(size)
	{

	}

	cell address()
	{
		return _addr;
	}

	cell size()
	{
		return _size;
	}

	cell free();
	bool valid();
	bool from_amx(AMX *amx);
	bool set(cell index, cell value);
	cell get(cell index);
	bool fill(unsigned char value);
};

#endif
