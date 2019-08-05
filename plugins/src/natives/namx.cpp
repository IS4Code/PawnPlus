#include "natives.h"
#include "amxinfo.h"
#include "context.h"
#include "errors.h"
#include "modules/amxutils.h"
#include "modules/strings.h"
#include "modules/containers.h"
#include "modules/guards.h"
#include "utils/shared_id_set_pool.h"
#include <limits>

cell pawn_call(AMX *amx, cell paramsize, cell *params, bool native, bool try_, std::string *msg, AMX *target_amx);
AMX *source_amx;

namespace Natives
{
	// native Amx:amx_this();
	AMX_DEFINE_NATIVE_TAG(amx_this, 0, cell)
	{
		return reinterpret_cast<cell>(amx);
	}

	// native Handle:amx_handle();
	AMX_DEFINE_NATIVE_TAG(amx_handle, 0, handle)
	{
		return handle_pool.get_id(handle_pool.emplace(dyn_object(reinterpret_cast<cell>(amx), tags::find_tag("Amx")), amx::load(amx), true));
	}

	// native Amx:amx_source();
	AMX_DEFINE_NATIVE_TAG(amx_source, 0, cell)
	{
		return reinterpret_cast<cell>(source_amx);
	}

	// native Handle:amx_source_handle();
	AMX_DEFINE_NATIVE_TAG(amx_source_handle, 0, handle)
	{
		if(!source_amx) return 0;
		return handle_pool.get_id(handle_pool.emplace(dyn_object(reinterpret_cast<cell>(source_amx), tags::find_tag("Amx")), amx::load(source_amx), true));
	}

	// native amx_name(name[], size=sizeof(name));
	AMX_DEFINE_NATIVE_TAG(amx_name, 2, cell)
	{
		const auto &name = amx::load_lock(amx)->name;
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		amx_SetString(addr, name.c_str(), false, false, params[2]);
		return name.size();
	}

	// native String:amx_name_s();
	AMX_DEFINE_NATIVE_TAG(amx_name_s, 0, string)
	{
		return strings::create(amx::load_lock(amx)->name);
	}

	cell amx_call(AMX *amx, cell *params, bool native, bool try_, std::string *msg)
	{
		cell result = 0;
		switch(params[1])
		{
			case -1:
				amx::call_all([&](AMX *target_amx)
				{
					result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, msg, target_amx);
				});
				break;
			case -2:
				amx::call_all([&](AMX *target_amx)
				{
					if(amx != target_amx)
					{
						result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, msg, target_amx);
					}
				});
				break;
			default:
				auto target_amx = reinterpret_cast<AMX*>(params[1]);
				if(!amx::valid(target_amx)) amx_LogicError(errors::pointer_invalid, "AMX", params[1]);
				result = pawn_call(amx, params[0] - sizeof(cell), params + 2, native, try_, msg, target_amx);
				break;
		}
		return result;
	}

	// native amx_call_native(Amx:amx, const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE(amx_call_native, 3)
	{
		return amx_call(amx, params, true, false, nullptr);
	}

	// native amx_call_public(Amx:amx, const function[], const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(amx_call_public, 3, cell)
	{
		return amx_call(amx, params, false, false, nullptr);
	}

	// native amx_err:amx_try_call_native(Amx:amx, const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(amx_try_call_native, 4, cell)
	{
		return amx_call(amx, params, true, true, nullptr);
	}

	// native amx_err:amx_try_call_native_msg(Amx:amx, const function[], &result, msg[], msg_size=sizeof(msg), const format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(amx_try_call_native_msg, 5, cell)
	{
		std::string msg;
		cell *msg_addr = amx_GetAddrSafe(amx, params[4]);
		cell result = amx_call(amx, params, true, true, &msg);
		if(msg.size() == 0)
		{
			amx_SetString(msg_addr, amx::StrError(result), false, false, params[5]);
		}else{
			amx_SetString(msg_addr, msg.c_str(), false, false, params[5]);
		}
		return result;
	}

	// native amx_err:amx_try_call_native_msg_s(Amx:amx, const function[], &result, &StringTag:msg, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(amx_try_call_native_msg_s, 4, cell)
	{
		std::string msg;
		cell *msg_addr = amx_GetAddrSafe(amx, params[4]);
		cell result = amx_call(amx, params, true, true, &msg);
		if(msg.size() == 0)
		{
			*msg_addr = strings::create(amx::StrError(result));
		}else{
			*msg_addr = strings::create(msg);
		}
		return result;
	}

	// native amx_err:amx_try_call_public(Amx:amx, const function[], &result, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(amx_try_call_public, 4, cell)
	{
		return amx_call(amx, params, false, true, nullptr);
	}

	// native amx_name_length();
	AMX_DEFINE_NATIVE_TAG(amx_name_length, 0, cell)
	{
		int len;
		int err = amx_NameLength(amx, &len);
		if(err != AMX_ERR_NONE)
		{
			amx_RaiseError(amx, err);
		}
		return len;
	}

	// native amx_num_publics();
	AMX_DEFINE_NATIVE_TAG(amx_num_publics, 0, cell)
	{
		int num;
		int err = amx_NumPublics(amx, &num);
		if(err != AMX_ERR_NONE)
		{
			amx_RaiseError(amx, err);
		}
		return num;
	}

	// native amx_public_index(const function[]);
	AMX_DEFINE_NATIVE_TAG(amx_public_index, 1, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		int index;
		if(amx_FindPublicSafe(amx, name, &index) != AMX_ERR_NONE)
		{
			return -1;
		}
		return index;
	}

	// native amx_public_name(index, name[], size=sizeof(name));
	AMX_DEFINE_NATIVE_TAG(amx_public_name, 3, cell)
	{
		int len;
		amx_NameLength(amx, &len);
		char *name = static_cast<char*>(alloca(len + 1));
		if(amx_GetPublic(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_SetString(addr, name, false, false, params[3]);
		return strlen(name);
	}

	// native String:amx_public_name_s(index);
	AMX_DEFINE_NATIVE_TAG(amx_public_name_s, 1, string)
	{
		char *name = amx_NameBuffer(amx);
		if(amx_GetPublic(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		return strings::create(name);
	}

	// native [2]amx_encode_public(index);
	AMX_DEFINE_NATIVE_TAG(amx_encode_public, 1, cell)
	{
		cell index = params[1];
		if(index == -1) amx_LogicError(errors::out_of_range, "index");
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_encode_value(addr, 0x1B, index);
		return 1;
	}

	// native [2]amx_encode_public_name(const function[]);
	AMX_DEFINE_NATIVE_TAG(amx_encode_public_name, 1, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		int pubindex;
		if(amx_FindPublicSafe(amx, name, &pubindex) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public");
		}
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_encode_value(addr, 0x1B, pubindex);
		return 1;
	}

	// native [3]amx_encode_value_public(index, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(amx_encode_value_public, 2, cell)
	{
		cell index = params[1];
		if(index == -1) amx_LogicError(errors::out_of_range, "index");
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		amx_encode_value(addr, 0x1B, index, params[2]);
		return 1;
	}

	// native [3]amx_encode_value_public_name(const function[], AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(amx_encode_value_public_name, 2, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		int pubindex;
		if(amx_FindPublicSafe(amx, name, &pubindex) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public");
		}
		cell *addr = amx_GetAddrSafe(amx, params[3]);
		amx_encode_value(addr, 0x1B, pubindex, params[2]);
		return 1;
	}

	// native amx_public_addr(index);
	AMX_DEFINE_NATIVE_TAG(amx_public_addr, 1, cell)
	{
		cell index = params[1];
		int num;
		if(index >= 0 && amx_NumPublics(amx, &num) == AMX_ERR_NONE && index < num)
		{
			auto hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
			auto rec = reinterpret_cast<AMX_FUNCSTUB*>(amx->base + hdr->publics + index * hdr->defsize);
			return rec->address;
		}
		amx_LogicError(errors::out_of_range, "index");
		return 0;
	}

	// native amx_public_at(code=cellmin);
	AMX_DEFINE_NATIVE_TAG(amx_public_at, 0, cell)
	{
		ucell code = optparam(1, std::numeric_limits<cell>::min());
		if(code == std::numeric_limits<cell>::min())
		{
			code = amx->cip - 2 * sizeof(cell);
		}
		auto hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
		int num;
		if(amx_NumPublics(amx, &num) == AMX_ERR_NONE)
		{
			int idx = -1;
			ucell dist = std::numeric_limits<int>::max();
			for(int i = -1; i < num; i++)
			{
				ucell addr;
				if(i == -1)
				{
					if(hdr->cip >= 0)
					{
						addr = hdr->cip;
					}else{
						addr = -1;
					}
				}else{
					auto rec = reinterpret_cast<AMX_FUNCSTUB*>(amx->base + hdr->publics + i * hdr->defsize);
					addr = rec->address;
				}
				if(addr <= code && code - addr <= dist)
				{
					idx = i;
					dist = code - addr;
				}
			}
			return idx;
		}
		return -1;
	}

	// native amx_num_natives();
	AMX_DEFINE_NATIVE_TAG(amx_num_natives, 0, cell)
	{
		int num;
		int err = amx_NumNatives(amx, &num);
		if(err != AMX_ERR_NONE)
		{
			amx_RaiseError(amx, err);
		}
		return num;
	}

	// native amx_native_index(const function[]);
	AMX_DEFINE_NATIVE_TAG(amx_native_index, 1, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		int index;
		if(amx_FindNative(amx, name, &index) != AMX_ERR_NONE)
		{
			return -1;
		}
		return index;
	}

	// native amx_native_name(index, name[], size=sizeof(name));
	AMX_DEFINE_NATIVE_TAG(amx_native_name, 3, cell)
	{
		char *name = amx_NameBuffer(amx);
		if(amx_GetNative(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_SetString(addr, name, false, false, params[3]);
		return strlen(name);
	}

	// native String:amx_native_name_s(index);
	AMX_DEFINE_NATIVE_TAG(amx_native_name_s, 1, string)
	{
		char *name = amx_NameBuffer(amx);
		if(amx_GetNative(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		return strings::create(name);
	}

	// native [2]amx_encode_native(index);
	AMX_DEFINE_NATIVE_TAG(amx_encode_native, 1, cell)
	{
		char *name = amx_NameBuffer(amx);
		if(amx_GetNative(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);

		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_encode_value(addr, 0x1C, reinterpret_cast<uintptr_t>(func));
		return 1;
	}

	// native [2]amx_encode_native_name(const function[]);
	AMX_DEFINE_NATIVE_TAG(amx_encode_native_name, 1, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);

		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_encode_value(addr, 0x1C, reinterpret_cast<uintptr_t>(func));
		return 1;
	}

	// native [3]amx_encode_value_native(index, AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(amx_encode_value_native, 2, cell)
	{
		char *name = amx_NameBuffer(amx);
		if(amx_GetNative(amx, params[1], name) != AMX_ERR_NONE)
		{
			amx_LogicError(errors::out_of_range, "index");
		}
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);

		cell *addr = amx_GetAddrSafe(amx, params[3]);
		amx_encode_value(addr, 0x1C, reinterpret_cast<uintptr_t>(func), params[2]);
		return 1;
	}

	// native [3]amx_encode_value_native_name(const function[], AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(amx_encode_value_native_name, 2, cell)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		if(name == nullptr) amx_FormalError(errors::arg_empty, "function");
		auto func = amx::find_native(amx, name);
		if(func == nullptr) amx_FormalError(errors::func_not_found, "native", name);

		cell *addr = amx_GetAddrSafe(amx, params[3]);
		amx_encode_value(addr, 0x1C, reinterpret_cast<uintptr_t>(func), params[2]);
		return 1;
	}

	// native amx_encoded_length();
	AMX_DEFINE_NATIVE_TAG(amx_encoded_length, 0, cell)
	{
		return 6;
	}

	// native bool:amx_try_decode_value(const encoded[], &AnyTag:value);
	AMX_DEFINE_NATIVE_TAG(amx_try_decode_value, 2, bool)
	{
		const char *name;
		amx_StrParam(amx, params[1], name);
		auto addr = reinterpret_cast<ucell*>(amx_GetAddrSafe(amx, params[2]));
		if(name && (*name == 0x1B || *name == 0x1C))
		{
			auto str = reinterpret_cast<const unsigned char*>(name + 1);
			return amx_decode_value(str, *addr) && amx_decode_value(str, *addr);
		}
		return 0;
	}

	// native Var:amx_var(&AnyTag:var);
	AMX_DEFINE_NATIVE_TAG(amx_var, 1, var)
	{
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, params[1], 1));
	}

	// native Var:amx_var_arr(AnyTag:arr[], size=sizeof(arr));
	AMX_DEFINE_NATIVE_TAG(amx_var_arr, 2, var)
	{
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, params[1], params[2]));
	}

	// native Var:amx_public_var(const name[]);
	AMX_DEFINE_NATIVE_TAG(amx_public_var, 1, var)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty, "name");
		}

		cell amx_addr;
		if(amx_FindPubVar(amx, name, &amx_addr) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::var_not_found, "public", name);
		}
		return amx_var_pool.get_id(amx_var_pool.emplace(amx, amx_addr, 1));
	}

	// native amx_set(Var:var, AnyTag:value, index=0);
	AMX_DEFINE_NATIVE_TAG(amx_set, 2, cell)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->set(optparam(3, 0), params[2]);
	}

	// native amx_get(Var:var, index=0);
	AMX_DEFINE_NATIVE_TAG(amx_get, 1, cell)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->get(optparam(2, 0));
	}

	// native bool:amx_valid(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_valid, 1, bool)
	{
		amx_var_info *info;
		return amx_var_pool.get_by_id(params[1], info);
	}

	// native amx_delete(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_delete, 1, cell)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return amx_var_pool.remove(info);
	}

	// native bool:amx_linked(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_linked, 1, bool)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->valid();
	}

	// native bool:amx_inside(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_inside, 1, bool)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->inside();
	}

	// native amx_sizeof(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_sizeof, 1, cell)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->size();
	}

	// native bool:amx_my(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_my, 1, bool)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		
		return info->from_amx(amx);
	}

	// native amx_to_ref(Var:var, ref[1][]);
	AMX_DEFINE_NATIVE_TAG(amx_to_ref, 2, cell)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		if(!info->from_amx(amx)) amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
		
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		*addr = info->address() - params[2];
		return 1;
	}

	// native bool:amx_fork(fork_level:level=fork_machine, &result=0, bool:use_data=true, &amx_err:error=amx_err:0);
	AMX_DEFINE_NATIVE_TAG(amx_fork, 0, bool)
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
	AMX_DEFINE_NATIVE_TAG(amx_commit, 0, cell)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkCommit | (SleepReturnValueMask & optparam(1, 1));
	}

	// native amx_fork_end();
	AMX_DEFINE_NATIVE_TAG(amx_fork_end, 0, cell)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnForkEnd;
	}

	// native amx_error(amx_err:code, result=0);
	AMX_DEFINE_NATIVE_TAG(amx_error, 1, cell)
	{
		amx_RaiseError(amx, params[1]);
		return optparam(2, 0);
	}

	// native Var:amx_alloc(size, bool:zero=true);
	AMX_DEFINE_NATIVE_TAG(amx_alloc, 1, var)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return (optparam(2, 1) ? SleepReturnAllocVarZero : SleepReturnAllocVar) | (SleepReturnValueMask & params[1]);
	}

	// native bool:amx_free(Var:var);
	AMX_DEFINE_NATIVE_TAG(amx_free, 1, bool)
	{
		amx_var_info *info;
		if(!amx_var_pool.get_by_id(params[1], info)) amx_LogicError(errors::pointer_invalid, "AMX variable", params[1]);
		if(!info->from_amx(amx)) amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
			
		cell addr = info->free();
		if(addr != -1)
		{
			amx_RaiseError(amx, AMX_ERR_SLEEP);
			return SleepReturnFreeVar | (SleepReturnValueMask & ((addr - amx->hlw) / sizeof(cell)));
		}

		amx_LogicError(errors::operation_not_supported, "AMX variable", params[1]);
		return 0;
	}

	// native amx_parallel_begin(count=1);
	AMX_DEFINE_NATIVE_TAG(amx_parallel_begin, 0, cell)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnParallel | (SleepReturnValueMask & optparam(1, 1));
	}

	// native amx_parallel_end();
	AMX_DEFINE_NATIVE_TAG(amx_parallel_end, 0, cell)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnParallelEnd;
	}

	// native amx_tailcall();
	AMX_DEFINE_NATIVE_TAG(amx_tailcall, 0, cell)
	{
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnTailCall;
	}

	// native AmxGuard:amx_guard(AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(amx_guard, 2, amx_guard)
	{
		auto obj = dyn_object(amx, params[1], params[2]);
		return amx_guards::get_id(amx, amx_guards::add(amx, std::move(obj)));
	}

	// native AmxGuard:amx_guard_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(amx_guard_arr, 3, amx_guard)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		auto obj = dyn_object(amx, addr, params[2], params[3]);
		return amx_guards::get_id(amx, amx_guards::add(amx, std::move(obj)));
	}

	// native bool:amx_guard_valid(AmxGuard:guard);
	AMX_DEFINE_NATIVE_TAG(amx_guard_valid, 1, bool)
	{
		handle_t *obj;
		return amx_guards::get_by_id(amx, params[1], obj);
	}

	// native amx_guard_free(AmxGuard:guard);
	AMX_DEFINE_NATIVE_TAG(amx_guard_free, 1, cell)
	{
		handle_t *obj;
		if(!amx_guards::get_by_id(amx, params[1], obj)) amx_LogicError(errors::pointer_invalid, "guard", params[1]);

		return amx_guards::free(amx, obj);
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
	AMX_DECLARE_NATIVE(amx_try_call_native_msg),
	AMX_DECLARE_NATIVE(amx_try_call_native_msg_s),
	AMX_DECLARE_NATIVE(amx_try_call_public),
	AMX_DECLARE_NATIVE(amx_name_length),
	AMX_DECLARE_NATIVE(amx_num_publics),
	AMX_DECLARE_NATIVE(amx_public_index),
	AMX_DECLARE_NATIVE(amx_public_name),
	AMX_DECLARE_NATIVE(amx_public_name_s),
	AMX_DECLARE_NATIVE(amx_encode_public),
	AMX_DECLARE_NATIVE(amx_encode_public_name),
	AMX_DECLARE_NATIVE(amx_encode_value_public),
	AMX_DECLARE_NATIVE(amx_encode_value_public_name),
	AMX_DECLARE_NATIVE(amx_public_addr),
	AMX_DECLARE_NATIVE(amx_public_at),
	AMX_DECLARE_NATIVE(amx_num_natives),
	AMX_DECLARE_NATIVE(amx_native_index),
	AMX_DECLARE_NATIVE(amx_native_name),
	AMX_DECLARE_NATIVE(amx_native_name_s),
	AMX_DECLARE_NATIVE(amx_encode_native),
	AMX_DECLARE_NATIVE(amx_encode_native_name),
	AMX_DECLARE_NATIVE(amx_encode_value_native),
	AMX_DECLARE_NATIVE(amx_encode_value_native_name),
	AMX_DECLARE_NATIVE(amx_encoded_length),
	AMX_DECLARE_NATIVE(amx_try_decode_value),
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
	AMX_DECLARE_NATIVE(amx_guard),
	AMX_DECLARE_NATIVE(amx_guard_arr),
	AMX_DECLARE_NATIVE(amx_guard_valid),
	AMX_DECLARE_NATIVE(amx_guard_free),
};

int RegisterAmxNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
