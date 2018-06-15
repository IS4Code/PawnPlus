#include "amxutils.h"
#include <cstring>

cell amx_var_info::free()
{
	if(auto lock = _amx.lock())
	{
		if(lock->valid())
		{
			AMX *amx = lock->get();
			if(_addr >= amx->hlw && _addr < amx->hea)
			{
				return _addr;
			}
		}
	}
	return -1;
}

bool amx_var_info::valid()
{
	if(auto lock = _amx.lock())
	{
		return lock->valid();
	}
	return false;
}

bool amx_var_info::from_amx(AMX *amx)
{
	if(auto lock = _amx.lock())
	{
		return lock->get() == amx;
	}
	return false;
}

bool amx_var_info::set(cell index, cell value)
{
	if(index < 0 || index >= _size) return false;
	if(auto lock = _amx.lock())
	{
		if(lock->valid())
		{
			cell *addr;
			if(amx_GetAddr(lock->get(), _addr + index, &addr) == AMX_ERR_NONE)
			{
				*addr = value;
			}
		}
	}
	return false;
}

cell amx_var_info::get(cell index)
{
	if(index < 0 || index >= _size) return false;
	if(auto lock = _amx.lock())
	{
		if(lock->valid())
		{
			cell *addr;
			if(amx_GetAddr(lock->get(), _addr + index, &addr) == AMX_ERR_NONE)
			{
				return *addr;
			}
		}
	}
	return 0;
}

bool amx_var_info::fill(unsigned char value)
{
	if(auto lock = _amx.lock())
	{
		if(lock->valid())
		{
			AMX *amx = lock->get();
			if(_addr >= amx->hlw && _addr < amx->hea)
			{
				cell *addr;
				if(amx_GetAddr(amx, _addr, &addr) == AMX_ERR_NONE)
				{
					std::memset(addr, value, _size * sizeof(cell));
					return true;
				}
			}
		}
	}
	return false;
}
