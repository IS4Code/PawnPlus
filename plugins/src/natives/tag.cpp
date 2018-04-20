#include "natives.h"
#include "modules/strings.h"
#include "modules/tags.h"

namespace Natives
{

	// native tag_uid(tag_id);
	static cell AMX_NATIVE_CALL tag_uid(AMX *amx, cell *params)
	{
		return tags::find_tag(amx, params[1])->uid;
	}

	// native tag_id(tag_uid);
	static cell AMX_NATIVE_CALL tag_id(AMX *amx, cell *params)
	{
		return tags::find_tag(params[1])->get_id(amx);
	}

	// native tag_name(tag_uid, name[], size=sizeof(name));
	static cell AMX_NATIVE_CALL tag_name(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		tag_ptr tag = tags::find_tag(params[1]);
		amx_SetString(addr, tag->name.c_str(), 0, 0, params[3]);
		return tag->name.size();
	}

	// native String:tag_name_s(tag_uid);
	static cell AMX_NATIVE_CALL tag_name_s(AMX *amx, cell *params)
	{
		tag_ptr tag = tags::find_tag(params[1]);
		return reinterpret_cast<cell>(strings::create(tag->name, true));
	}

	// native bool:tag_derived_from(tag_uid, baseuid);
	static cell AMX_NATIVE_CALL tag_derived_from(AMX *amx, cell *params)
	{
		return tags::find_tag(params[1])->inherits_from(tags::find_tag(params[2]));
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(tag_uid),
	AMX_DECLARE_NATIVE(tag_id),
	AMX_DECLARE_NATIVE(tag_name),
	AMX_DECLARE_NATIVE(tag_name_s),
	AMX_DECLARE_NATIVE(tag_derived_from),
};

int RegisterTagNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
