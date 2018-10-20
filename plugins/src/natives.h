#ifndef NATIVES_H_INCLUDED
#define NATIVES_H_INCLUDED

#include "main.h"
#include "sdk/amx/amx.h"

#define optparam(idx, optvalue) (params[0] / sizeof(cell) < (idx) ? (optvalue) : params[idx])
#define amx_OptStrParam(amx,idx,result,default)                             \
    do {                                                                    \
      if (params[0] / sizeof(cell) < idx) { result = default; break; }      \
      cell *amx_cstr_; int amx_length_;                                     \
      amx_GetAddr((amx), (params[idx]), &amx_cstr_);                        \
      amx_StrLen(amx_cstr_, &amx_length_);                                  \
      if (amx_length_ > 0 &&                                                \
          ((result) = (char*)alloca((amx_length_ + 1) * sizeof(*(result)))) != NULL) \
        amx_GetString((char*)(result), amx_cstr_, sizeof(*(result))>1, amx_length_ + 1); \
      else (result) = NULL;                                                 \
    } while (0)

namespace impl
{
	template <AMX_NATIVE Native>
	struct native_info;

	template <AMX_NATIVE Native>
	static cell AMX_NATIVE_CALL adapt_native(AMX *amx, cell *params)
	{
		if(params[0] < native_info<Native>::arg_count * static_cast<cell>(sizeof(cell)))
		{
			logerror(amx, AMX_ERR_PARAMS, "[PP] %s: not enough arguments (%d expected, got %d)", native_info<Native>::name, native_info<Native>::arg_count, params[0] / static_cast<cell>(sizeof(cell)));
			return 0;
		}
		return Native(amx, params);
	}
}

#define AMX_DEFINE_NATIVE(Name, ArgCount) \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params); \
} \
namespace impl \
{ \
	template <> \
	struct native_info<&Natives::Name> \
	{ \
		static constexpr const char name[] = #Name; \
		static constexpr const cell arg_count = ArgCount; \
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
	return AMX_ERR_NONE;
}

#endif
