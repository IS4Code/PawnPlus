#include "natives.h"
#include "main.h"
#include "errors.h"
#include "context.h"
#include "modules/debug.h"
#include "modules/strings.h"

cell get_level(AMX *amx, cell level)
{
	if(level == 0)
	{
		return amx->cip;
	}

	auto hdr = (AMX_HEADER *)amx->base;
	auto data = amx->data ? amx->data : amx->base + (int)hdr->dat;
	cell frm = amx->frm;
	for(cell i = 1; i < level; i++)
	{
		frm = *reinterpret_cast<cell*>(data + frm);
		if(frm == 0)
		{
			return 0;
		}
	}
	cell cip = reinterpret_cast<cell*>(data + frm)[1];
	if(cip == 0)
	{
		return 0;
	}
	return cip - sizeof(cell);
}

namespace Natives
{
	// native debug_get_line(level=0);
	AMX_DEFINE_NATIVE(debug_get_line, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell cip = get_level(amx, optparam(1, 0));

		long line;
		if(dbg_LookupLine(dbg, cip, &line) == AMX_ERR_NONE)
		{
			return line;
		}
		return -1;
	}

	// native String:debug_get_file_s(level=0);
	AMX_DEFINE_NATIVE(debug_get_file_s, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell cip = get_level(amx, optparam(1, 0));

		const char *filename;
		if(dbg_LookupFile(dbg, cip, &filename) == AMX_ERR_NONE)
		{
			return strings::create(filename);
		}
		return 0;
	}

	// native String:debug_get_function_s(level=0);
	AMX_DEFINE_NATIVE(debug_get_function_s, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell cip = get_level(amx, optparam(1, 0));

		const char *function;
		if(dbg_LookupFunction(dbg, cip, &function) == AMX_ERR_NONE)
		{
			return strings::create(function);
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(debug_get_line),
	AMX_DECLARE_NATIVE(debug_get_file_s),
	AMX_DECLARE_NATIVE(debug_get_function_s),
};

int RegisterDebugNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
