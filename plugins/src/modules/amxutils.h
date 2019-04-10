#ifndef AMXUTILS_H_INCLUDED
#define AMXUTILS_H_INCLUDED

#include "amxinfo.h"
#include "utils/shared_id_set_pool.h"
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
	amx_var_info() = default;

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
	bool inside();
	bool fill(unsigned char value);
};

extern aux::shared_id_set_pool<amx_var_info> amx_var_pool;

inline void amx_encode_value(cell *str, char signature, ucell value)
{
	str[0] = (static_cast<ucell>(static_cast<unsigned char>(signature)) << 24) | ((value / 4228250625 + 1 + 2 * (value % 127)) << 16) | ((value / 16581375 % 255 + 1) << 8) | (value / 65025 % 255 + 1);
	str[1] = ((value / 255 % 255 + 1) << 24) | ((value % 255 + 1) << 16);
}

inline void amx_encode_value(cell *str, char signature, ucell value1, ucell value2)
{
	amx_encode_value(str, signature, value1);
	str[1] |= ((value2 / 4228250625 + 1 + 2 * (value2 % 127)) << 8) | (value2 / 16581375 % 255 + 1);
	str[2] = ((value2 / 65025 % 255 + 1) << 24) | ((value2 / 255 % 255 + 1) << 16) | ((value2 % 255 + 1) << 8);
}

template <class Iter>
bool amx_decode_value(Iter &str, ucell &value)
{
	cell check = 0;
	for(size_t i = 0; i < 5; i++)
	{
		cell c = *str - 1;
		if(c < 0 || c >= 255)
		{
			return false;
		}
		if(i == 0)
		{
			check = c / 2;
			value = c % 2;
		}else{
			value = value * 255 + c;
		}
		++str;
	}
	return value % 127 == check;
}

#endif
