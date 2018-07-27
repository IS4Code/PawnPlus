#include "natives.h"
#include "amxinfo.h"
#include "context.h"
#include "modules/amxutils.h"
#include "utils/id_set_pool.h"

aux::id_set_pool<amx_var_info> amx_var_pool;

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
		return amx_var_pool.get_id(amx_var_pool.add(amx_var_info(amx, params[1], 1)));
	}

	// native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
	static cell AMX_NATIVE_CALL amx_var_arr(AMX *amx, cell *params)
	{
		return amx_var_pool.get_id(amx_var_pool.add(amx_var_info(amx, params[1], params[2])));
	}

	// native amx_set(Var:var, AnyTag:value, index=0);
	static cell AMX_NATIVE_CALL amx_set(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			return info->set(params[3], params[2]);
		}
		return 0;
	}

	// native amx_get(Var:var, index=0);
	static cell AMX_NATIVE_CALL amx_get(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			return info->get(params[2]);
		}
		return 0;
	}

	// native bool:amx_valid(Var:var);
	static cell AMX_NATIVE_CALL amx_valid(AMX *amx, cell *params)
	{
		amx_var_info *info;
		return amx_var_pool.get_by_id(params[1], info);
	}

	// native bool:amx_delete(Var:var);
	static cell AMX_NATIVE_CALL amx_delete(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			return amx_var_pool.remove(info);
		}
		return 0;
	}

	// native amx_sizeof(Var:var);
	static cell AMX_NATIVE_CALL amx_sizeof(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			return info->size();
		}
		return 0;
	}

	// native bool:amx_my(Var:var);
	static cell AMX_NATIVE_CALL amx_my(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			return info->from_amx(amx);
		}
		return 0;
	}

	// native bool:amx_to_ref(Var:var, ref[1][]);
	static cell AMX_NATIVE_CALL amx_to_ref(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info) && info->from_amx(amx))
		{
			cell *addr;
			amx_GetAddr(amx, params[2], &addr);
			*addr = info->address() - params[2];
			return 1;
		}
		return 0;
	}

	// native bool:amx_fork(fork_level:level=fork_script, &result=0, bool:use_data=true, &amx_err:error=amx_err:0);
	static cell AMX_NATIVE_CALL amx_fork(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		cell flags = params[1] & SleepReturnForkFlagsMethodMask;
		if(params[3])
		{
			flags |= SleepReturnForkFlagsCopyData;
		}
		amx::object owner;
		auto &extra = amx::get_context(amx, owner).get_extra<fork_info_extra>();
		extra.result_address = params[2];
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

	// native Var:amx_alloc(size, bool:zero=true);
	static cell AMX_NATIVE_CALL amx_alloc(AMX *amx, cell *params)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return (params[2] ? SleepReturnAllocVarZero : SleepReturnAllocVar) | (SleepReturnValueMask & params[1]);
	}

	// native bool:amx_free(Var:var);
	static cell AMX_NATIVE_CALL amx_free(AMX *amx, cell *params)
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(params[1], info))
		{
			if(info->from_amx(amx))
			{
				amx_RaiseError(amx, AMX_ERR_SLEEP);
				cell addr = info->free();
				if(addr != -1)
				{
					return SleepReturnFreeVar | (SleepReturnValueMask & addr);
				}
			}
		}
		return 0;
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
	AMX_DECLARE_NATIVE(amx_alloc),
	AMX_DECLARE_NATIVE(amx_free),
	AMX_DECLARE_NATIVE(amx_sizeof),
	AMX_DECLARE_NATIVE(amx_my),
	AMX_DECLARE_NATIVE(amx_to_ref),
	AMX_DECLARE_NATIVE(amx_fork),
	AMX_DECLARE_NATIVE(amx_commit),
	AMX_DECLARE_NATIVE(amx_end_fork),
	AMX_DECLARE_NATIVE(amx_error),
};

int RegisterAmxNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
