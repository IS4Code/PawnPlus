#include "natives.h"
#include "main.h"
#include "errors.h"
#include "context.h"
#include "modules/debug.h"
#include "modules/strings.h"
#include "modules/tags.h"

#include <limits>
#include <cstring>

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
	// native bool:debug_loaded();
	AMX_DEFINE_NATIVE(debug_loaded, 0)
	{
		auto obj = amx::load_lock(amx);
		return obj->has_extra<debug::info>();
	}

	// native AmxDebug:debug_get_ptr();
	AMX_DEFINE_NATIVE(debug_get_ptr, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			return 0;
		}
		return reinterpret_cast<cell>(obj->get_extra<debug::info>().dbg);
	}

	// native debug_code(level=0);
	AMX_DEFINE_NATIVE(debug_code, 0)
	{
		cell level = optparam(1, 0);
		if(level < 0)
		{
			amx_LogicError(errors::out_of_range, "level");
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
			amx_LogicError(errors::out_of_range, "code");
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
			amx_LogicError(errors::out_of_range, "code");
			return 0;
		}
		const char *filename;
		if(dbg_LookupFile(dbg, cip, &filename) == AMX_ERR_NONE)
		{
			return strings::create(filename);
		}
		return 0;
	}

	// native Symbol:debug_symbol(const name[], code=cellmin, symbol_kind:kind=symbol_kind:-1, symbol_class:class=symbol_class:-1);
	AMX_DEFINE_NATIVE(debug_symbol, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		const char *name;
		amx_StrParam(amx, params[1], name);
		
		ucell cip = optparam(2, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		}else if(static_cast<cell>(cip) < 0)
		{
			amx_LogicError(errors::out_of_range, "code");
			return 0;
		}

		cell kind = optparam(3, -1);
		cell cls = optparam(4, -1);

		cell minindex = std::numeric_limits<cell>::min();
		ucell mindist;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto sym = dbg->symboltbl[i];
			if(sym->codestart <= cip && sym->codeend > cip && ((1 << (sym->ident - 1)) & kind) && ((1 << (sym->vclass - 1)) & cls) && !std::strcmp(sym->name, name))
			{
				if(minindex == std::numeric_limits<cell>::min() || sym->codeend - sym->codestart < mindist)
				{
					minindex = i;
					mindist = sym->codeend - sym->codestart;
				}
			}
		}
		return minindex;
	}

	// native Symbol:debug_func(code=cellmin);
	AMX_DEFINE_NATIVE(debug_func, 0)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;
		
		ucell cip = optparam(1, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		}else if(static_cast<cell>(cip) < 0)
		{
			amx_LogicError(errors::out_of_range, "code");
			return 0;
		}

		cell minindex = std::numeric_limits<cell>::min();
		ucell mindist;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto sym = dbg->symboltbl[i];
			if(sym->ident == iFUNCTN && sym->codestart <= cip && sym->codeend > cip)
			{
				if(minindex == std::numeric_limits<cell>::min() || sym->codeend - sym->codestart < mindist)
				{
					minindex = i;
					mindist = sym->codeend - sym->codestart;
				}
			}
		}
		return minindex;
	}

	// native Symbol:debug_var(AnyTag:&var);
	AMX_DEFINE_NATIVE(debug_var, 0)
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
			amx_LogicError(errors::out_of_range, "var");
			return 0;
		}else if(addr >= amx->hlw && addr < amx->hea)
		{
			return 0;
		}

		if(addr < amx->hlw) // data variable
		{
			for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
			{
				auto sym = dbg->symboltbl[i];
				if(sym->vclass == 0 || sym->vclass == 2) // global/static
				{
					if(sym->ident == iVARIABLE || sym->ident == iARRAY)
					{
						if(sym->address == addr)
						{
							return i;
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
						if(sym->ident == iVARIABLE || sym->ident == iARRAY)
						{
							if(sym->address == addr && sym->codestart <= cip && cip < sym->codeend)
							{
								return i;
							}
						}
					}
				}
			}
		}
		return std::numeric_limits<cell>::min();
	}
	
	// native symbol_kind:debug_symbol_kind(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_kind, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return 1 << (sym->ident - 1);
	}
	
	// native symbol_class:debug_symbol_class(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_class, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return 1 << (sym->vclass - 1);
	}

	// native debug_symbol_tag(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_tag, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		cell tag = sym->tag;

		if(tag == 0)
		{
			return 0x80000000;
		}

		const char *tagname;
		if(dbg_GetTagName(dbg, tag, &tagname) == AMX_ERR_NONE)
		{
			if(*tagname >= 'A' && *tagname <= 'Z')
			{
				tag |= 0x40000000;
			}

			if(tags::find_tag(amx, tag)->uid != tags::tag_unknown)
			{
				tag |= 0x80000000;
			}
		}

		return tag;
	}

	// native Symbol:debug_symbol_func(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_func, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto insym = dbg->symboltbl[index];

		cell minindex = std::numeric_limits<cell>::min();
		ucell mindist;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto sym = dbg->symboltbl[i];
			if(sym->ident == iFUNCTN && sym->codestart <= insym->codestart && sym->codeend >= insym->codeend)
			{
				if(minindex == std::numeric_limits<cell>::min() || sym->codeend - sym->codestart < mindist)
				{
					minindex = i;
					mindist = sym->codeend - sym->codestart;
				}
			}
		}
		return minindex;
	}

	// native String:debug_symbol_name_s(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_name_s, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return strings::create(sym->name);
	}

	// native debug_symbol_range(Symbol:symbol, &codestart, &codeend);
	AMX_DEFINE_NATIVE(debug_symbol_range, 3)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		cell *start, *end;
		amx_GetAddr(amx, params[2], &start);
		amx_GetAddr(amx, params[3], &end);
		if(start)
		{
			*start = sym->codestart;
		}
		if(end)
		{
			*end = sym->codeend;
		}
		return 1;
	}

	// native debug_symbol_line(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_line, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		long line;
		if(dbg_LookupLine(dbg, sym->codestart, &line) == AMX_ERR_NONE)
		{
			return line;
		}
		return -1;
	}

	// native String:debug_symbol_file_s(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_file_s, 1)
	{
		auto obj = amx::load_lock(amx);
		if(!obj->has_extra<debug::info>())
		{
			amx_LogicError(errors::no_debug_error);
			return 0;
		}
		auto dbg = obj->get_extra<debug::info>().dbg;

		cell index = params[1];
		if(index < 0 || index > dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		const char *filename;
		if(dbg_LookupFile(dbg, sym->codestart, &filename) == AMX_ERR_NONE)
		{
			return strings::create(filename);
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(debug_loaded),
	AMX_DECLARE_NATIVE(debug_get_ptr),
	AMX_DECLARE_NATIVE(debug_code),
	AMX_DECLARE_NATIVE(debug_line),
	AMX_DECLARE_NATIVE(debug_file_s),
	AMX_DECLARE_NATIVE(debug_symbol),
	AMX_DECLARE_NATIVE(debug_func),
	AMX_DECLARE_NATIVE(debug_var),
	AMX_DECLARE_NATIVE(debug_symbol_kind),
	AMX_DECLARE_NATIVE(debug_symbol_class),
	AMX_DECLARE_NATIVE(debug_symbol_tag),
	AMX_DECLARE_NATIVE(debug_symbol_func),
	AMX_DECLARE_NATIVE(debug_symbol_name_s),
	AMX_DECLARE_NATIVE(debug_symbol_range),
	AMX_DECLARE_NATIVE(debug_symbol_line),
	AMX_DECLARE_NATIVE(debug_symbol_file_s),
};

int RegisterDebugNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
