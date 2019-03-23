#include "natives.h"
#include "amxinfo.h"
#include "context.h"
#include "errors.h"
#include "modules/amxutils.h"
#include "modules/strings.h"
#include "modules/containers.h"
#include "utils/shared_id_set_pool.h"

cell pawn_call(AMX *amx, cell paramsize, cell *params, bool native, bool try_, AMX *target_amx);
AMX *source_amx;

namespace Natives
{
	// native Amx:amx_this();
	AMX_DEFINE_NATIVE(amx_this, 0)
	{
		return reinterpret_cast<cell>(amx);
	}

	// native Handle:amx_handle();
	AMX_DEFINE_NATIVE(amx_handle, 0)
	{
		return handle_pool.get_id(handle_pool.emplace(dyn_object(reinterpret_cast<cell>(amx), tags::find_tag("Amx")), amx::load(amx), true));
	}

	// native Amx:amx_source();
	AMX_DEFINE_NATIVE(amx_source, 0)
	{
		return reinterpret_cast<cell>(source_amx);
	}

	// native Handle:amx_source_handle();
	AMX_DEFINE_NATIVE(amx_source_handle, 0)
	{
		if(!source_amx) return 0;
		return handle_pool.get_id(handle_pool.emplace(dyn_object(reinterpret_cast<cell>(source_amx), tags::find_tag("Amx")), amx::load(source_amx), true));
	}

	// native amx_name(name[], size=sizeof(name));
	AMX_DEFINE_NATIVE(amx_name, 2)
	{
		const auto &name = amx::load_lock(amx)->name;
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		amx_SetString(addr, name.c_str(), false, false, params[2]);
		return name.size();
	}

	// native String:amx_name_s();
	AMX_DEFINE_NATIVE(amx_name_s, 0)
	{
		return strings::create(amx::load_lock(amx)->name);
	}

	cell amx_call(AMX *amx, cell *params, bool native, bool try_)
	{
		cell result = 0;
		switch(params[1])
		{
			case -1:
				amx::call_all([&](AMX *target_amx)
				{
					result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, target_amx);
				});
				break;
			case -2:
				amx::call_all([&](AMX *target_amx)
				{
					if(amx != target_amx)
					{
						result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, target_amx);
					}
				});
				break;
			default:
				auto target_amx = reinterpret_cast<AMX*>(params[1]);
				if(!amx::valid(target_amx)) amx_LogicError(errors::pointer_invalid, "AMX", params[1]);
				result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, target_amx);
				break;
		}
		return result;
	}

	// native amx_call_native(Amx:amx, const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(amx_call_native, 3)
	{
		return amx_call(amx, params, true, false);
	}

	// native amx_call_public(Amx:amx, const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(amx_call_public, 3)
	{
		return amx_call(amx, params, false, false);
	}

	// native amx_err:amx_try_call_native(Amx:amx, const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(amx_try_call_native, 4)
	{
		return amx_call(amx, params, true, true);
	}

	// native amx_err:amx_try_call_public(Amx:amx, const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(amx_try_call_public, 4)
	{
		return amx_call(amx, params, false, true);
	}

	// native Var:amx_var(&AnyTag:var);
	AMX_DEFINE_NATIVE(amx_var, 1)
	{
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, params[1], 1));
	}

	// native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
	AMX_DEFINE_NATIVE(amx_var_arr, 2)
	{
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, params[1], params[2]));
	}

	// native Var:amx_public_var(const name[]);
	AMX_DEFINE_NATIVE(amx_public_var, 1)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty);
		}

		cell amx_addr;
		if(amx_FindPubVar(amx, name, &amx_addr) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::var_not_found, "public", name);
		}
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, amx_addr, 1));
	}

	// native amx_set(Var:var, AnyTag:value, index=0);
	AMX_DEFINE_NATIVE(amx_set, 2)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->set(optparam(3, 0), params[2]);
	}

	// native amx_get(Var:var, index=0);
	AMX_DEFINE_NATIVE(amx_get, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->get(optparam(2, 0));
	}

	// native bool:amx_valid(Var:var);
	AMX_DEFINE_NATIVE(amx_valid, 1)
	{
		amx_var_info *info;
		return amx_var_pool.get_by_id(params[1], info);
	}

	// native amx_delete(Var:var);
	AMX_DEFINE_NATIVE(amx_delete, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return amx_var_pool.remove(info);
	}

	// native bool:amx_linked(Var:var);
	AMX_DEFINE_NATIVE(amx_linked, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->valid();
	}

	// native bool:amx_inside(Var:var);
	AMX_DEFINE_NATIVE(amx_inside, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->inside();
	}

	// native amx_sizeof(Var:var);
	AMX_DEFINE_NATIVE(amx_sizeof, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->size();
	}

	// native bool:amx_my(Var:var);
	AMX_DEFINE_NATIVE(amx_my, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->from_amx(amx);
	}

	// native amx_to_ref(Var:var, ref[1][]);
	AMX_DEFINE_NATIVE(amx_to_ref, 2)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		if(!info->from_amx(amx)) amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
		
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		*addr = info->address() - params[2];
		return 1;
	}

	// native bool:amx_fork(fork_level:level=fork_machine, &result=0, bool:use_data=true, &amx_err:error=amx_err:0);
	AMX_DEFINE_NATIVE(amx_fork, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		cell flags = optparam(1, 2) & SleepReturnForkFlagsMethodMask;
		if(optparam(3, 1))
		{
			flags |= SleepReturnForkFlagsCopyData;
		}
		amx::object owner;
		auto &extra = amx::get_context(amx, owner).get_extra<fork_info_extra>();
		extra.result_address = optparam(2, -1);
		extra.error_address = optparam(4, -1);
		return SleepReturnFork | flags;
	}

	// native amx_commit(bool:context=true);
	AMX_DEFINE_NATIVE(amx_commit, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkCommit | (SleepReturnValueMask & optparam(1, 1));
	}

	// native amx_fork_end();
	AMX_DEFINE_NATIVE(amx_fork_end, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkEnd;
	}

	// native amx_error(amx_err:code, result=0);
	AMX_DEFINE_NATIVE(amx_error, 1)
	{
		amx_RaiseError(amx, params[1]);
		return optparam(2, 0);
	}

	// native Var:amx_alloc(size, bool:zero=true);
	AMX_DEFINE_NATIVE(amx_alloc, 1)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return (optparam(2, 1) ? SleepReturnAllocVarZero : SleepReturnAllocVar) | (SleepReturnValueMask & params[1]);
	}

	// native bool:amx_free(Var:var);
	AMX_DEFINE_NATIVE(amx_free, 1)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		if(!info->from_amx(amx)) amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
			
		cell addr = info->free();
		if(addr != -1)
		{
			amx_RaiseError(amx, AMX_ERR_SLEEP);
			return SleepReturnFreeVar | (SleepReturnValueMask & addr);
		}

		amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
		return 0;
	}

	// native amx_parallel_begin(count=1);
	AMX_DEFINE_NATIVE(amx_parallel_begin, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnParallel | (SleepReturnValueMask & optparam(1, 1));
	}

	// native amx_parallel_end();
	AMX_DEFINE_NATIVE(amx_parallel_end, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnParallelEnd;
	}

	// native amx_tailcall();
	AMX_DEFINE_NATIVE(amx_tailcall, 0)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnTailCall;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(amx_this),
	AMX_DECLARE_NATIVE(amx_handle),
	AMX_DECLARE_NATIVE(amx_source),
	AMX_DECLARE_NATIVE(amx_source_handle),
	AMX_DECLARE_NATIVE(amx_name),
	AMX_DECLARE_NATIVE(amx_name_s),
	AMX_DECLARE_NATIVE(amx_call_native),
	AMX_DECLARE_NATIVE(amx_call_public),
	AMX_DECLARE_NATIVE(amx_try_call_native),
	AMX_DECLARE_NATIVE(amx_try_call_public),
	AMX_DECLARE_NATIVE(amx_var),
	AMX_DECLARE_NATIVE(amx_var_arr),
	AMX_DECLARE_NATIVE(amx_public_var),
	AMX_DECLARE_NATIVE(amx_set),
	AMX_DECLARE_NATIVE(amx_get),
	AMX_DECLARE_NATIVE(amx_valid),
	AMX_DECLARE_NATIVE(amx_delete),
	AMX_DECLARE_NATIVE(amx_linked),
	AMX_DECLARE_NATIVE(amx_inside),
	AMX_DECLARE_NATIVE(amx_alloc),
	AMX_DECLARE_NATIVE(amx_free),
	AMX_DECLARE_NATIVE(amx_sizeof),
	AMX_DECLARE_NATIVE(amx_my),
	AMX_DECLARE_NATIVE(amx_to_ref),
	AMX_DECLARE_NATIVE(amx_fork),
	AMX_DECLARE_NATIVE(amx_commit),
	AMX_DECLARE_NATIVE(amx_fork_end),
	AMX_DECLARE_NATIVE(amx_error),
	AMX_DECLARE_NATIVE(amx_parallel_begin),
	AMX_DECLARE_NATIVE(amx_parallel_end),
	AMX_DECLARE_NATIVE(amx_tailcall),
};

int RegisterAmxNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
