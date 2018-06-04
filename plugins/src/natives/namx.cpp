#include "natives.h"
#include "amxinfo.h"
#include "context.h"
#include "modules/amxutils.h"
#include "utils/set_pool.h"

class amx_var_info
{
	amx::handle _amx;
	cell _addr;
	cell _size;

public:
	amx_var_info(AMX *amx, cell addr, cell size) : _amx(amx::load(amx)), _addr(addr), _size(size)
	{

	}

	cell size()
	{
		return _size;
	}

	bool valid()
	{
		return !_amx.expired() && _amx.lock()->valid();
	}

	bool set(cell index, cell value)
	{
		if(index < 0 || index >= _size) return false;
		if(auto lock = _amx.lock())
		{
			cell *addr;
			if(amx_GetAddr(lock->get(), _addr + index, &addr) == AMX_ERR_NONE)
			{
				*addr = value;
			}
		}
		return false;
	}

	cell get(cell index)
	{
		if(index < 0 || index >= _size) return false;
		if(auto lock = _amx.lock())
		{
			cell *addr;
			if(amx_GetAddr(lock->get(), _addr + index, &addr) == AMX_ERR_NONE)
			{
				return *addr;
			}
		}
		return 0;
	}
};

aux::set_pool<amx_var_info> amx_var_pool;

namespace Natives
{
	// native AMX:amx_this();
	static cell AMX_NATIVE_CALL amx_this(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(amx);
	}

	// native Var:amx_var(&AnyTag:var);
	static cell AMX_NATIVE_CALL amx_var(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(amx_var_pool.add(amx_var_info(amx, params[1], 1)));
	}

	// native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
	static cell AMX_NATIVE_CALL amx_var_arr(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(amx_var_pool.add(amx_var_info(amx, params[1], params[2])));
	}

	// native amx_set(Var:var, AnyTag:value, index=0);
	static cell AMX_NATIVE_CALL amx_set(AMX *amx, cell *params)
	{
		auto info = reinterpret_cast<amx_var_info*>(params[1]);
		if(amx_var_pool.contains(info))
		{
			return info->set(params[3], params[2]);
		}
		return 0;
	}

	// native amx_get(Var:var, index=0);
	static cell AMX_NATIVE_CALL amx_get(AMX *amx, cell *params)
	{
		auto info = reinterpret_cast<amx_var_info*>(params[1]);
		if(amx_var_pool.contains(info))
		{
			return info->get(params[2]);
		}
		return 0;
	}

	// native bool:amx_valid(Var:var);
	static cell AMX_NATIVE_CALL amx_valid(AMX *amx, cell *params)
	{
		auto info = reinterpret_cast<amx_var_info*>(params[1]);
		if(amx_var_pool.contains(info))
		{
			return info->valid();
		}
		return 0;
	}

	// native bool:amx_delete(Var:var);
	static cell AMX_NATIVE_CALL amx_delete(AMX *amx, cell *params)
	{
		auto info = reinterpret_cast<amx_var_info*>(params[1]);
		return amx_var_pool.remove(info);
	}

	// native amx_sizeof(Var:var);
	static cell AMX_NATIVE_CALL amx_sizeof(AMX *amx, cell *params)
	{
		auto info = reinterpret_cast<amx_var_info*>(params[1]);
		if(amx_var_pool.contains(info))
		{
			return info->size();
		}
		return 0;
	}

	// native bool:amx_fork(&result=0, bool:clone=true, bool:use_data=true, &error=0);
	static cell AMX_NATIVE_CALL amx_fork(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		cell flags = SleepReturnFork;
		if(params[2])
		{
			flags |= SleepReturnForkFlagsClone;
		}
		if(params[3])
		{
			flags |= SleepReturnForkFlagsCopyData;
		}
		amx::object owner;
		auto &extra = amx::get_context(amx, owner).get_extra<fork_info_extra>();
		extra.result_address = params[1];
		extra.error_address = params[4];
		return SleepReturnFork | flags;
	}

	// native amx_commit(bool:context=true);
	static cell AMX_NATIVE_CALL amx_commit(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkCommit | (SleepReturnValueMask & params[1]);
	}

	// native amx_end_fork();
	static cell AMX_NATIVE_CALL amx_end_fork(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkEnd;
	}

	// native amx_error(amx_err:code, result=0);
	static cell AMX_NATIVE_CALL amx_error(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, params[1]);
		return params[2];
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(amx_this),
	AMX_DECLARE_NATIVE(amx_var),
	AMX_DECLARE_NATIVE(amx_var_arr),
	AMX_DECLARE_NATIVE(amx_set),
	AMX_DECLARE_NATIVE(amx_get),
	AMX_DECLARE_NATIVE(amx_valid),
	AMX_DECLARE_NATIVE(amx_delete),
	AMX_DECLARE_NATIVE(amx_sizeof),
	AMX_DECLARE_NATIVE(amx_fork),
	AMX_DECLARE_NATIVE(amx_commit),
	AMX_DECLARE_NATIVE(amx_end_fork),
	AMX_DECLARE_NATIVE(amx_error),
};

int RegisterAmxNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
