#include "natives.h"
#include "modules/strings.h"
#include "modules/tags.h"

namespace Natives
{
	// native tag_uid:tag_uid(tag_id);
	AMX_DEFINE_NATIVE_TAG(tag_uid, 1, cell)
	{
		return tags::find_tag(amx, params[1])->uid;
	}

	// native tag_id(tag_uid:tag_uid);
	AMX_DEFINE_NATIVE_TAG(tag_id, 1, cell)
	{
		return tags::find_tag(params[1])->get_id(amx);
	}

	// native tag_name(tag_uid:tag_uid, name[], size=sizeof(name));
	AMX_DEFINE_NATIVE_TAG(tag_name, 3, cell)
	{
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		tag_ptr tag = tags::find_tag(params[1]);
		amx_SetString(addr, tag->name.c_str(), 0, 0, params[3]);
		return tag->name.size();
	}

	// native String:tag_name_s(tag_uid:tag_uid);
	AMX_DEFINE_NATIVE_TAG(tag_name_s, 1, string)
	{
		tag_ptr tag = tags::find_tag(params[1]);
		return strings::create(tag->name);
	}

	// native tag_uid:tag_base(tag_uid:tag_uid);
	AMX_DEFINE_NATIVE_TAG(tag_base, 1, cell)
	{
		auto base = tags::find_tag(params[1])->base;
		if(base != nullptr) return base->uid;
		return 0;
	}

	// native bool:tag_derived_from(tag_uid:tag_uid, tag_uid:base_uid);
	AMX_DEFINE_NATIVE_TAG(tag_derived_from, 2, bool)
	{
		return tags::find_tag(params[1])->inherits_from(tags::find_tag(params[2]));
	}

	// native tag_uid:tag_find(const name[]);
	AMX_DEFINE_NATIVE_TAG(tag_find, 1, cell)
	{
		char *name;
		amx_StrParam(amx, params[1], name);
		return tags::find_existing_tag(name)->uid;
	}

	// native tag_uid:tag_new(const name[], tag_uid:base=tag_uid_unknown);
	AMX_DEFINE_NATIVE_TAG(tag_new, 1, cell)
	{
		char *name;
		amx_StrParam(amx, params[1], name);

		return tags::new_tag(name, optparam(2, 0))->uid;
	}

	// native bool:tag_set_op(tag_uid:tag_uid, tag_op:tag_op, const handler[], const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(tag_set_op, 3, bool)
	{
		tag_control *ctl = tags::find_tag(params[1])->get_control();
		if(ctl == nullptr) return 0;

		char *handler;
		amx_StrParam(amx, params[3], handler);

		const char *add_format;
		amx_OptStrParam(amx, 4, add_format, "");

		cell *args = params + 5;
		int numargs = (params[0] / sizeof(cell)) - 4;

		return ctl->set_op(static_cast<op_type>(params[2]), amx, handler, add_format, args, numargs);
	}

	// native bool:tag_lock(tag_uid:tag_uid);
	AMX_DEFINE_NATIVE_TAG(tag_lock, 1, bool)
	{
		tag_control *ctl = tags::find_tag(params[1])->get_control();
		if(ctl == nullptr) return 0;
		return ctl->lock();
	}

	// native tag_call_op(tag_uid:tag_uid, tag_op:tag_op, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(tag_call_op, 2, cell)
	{
		auto tag = tags::find_tag(params[1]);
		size_t numargs = (params[0] / sizeof(cell)) - 2;
		cell *args = new cell[numargs];
		for(size_t i = 0; i < numargs; i++)
		{
			cell *addr = amx_GetAddrSafe(amx, params[3 + i]);
			args[i] = *addr;
		}
		cell result = tag->call_op(tag, static_cast<op_type>(params[2]), args, numargs);
		delete[] args;
		return result;
	}

	// native tag_uid:tag_element(tag_uid:tag_uid);
	AMX_DEFINE_NATIVE_TAG(tag_element, 1, cell)
	{
		auto element = tags::find_tag(params[1])->get_ops().get_element();
		if(element != nullptr) return element->uid;
		return 0;
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
	AMX_DECLARE_NATIVE(tag_element),
};

int RegisterTagNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
