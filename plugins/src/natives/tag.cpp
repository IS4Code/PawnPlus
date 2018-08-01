#include "natives.h"
#include "modules/strings.h"
#include "modules/tags.h"

namespace Natives
{

	// native tag_uid:tag_uid(tag_id);
	static cell AMX_NATIVE_CALL tag_uid(AMX *amx, cell *params)
	{
		return tags::find_tag(amx, params[1])->uid;
	}

	// native tag_id(tag_uid:tag_uid);
	static cell AMX_NATIVE_CALL tag_id(AMX *amx, cell *params)
	{
		return tags::find_tag(params[1])->get_id(amx);
	}

	// native tag_name(tag_uid:tag_uid, name[], size=sizeof(name));
	static cell AMX_NATIVE_CALL tag_name(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		tag_ptr tag = tags::find_tag(params[1]);
		amx_SetString(addr, tag->name.c_str(), 0, 0, params[3]);
		return tag->name.size();
	}

	// native String:tag_name_s(tag_uid:tag_uid);
	static cell AMX_NATIVE_CALL tag_name_s(AMX *amx, cell *params)
	{
		tag_ptr tag = tags::find_tag(params[1]);
		return strings::pool.get_id(strings::create(tag->name, true));
	}

	// native tag_base(tag_uid:tag_uid);
	static cell AMX_NATIVE_CALL tag_base(AMX *amx, cell *params)
	{
		auto base = tags::find_tag(params[1])->base;
		if(base != nullptr) return base->uid;
		return 0;
	}

	// native bool:tag_derived_from(tag_uid:tag_uid, tag_uid:base_uid);
	static cell AMX_NATIVE_CALL tag_derived_from(AMX *amx, cell *params)
	{
		return tags::find_tag(params[1])->inherits_from(tags::find_tag(params[2]));
	}

	// native tag_uid:tag_find(const name[]);
	static cell AMX_NATIVE_CALL tag_find(AMX *amx, cell *params)
	{
		char *name;
		amx_StrParam(amx, params[1], name);
		return tags::find_existing_tag(name)->uid;
	}

	// native tag_uid:tag_new(const name[], tag_uid:base=tag_uid_unknown);
	static cell AMX_NATIVE_CALL tag_new(AMX *amx, cell *params)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		return tags::new_tag(name, params[2])->uid;
	}

	// native bool:tag_set_op(tag_uid:tag_uid, tag_op:tag_op, const handler[], const additional_format[]="", AnyTag:...);
	static cell AMX_NATIVE_CALL tag_set_op(AMX *amx, cell *params)
	{
		tag_control *ctl = tags::find_tag(params[1])->get_control();
		if(ctl == nullptr) return 0;

		char *handler;
		amx_StrParam(amx, params[3], handler);

		char *add_format;
		amx_StrParam(amx, params[4], add_format);

		cell *args = params + 5;
		int numargs = (params[0] / sizeof(cell)) - 4;

		return ctl->set_op(static_cast<op_type>(params[2]), amx, handler, add_format, args, numargs);
	}

	// native bool:tag_lock(tag_uid:tag_uid);
	static cell AMX_NATIVE_CALL tag_lock(AMX *amx, cell *params)
	{
		tag_control *ctl = tags::find_tag(params[1])->get_control();
		if(ctl == nullptr) return 0;
		return ctl->lock();
	}

	// native bool:tag_call_op(tag_uid:tag_uid, tag_op:tag_op, AnyTag:arg1, AnyTag:arg2=0);
	static cell AMX_NATIVE_CALL tag_call_op(AMX *amx, cell *params)
	{
		auto tag = tags::find_tag(params[1]);
		return tag->call_op(tag, static_cast<op_type>(params[2]), params[3], params[4]);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(tag_uid),
	AMX_DECLARE_NATIVE(tag_id),
	AMX_DECLARE_NATIVE(tag_name),
	AMX_DECLARE_NATIVE(tag_name_s),
	AMX_DECLARE_NATIVE(tag_base),
	AMX_DECLARE_NATIVE(tag_derived_from),
	AMX_DECLARE_NATIVE(tag_find),
	AMX_DECLARE_NATIVE(tag_new),
	AMX_DECLARE_NATIVE(tag_set_op),
	AMX_DECLARE_NATIVE(tag_call_op),
	AMX_DECLARE_NATIVE(tag_lock),
};

int RegisterTagNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
