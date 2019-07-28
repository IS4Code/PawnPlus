#ifndef NATIVES_H_INCLUDED
#define NATIVES_H_INCLUDED

#include "main.h"
#include "errors.h"
#include "amxinfo.h"
#include "modules/tags.h"
#include "sdk/amx/amx.h"
#include <unordered_map>

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
	struct runtime_native_info
	{
		const char *name;
		cell arg_count;
		cell tag_uid;
		AMX_NATIVE inner;

		runtime_native_info(const char *name, cell arg_count, cell tag_uid, AMX_NATIVE inner) : name(name), arg_count(arg_count), tag_uid(tag_uid), inner(inner)
		{

		}
	};

	std::unordered_map<AMX_NATIVE, runtime_native_info> &runtime_native_map();

	cell handle_error(AMX *amx, const cell *params, const char *native, size_t native_size, const errors::native_error &error);

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
			if(native_info<Native>::tag_uid() == tags::tag_unknown)
			{
				return Native(amx, params);
			}else{
				cell result = Native(amx, params);
				native_return_tag = tags::find_tag(native_info<Native>::tag_uid());
				return result;
			}
		}catch(const errors::end_of_arguments_error &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), native_info<Native>::name_size(), errors::native_error(errors::not_enough_args, 3, err.argbase - params - 1 + err.required, params[0] / static_cast<cell>(sizeof(cell))));
		}catch(const errors::native_error &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), native_info<Native>::name_size(), err);
		}catch(const errors::amx_error &err)
		{
			amx_RaiseError(amx, err.code);
			return 0;
		}
#ifndef _DEBUG
		catch(const std::exception &err)
		{
			return handle_error(amx, params, native_info<Native>::name(), native_info<Native>::name_size(), errors::native_error(errors::unhandled_exception, 2, err.what()));
		}
#endif
	}

	template <AMX_NATIVE Native>
	static AMX_NATIVE init_native() noexcept
	{
		return runtime_native_map().emplace(std::piecewise_construct, std::forward_as_tuple(adapt_native<Native>), std::forward_as_tuple(native_info<Native>::name(), native_info<Native>::arg_count(), native_info<Native>::tag_uid(), Native)).first->first;
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
		static constexpr size_t name_size() { return sizeof(#Name) - 1; } \
		static constexpr cell arg_count() { return ArgCount; } \
		static constexpr cell tag_uid() { return tags::tag_unknown; } \
	}; \
} \
namespace Natives \
{ \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params)

#define AMX_DEFINE_NATIVE_TAG(Name, ArgCount, Tag) \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params); \
} \
namespace impl \
{ \
	template <> \
	struct native_info<&Natives::Name> \
	{ \
		static constexpr char *name() { return #Name; } \
		static constexpr size_t name_size() { return sizeof(#Name) - 1; } \
		static constexpr cell arg_count() { return ArgCount; } \
		static constexpr cell tag_uid() { return tags::tag_##Tag; } \
	}; \
} \
namespace Natives \
{ \
	cell AMX_NATIVE_CALL Name(AMX *amx, cell *params)

#define AMX_DECLARE_NATIVE(Name) {#Name, impl::init_native<&Natives::Name>()}

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
