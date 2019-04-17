#include "amxutils.h"
#include "errors.h"
#include <cstring>

aux::shared_id_set_pool<amx_var_info> amx_var_pool;

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
	if(index >= 0 && index < _size)
	{
		if(auto lock = _amx.lock())
		{
			if(lock->valid())
			{
				auto amx = lock->get();
				cell amx_addr = _addr + index * sizeof(cell);
				if((amx_addr >= 0 && amx_addr < amx->hea) || (amx_addr >= amx->stk && amx_addr < amx->stp))
				{
					cell *addr;
					if(amx_GetAddr(amx, amx_addr, &addr) == AMX_ERR_NONE)
					{
						*addr = value;
						return true;
					}
				}
			}
		}
	}
	amx_LogicError(errors::out_of_range, "index");
	return false;
}

cell amx_var_info::get(cell index)
{
	if(index >= 0 && index < _size)
	{
		if(auto lock = _amx.lock())
		{
			if(lock->valid())
			{
				auto amx = lock->get();
				cell amx_addr = _addr + index * sizeof(cell);
				if((amx_addr >= 0 && amx_addr < amx->hea) || (amx_addr >= amx->stk && amx_addr < amx->stp))
				{
					cell *addr;
					if(amx_GetAddr(amx, amx_addr, &addr) == AMX_ERR_NONE)
					{
						return *addr;
					}
				}
			}
		}
	}
	amx_LogicError(errors::out_of_range, "index");
	return 0;
}

bool amx_var_info::inside()
{
	if(auto lock = _amx.lock())
	{
		if(lock->valid())
		{
			auto amx = lock->get();
			cell amx_addr1 = _addr;
			cell amx_addr2 = _addr + _size - 1;
			if((amx_addr1 >= 0 && amx_addr1 < amx->hea) || (amx_addr1 >= amx->stk && amx_addr1 < amx->stp))
			{
				if((amx_addr2 >= 0 && amx_addr2 < amx->hea) || (amx_addr2 >= amx->stk && amx_addr2 < amx->stp))
				{
					return true;
				}
			}
		}
	}
	return false;
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
