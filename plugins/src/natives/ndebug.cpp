#include "natives.h"
#include "main.h"
#include "errors.h"
#include "context.h"
#include "modules/debug.h"
#include "modules/strings.h"

#include <limits>

cell get_level(AMX *amx, cell level)
{
	if(level == 0)
	{
		return amx->cip - 2 * sizeof(cell);
	}

	auto hdr = (AMX_HEADER *)amx->base;
	auto data = amx->data ? amx->data : amx->base + (int)hdr->dat;
	cell frm = amx->frm;
	for(cell i = 1; i < level; i++)
	{
		frm = *reinterpret_cast<cell*>(data + frm);
		if(frm == 0)
		{
			return -1;
		}
	}
	cell cip = reinterpret_cast<cell*>(data + frm)[1];
	if(cip == 0)
	{
		return -1;
	}
	return cip - 2 * sizeof(cell);
}

namespace Natives
{
	// native debug_code(level=0);
	AMX_DEFINE_NATIVE(debug_code, 0)
	{
		cell level = optparam(1, 0);
		if(level < 0)
		{
			amx_LogicError(errors::out_of_range);
			return 0;
		}
		return get_level(amx, level);
	}

	// native debug_line(code=cellmin);
	AMX_DEFINE_NATIVE(debug_line, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;
		
		cell cip = optparam(1, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		}else if(cip < 0)
		{
			amx_LogicError(errors::out_of_range);
			return 0;
		}
		long line;
		if(dbg_LookupLine(dbg, cip, &line) == AMX_ERR_NONE)
		{
			return line;
		}
		return -1;
	}

	// native String:debug_file_s(code=cellmin);
	AMX_DEFINE_NATIVE(debug_file_s, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell cip = optparam(1, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		}else if(cip < 0)
		{
			amx_LogicError(errors::out_of_range);
			return 0;
		}
		const char *filename;
		if(dbg_LookupFile(dbg, cip, &filename) == AMX_ERR_NONE)
		{
			return strings::create(filename);
		}
		return 0;
	}

	// native String:debug_funcname_s(code=cellmin);
	AMX_DEFINE_NATIVE(debug_funcname_s, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;
		
		cell cip = optparam(1, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		}else if(cip < 0)
		{
			amx_LogicError(errors::out_of_range);
			return 0;
		}
		const char *function;
		if(dbg_LookupFunction(dbg, cip, &function) == AMX_ERR_NONE)
		{
			return strings::create(function);
		}
		return 0;
	}

	// native String:debug_varname_s(AnyTag:&var);
	AMX_DEFINE_NATIVE(debug_varname_s, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;


		cell addr = params[1];
		if(addr < 0 || addr >= amx->stp || (addr >= amx->hea && addr < amx->stk))
		{
			amx_LogicError(errors::out_of_range);
			return 0;
		}else if(addr >= amx->hlw && addr < amx->hea)
		{
			return 0;
		}

		AMX_DBG_SYMBOL *found = nullptr;
		if(addr < amx->hlw) // data variable
		{
			for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
			{
				auto sym = dbg->symboltbl[i];
				if(sym->vclass == 0 || sym->vclass == 2) // global/static
				{
					if(sym->ident == 1 || sym->ident == 3) // variable/array
					{
						if(sym->address == addr)
						{
							found = sym;
							break;
						}
					}
				}
			}
		}else{ // stack variable
			auto hdr = (AMX_HEADER *)amx->base;
			auto data = amx->data ? amx->data : amx->base + (int)hdr->dat;
			ucell frm = amx->frm, cip = amx->cip - 2 * sizeof(cell);
			while(frm != 0)
			{
				auto frame = reinterpret_cast<cell*>(data + frm);
				auto args = frame[2];
				if(static_cast<ucell>(addr) < frm + 3 * sizeof(cell) + args)
				{
					break;
				}
				frm = frame[0];
				cip = frame[1] - 2 * sizeof(cell);
			}

			if(frm != 0)
			{
				addr -= frm;

				for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
				{
					auto sym = dbg->symboltbl[i];
					if(sym->vclass == 1) // local
					{
						if(sym->ident == 1 || sym->ident == 3) // variable/array
						{
							if(sym->address == addr && sym->codestart <= cip && cip <= sym->codeend)
							{
								found = sym;
								break;
							}
						}
					}
				}
			}
		}
		if(found != nullptr)
		{
			return strings::create(found->name);
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(debug_code),
	AMX_DECLARE_NATIVE(debug_line),
	AMX_DECLARE_NATIVE(debug_file_s),
	AMX_DECLARE_NATIVE(debug_funcname_s),
	AMX_DECLARE_NATIVE(debug_varname_s),
};

int RegisterDebugNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
