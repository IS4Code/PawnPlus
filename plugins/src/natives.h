#ifndef NATIVES_H_INCLUDED
#define NATIVES_H_INCLUDED

#include "main.h"
#include "errors.h"
#include "amxinfo.h"
#include "modules/tags.h"
#include "sdk/amx/amx.h"

#define optparam(idx, optvalue) (params[0] / sizeof(cell) < (idx) ? (optvalue) : params[idx])
#define optparamref(idx, optvalue) (params[0] / sizeof(cell) < (idx) ? &(*reinterpret_cast<cell*>(alloca(sizeof(cell))) = (optvalue)) : amx_GetAddrSafe(amx, params[idx]))
#define amx_OptStrParam(amx,idx,result,default)                             \
    do {                                                                    \
      if (params[0] / sizeof(cell) < idx) { result = default; break; }      \
      cell *amx_cstr_; int amx_length_;                                     \
      amx_cstr_ = amx_GetAddrSafe((amx), (params[idx]));                    \
      amx_StrLen(amx_cstr_, &amx_length_);                                  \
      if (amx_length_ > 0 &&                                                \
          ((result) = (char*)alloca((amx_length_ + 1) * sizeof(*(result)))) != NULL) \
        amx_GetString((char*)(result), amx_cstr_, sizeof(*(result))>1, amx_length_ + 1); \
      else (result) = NULL;                                                 \
    } while (0)

#ifdef amx_StrParam
#undef amx_StrParam
#define amx_StrParam(amx,param,result)                                      \
    do {                                                                    \
      cell *amx_cstr_; int amx_length_;                                     \
      amx_cstr_ = amx_GetAddrSafe((amx), (param));                          \
      amx_StrLen(amx_cstr_, &amx_length_);                                  \
      if (amx_length_ > 0 &&                                                \
          ((result) = (char*)alloca((amx_length_ + 1) * sizeof(*(result)))) != NULL) \
        amx_GetString((char*)(result), amx_cstr_, sizeof(*(result))>1, amx_length_ + 1); \
      else (result) = NULL;                                                 \
    } while (0)
#endif

extern tag_ptr native_return_tag;

namespace impl
{
	cell handle_error(AMX *amx, const cell *params, const char *native, const errors::native_error &error);

	template <AMX_NATIVE Native>
	struct native_info;

	template <AMX_NATIVE Native>
	static cell AMX_NATIVE_CALL adapt_native(AMX *amx, cell *params) noexcept
	{
		try{
			if(params[0] < native_info<Native>::arg_count() * static_cast<cell>(sizeof(cell)))
			{
				amx_FormalError(errors::not_enough_args, native_info<Native>::arg_count(), params[0] / static_cast<cell>(sizeof(cell)));
			}
			native_return_tag = nullptr;
			return Native(amx, params);
		}catch(const errors::end_of_arguments_error &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), errors::native_error(errors::not_enough_args, 3, err.argbase - params - 1 + err.required, params[0] / static_cast<cell>(sizeof(cell))));
		}catch(const errors::native_error &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), err);
		}catch(const errors::amx_error &err)
		{
			amx_RaiseError(amx, err.code);
			return 0;
		}
#ifndef _DEBUG
		catch(const std::exception &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), errors::native_error(errors::unhandled_exception, 2, err.what()));
		}
#endif
	}
}

struct native_error_level : public amx::extra
{
	int level = 2;

	native_error_level(AMX *amx) : amx::extra(amx)
	{

	}
};

#define AMX_DEFINE_NATIVE(Name, ArgCount) \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params); \
} \
namespace impl \
{ \
	template <> \
	struct native_info<&Natives::Name> \
	{ \
		static constexpr char *name() { return #Name; } \
		static constexpr cell arg_count() { return ArgCount; } \
	}; \
} \
namespace Natives \
{ \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params)

#define AMX_DECLARE_NATIVE(Name) {#Name, impl::adapt_native<&Natives::Name>}

int RegisterConfigNatives(AMX *amx);
int RegisterPawnNatives(AMX *amx);
int RegisterStringsNatives(AMX *amx);
int RegisterTasksNatives(AMX *amx);
int RegisterThreadNatives(AMX *amx);
int RegisterVariantNatives(AMX *amx);
int RegisterListNatives(AMX *amx);
int RegisterMapNatives(AMX *amx);
int RegisterIterNatives(AMX *amx);
int RegisterTagNatives(AMX *amx);
int RegisterAmxNatives(AMX *amx);
int RegisterLinkedListNatives(AMX *amx);
int RegisterHandleNatives(AMX *amx);
int RegisterMathNatives(AMX *amx);
int RegisterDebugNatives(AMX *amx);
int RegisterPoolNatives(AMX *amx);
int RegisterExprNatives(AMX *amx);

inline int RegisterNatives(AMX *amx)
{
	RegisterConfigNatives(amx);
	RegisterPawnNatives(amx);
	RegisterStringsNatives(amx);
	RegisterTasksNatives(amx);
	RegisterThreadNatives(amx);
	RegisterVariantNatives(amx);
	RegisterListNatives(amx);
	RegisterMapNatives(amx);
	RegisterIterNatives(amx);
	RegisterTagNatives(amx);
	RegisterAmxNatives(amx);
	RegisterLinkedListNatives(amx);
	RegisterHandleNatives(amx);
	RegisterMathNatives(amx);
	RegisterDebugNatives(amx);
	RegisterPoolNatives(amx);
	RegisterExprNatives(amx);
	return AMX_ERR_NONE;
}

#endif
