#include "natives.h"
#include "main.h"
#include "errors.h"
#include "context.h"
#include "modules/debug.h"
#include "modules/strings.h"
#include "modules/tags.h"
#include "modules/containers.h"

#include <limits>
#include <cstring>

cell get_level(AMX *amx, cell level)
{
	if(level == 0)
	{
		return amx->cip - 2 * sizeof(cell);
	}

	auto data = amx_GetData(amx);
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

AMX_DBG *get_debug(AMX *amx)
{
	const auto &obj = amx::load_lock(amx);
	if(!obj->dbg)
	{
		amx_LogicError(errors::no_debug_error);
		return nullptr;
	}
	return obj->dbg.get();
}

cell *find_symbol_addr(AMX *amx, cell index, cell level, AMX_DBG *&dbg, AMX_DBG_SYMBOL *&sym)
{
	dbg = get_debug(amx);

	if(index < 0 || index >= dbg->hdr->symbols)
	{
		amx_LogicError(errors::out_of_range, "symbol");
		return 0;
	}
	sym = dbg->symboltbl[index];

	ucell cip;
	if(level < 0 || (cip = get_level(amx, level)) == -1)
	{
		amx_LogicError(errors::out_of_range, "level");
		return 0;
	}

	if(sym->ident == iFUNCTN || (sym->vclass == 1 && (sym->codestart > cip || cip >= sym->codeend)))
	{
		amx_LogicError(errors::operation_not_supported, "symbol");
		return 0;
	}

	auto data = amx_GetData(amx);
	cell *ptr;
	if(sym->vclass == 0 || sym->vclass == 2)
	{
		ptr = reinterpret_cast<cell*>(data + sym->address);
	}else{
		cell frm = amx->frm;
		for(cell i = 1; i < level; i++)
		{
			frm = *reinterpret_cast<cell*>(data + frm);
		}
		ptr = reinterpret_cast<cell*>(data + frm + sym->address);
	}

	if(sym->ident == iREFERENCE || sym->ident == iREFARRAY)
	{
		ptr = reinterpret_cast<cell*>(data + *ptr);
	}
	return ptr;
}

cell *find_symbol_addr(AMX *amx, cell index, cell level, AMX_DBG_SYMBOL *&sym)
{
	AMX_DBG *dbg;
	return find_symbol_addr(amx, index, level, dbg, sym);
}

class debug_symbol_var_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
	amx::handle amx;
	AMX_DBG *dbg;
	cell func;

	cell index;
	cell current;

public:
	debug_symbol_var_iterator(AMX *amx, AMX_DBG *dbg, cell func) : amx(amx::load(amx)), dbg(dbg), func(func)
	{

	}

	virtual bool expired() const override
	{
		return amx.expired();
	}

	virtual bool valid() const override
	{
		return !expired() && current >= 0 && current < dbg->hdr->symbols;
	}

	virtual bool move_next() override
	{
		if(current != -2)
		{
			ucell codestart = dbg->symboltbl[func]->codestart;
			ucell codeend = dbg->symboltbl[func]->codeend;
			while(++current < dbg->hdr->symbols)
			{
				auto var = dbg->symboltbl[current];
				if(var->ident != iFUNCTN && var->codestart >= codestart && var->codeend <= codeend)
				{
					index++;
					return true;
				}
			}
		}
		current = -2;
		return false;
	}

	virtual bool move_previous() override
	{
		if(current != -2)
		{
			ucell codestart = dbg->symboltbl[func]->codestart;
			ucell codeend = dbg->symboltbl[func]->codeend;
			while(--current >= 0)
			{
				auto var = dbg->symboltbl[current];
				if(var->ident != iFUNCTN && var->codestart >= codestart && var->codeend <= codeend)
				{
					index--;
					return true;
				}
			}
		}
		current = -2;
		return false;
	}

	virtual bool set_to_first() override
	{
		current = -1;
		index = -1;
		return move_next();
	}

	virtual bool set_to_last() override
	{
		current = dbg->hdr->symbols;
		index = 0;
		return move_previous();
	}

	virtual bool reset() override
	{
		current = -2;
		return true;
	}

	virtual bool erase() override
	{
		return false;
	}

	virtual std::unique_ptr<dyn_iterator> clone() const override
	{
		return std::make_unique<debug_symbol_var_iterator>(*this);
	}

	virtual std::shared_ptr<dyn_iterator> clone_shared() const override
	{
		return std::make_shared<debug_symbol_var_iterator>(*this);
	}

	virtual size_t get_hash() const override
	{
		return std::hash<cell>()(current);
	}

	virtual bool operator==(const dyn_iterator &obj) const override
	{
		auto other = dynamic_cast<const debug_symbol_var_iterator*>(&obj);
		if(other != nullptr)
		{
			return !amx.owner_before(other->amx) && !other->amx.owner_before(amx) && current == other->current;
		}
		return false;
	}

protected:
	virtual bool extract_dyn(const std::type_info &type, void *value) const override
	{
		if(valid())
		{
			if(type == typeid(std::shared_ptr<const dyn_object>))
			{
				*reinterpret_cast<std::shared_ptr<const dyn_object>*>(value) = std::make_shared<dyn_object>(current, tags::find_tag(tags::tag_symbol));
				return true;
			}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
			{
				*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(std::pair<const dyn_object, dyn_object>(dyn_object(index, tags::find_tag(tags::tag_cell)), dyn_object(current, tags::find_tag(tags::tag_symbol))));
				return true;
			}
		}
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, void *value) override
	{
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, const void *value) override
	{
		return false;
	}

public:
	virtual dyn_iterator *get() override
	{
		return this;
	}

	virtual const dyn_iterator *get() const override
	{
		return this;
	}
};

namespace Natives
{
	// native bool:debug_loaded();
	AMX_DEFINE_NATIVE(debug_loaded, 0)
	{
		const auto &obj = amx::load_lock(amx);
		return static_cast<bool>(obj->dbg);
	}

	// native AmxDebug:debug_get_ptr();
	AMX_DEFINE_NATIVE(debug_get_ptr, 0)
	{
		const auto &obj = amx::load_lock(amx);
		if(!obj->dbg)
		{
			return 0;
		}
		return reinterpret_cast<cell>(obj->dbg.get());
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
		auto dbg = get_debug(amx);
		
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

	template <cell Param, class Func>
	static cell debug_file_string(AMX *amx, cell *params, Func f)
	{
		auto dbg = get_debug(amx);

		cell cip = optparam(Param, std::numeric_limits<cell>::min());
		if(cip == std::numeric_limits<cell>::min())
		{
			cip = get_level(amx, 0);
		} else if(cip < 0)
		{
			amx_LogicError(errors::out_of_range, "code");
			return 0;
		}
		const char *filename;
		if(dbg_LookupFile(dbg, cip, &filename) == AMX_ERR_NONE)
		{
			return f(filename);
		}
		return f(nullptr);
	}

	// native debug_file(file[], code=cellmin, size=sizeof(file));
	AMX_DEFINE_NATIVE(debug_file, 3)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		return debug_file_string<2>(amx, params, [&](const char *str) -> cell
		{
			if(str)
			{
				amx_SetString(addr, str, false, false, params[3]);
				return std::strlen(str);
			}
			return 0;
		});
	}

	// native String:debug_file_s(code=cellmin);
	AMX_DEFINE_NATIVE(debug_file_s, 0)
	{
		return debug_file_string<1>(amx, params, [&](const char *str)
		{
			if(str)
			{
				return strings::create(str);
			}
			return 0;
		});
	}

	// native debug_num_symbols();
	AMX_DEFINE_NATIVE(debug_num_symbols, 0)
	{
		auto dbg = get_debug(amx);
		return dbg->hdr->symbols;
	}

	// native Symbol:debug_symbol(const name[], code=cellmin, symbol_kind:kind=symbol_kind:-1, symbol_class:class=symbol_class:-1);
	AMX_DEFINE_NATIVE(debug_symbol, 1)
	{
		auto dbg = get_debug(amx);

		const char *name;
		amx_StrParam(amx, params[1], name);

		if(name == nullptr)
		{
			amx_FormalError(errors::arg_empty, "name");
		}
		
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
			if((sym->ident == iFUNCTN || (sym->codestart <= cip && sym->codeend > cip)) && ((1 << (sym->ident - 1)) & kind) && ((1 << sym->vclass) & cls) && !std::strcmp(sym->name, name))
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
		auto dbg = get_debug(amx);
		
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
		auto dbg = get_debug(amx);

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
			auto data = amx_GetData(amx);
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
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
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
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return 1 << sym->vclass;
	}

	// native debug_symbol_tag(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_tag, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
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
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto insym = dbg->symboltbl[index];

		cell minindex = std::numeric_limits<cell>::min();
		if(insym->vclass == 0)
		{
			return minindex;
		}

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

	// native debug_symbol_name(Symbol:symbol, name[], size=sizeof(name));
	AMX_DEFINE_NATIVE(debug_symbol_name, 3)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		cell *addr = amx_GetAddrSafe(amx, params[2]);
		amx_SetString(addr, sym->name, false, false, params[3]);
		return std::strlen(sym->name);
	}
	
	// native String:debug_symbol_name_s(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_name_s, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return strings::create(sym->name);
	}

	// native debug_symbol_addr(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_addr, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		return sym->address;
	}

	// native debug_symbol_range(Symbol:symbol, &codestart, &codeend);
	AMX_DEFINE_NATIVE(debug_symbol_range, 3)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		cell *start, *end;
		start = amx_GetAddrSafe(amx, params[2]);
		end = amx_GetAddrSafe(amx, params[3]);
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
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
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

	// native debug_symbol_file(Symbol:symbol, file[], size=sizeof(file));
	AMX_DEFINE_NATIVE(debug_symbol_file, 3)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		cell *addr = amx_GetAddrSafe(amx, params[2]);

		const char *filename;
		if(dbg_LookupFile(dbg, sym->codestart, &filename) == AMX_ERR_NONE)
		{
			amx_SetString(addr, filename, false, false, params[3]);
			return std::strlen(filename);
		}
		return 0;
	}

	// native String:debug_symbol_file_s(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_file_s, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
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

	// native bool:debug_symbol_in_scope(Symbol:symbol, level=0);
	AMX_DEFINE_NATIVE(debug_symbol_in_scope, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];

		cell level = optparam(2, 0);
		ucell cip;
		if(level < 0 || (cip = get_level(amx, level)) == -1)
		{
			amx_LogicError(errors::out_of_range, "level");
			return 0;
		}

		return sym->codestart <= cip && cip < sym->codeend;
	}

	// native debug_symbol_set(Symbol:symbol, AnyTag:value, level=0);
	AMX_DEFINE_NATIVE(debug_symbol_set, 2)
	{
		AMX_DBG_SYMBOL *sym;
		cell *ptr = find_symbol_addr(amx, params[1], optparam(3, 0), sym);
		*ptr = params[2];
		return 1;
	}

	// native debug_symbol_get(Symbol:symbol, level=0);
	AMX_DEFINE_NATIVE(debug_symbol_get, 2)
	{
		AMX_DBG_SYMBOL *sym;
		cell *ptr = find_symbol_addr(amx, params[1], optparam(2, 0), sym);
		return *ptr;
	}

	// native bool:debug_symbol_set_safe(Symbol:symbol, AnyTag:value, level=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(debug_symbol_set_safe, 4)
	{
		AMX_DBG *dbg;
		AMX_DBG_SYMBOL *sym;
		cell *ptr = find_symbol_addr(amx, params[1], optparam(3, 0), dbg, sym);
		cell tag = sym->tag;
		const char *tagname;
		if(dbg_GetTagName(dbg, tag, &tagname) == AMX_ERR_NONE)
		{
			if(*tagname >= 'A' && *tagname <= 'Z')
			{
				tag |= 0x40000000;
			}
		}

		auto srctag = tags::find_tag(amx, params[4]);
		if((tag == 0 && !srctag->strong()) || (srctag->inherits_from(tags::find_tag(amx, tag))))
		{
			*ptr = params[2];
			return 1;
		}
		return 0;
	}

	// native bool:debug_symbol_get_safe(Symbol:symbol, &AnyTag:value, level=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(debug_symbol_get_safe, 4)
	{
		AMX_DBG *dbg;
		AMX_DBG_SYMBOL *sym;
		cell *ptr = find_symbol_addr(amx, params[1], optparam(3, 0), dbg, sym);
		cell tag = sym->tag;
		const char *tagname;
		if(dbg_GetTagName(dbg, tag, &tagname) == AMX_ERR_NONE)
		{
			if(*tagname >= 'A' && *tagname <= 'Z')
			{
				tag |= 0x40000000;
			}
		}

		auto srctag = tags::find_tag(amx, tag);
		tag = params[4];
		if((tag == 0 && !srctag->strong()) || (srctag->inherits_from(tags::find_tag(amx, tag))))
		{
			cell *addr = amx_GetAddrSafe(amx, params[2]);
			*addr = *ptr;
			return 1;
		}
		return 0;
	}

	// native debug_symbol_call(Symbol:symbol, AnyTag:...);
	AMX_DEFINE_NATIVE(debug_symbol_call, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		cell argslen = params[0] - sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
			}
		}
		if(error)
		{
			amx_FormalError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnDebugCall | index;
	}

	// native debug_symbol_call_list(Symbol:symbol, List:args);
	AMX_DEFINE_NATIVE(debug_symbol_call_list, 2)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		list_t *list;
		if(!list_pool.get_by_id(params[2], list))
		{
			amx_LogicError(errors::pointer_invalid, "list", params[2]);
			return 0;
		}

		cell argslen = list->size() * sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
			}
		}
		if(error)
		{
			amx_FormalError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return SleepReturnDebugCallList | index;
	}

	// native debug_symbol_indirect_call(Symbol:symbol, AnyTag:...);
	AMX_DEFINE_NATIVE(debug_symbol_indirect_call, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		auto data = amx_GetData(amx);
		auto stk = reinterpret_cast<cell*>(data + amx->stk);

		cell num = *stk - sizeof(cell);
		amx->stk -= num + 2 * sizeof(cell);
		for(cell i = 0; i < num; i += sizeof(cell))
		{
			cell val = *reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + num + sizeof(cell));
			*--stk = val;
		}
		*--stk = num;

		cell argslen = params[0] - sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
				if(vsym->ident == iVARIABLE && addr > 0)
				{
					auto arg = reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + addr);
					cell val = *arg;
					*arg = *reinterpret_cast<cell*>(data + val);
				}
			}
		}
		if(error)
		{
			amx_FormalError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		*--stk = 0;
		amx->cip = sym->address;
		cell ret;
		amx->error = amx_Exec(amx, &ret, AMX_EXEC_CONT);
		return ret;
	}

	// native debug_symbol_indirect_call_list(Symbol:symbol, List:args);
	AMX_DEFINE_NATIVE(debug_symbol_indirect_call_list, 2)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		list_t *list;
		if(!list_pool.get_by_id(params[2], list))
		{
			amx_LogicError(errors::pointer_invalid, "list", params[2]);
			return 0;
		}

		auto data = amx_GetData(amx);
		auto stk = reinterpret_cast<cell*>(data + amx->stk);

		cell argslen = list->size() * sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
			}
		}
		if(error)
		{
			amx_LogicError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		cell num = list->size() * sizeof(cell);
		amx->stk -= num + 2 * sizeof(cell);
		for(cell i = list->size() - 1; i >= 0; i--)
		{
			cell val = (*list)[i].store(amx);
			*--stk = val;
		}
		*--stk = num;
		*--stk = 0;
		amx->cip = sym->address;
		cell ret;
		amx->error = amx_Exec(amx, &ret, AMX_EXEC_CONT);
		return ret;
	}

	// native amx_err:debug_symbol_try_call(Symbol:symbol, &result, AnyTag:...);
	AMX_DEFINE_NATIVE(debug_symbol_try_call, 2)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		auto data = amx_GetData(amx);
		auto stk = reinterpret_cast<cell*>(data + amx->stk);

		cell num = *stk - sizeof(cell);
		amx->stk -= num + 2 * sizeof(cell);
		for(cell i = 0; i < num; i += sizeof(cell))
		{
			cell val = *reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + num + 2 * sizeof(cell));
			*--stk = val;
		}
		*--stk = num;

		cell argslen = params[0] - 2 * sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
				if(vsym->ident == iVARIABLE && addr > 0)
				{
					auto arg = reinterpret_cast<cell*>(reinterpret_cast<unsigned char*>(stk) + addr);
					cell val = *arg;
					*arg = *reinterpret_cast<cell*>(data + val);
				}
			}
		}
		if(error)
		{
			amx_FormalError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		*--stk = 0;
		amx->cip = sym->address;
		int err = amx_Exec(amx, amx_GetAddrSafe(amx, params[2]), AMX_EXEC_CONT);
		amx->error = AMX_ERR_NONE;
		return err;
	}

	// native amx_err:debug_symbol_try_call_list(Symbol:symbol, &result, List:args);
	AMX_DEFINE_NATIVE(debug_symbol_try_call_list, 3)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}
		auto sym = dbg->symboltbl[index];
		if(sym->ident != iFUNCTN)
		{
			amx_LogicError(errors::operation_not_supported, "symbol");
			return 0;
		}

		list_t *list;
		if(!list_pool.get_by_id(params[3], list))
		{
			amx_LogicError(errors::pointer_invalid, "list", params[3]);
			return 0;
		}

		auto data = amx_GetData(amx);
		auto stk = reinterpret_cast<cell*>(data + amx->stk);

		cell argslen = list->size() * sizeof(cell);
		cell argsneeded = 0;
		bool error = false;
		for(uint16_t i = 0; i < dbg->hdr->symbols; i++)
		{
			auto vsym = dbg->symboltbl[i];
			if(vsym->ident != iFUNCTN && vsym->vclass == 1 && vsym->codestart >= sym->codestart && vsym->codeend <= sym->codeend)
			{
				cell addr = vsym->address - 2 * sizeof(cell);
				if(addr > argslen)
				{
					error = true;
					if(addr > argsneeded)
					{
						argsneeded = addr;
					}
				}
			}
		}
		if(error)
		{
			amx_LogicError(errors::not_enough_args, argsneeded / sizeof(cell), argslen / sizeof(cell));
			return 0;
		}

		cell num = list->size() * sizeof(cell);
		amx->stk -= num + 2 * sizeof(cell);
		for(cell i = list->size() - 1; i >= 0; i--)
		{
			cell val = (*list)[i].store(amx);
			*--stk = val;
		}
		*--stk = num;
		*--stk = 0;
		amx->cip = sym->address;
		int err = amx_Exec(amx, amx_GetAddrSafe(amx, params[2]), AMX_EXEC_CONT);
		amx->error = AMX_ERR_NONE;
		return err;
	}

	// native Iter:debug_symbol_variables(Symbol:symbol);
	AMX_DEFINE_NATIVE(debug_symbol_variables, 1)
	{
		auto dbg = get_debug(amx);

		cell index = params[1];
		if(index < 0 || index >= dbg->hdr->symbols)
		{
			amx_LogicError(errors::out_of_range, "symbol");
			return 0;
		}

		auto &iter = iter_pool.emplace_derived<debug_symbol_var_iterator>(amx, dbg, index);
		iter->set_to_first();
		return iter_pool.get_id(iter);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(debug_loaded),
	AMX_DECLARE_NATIVE(debug_get_ptr),
	AMX_DECLARE_NATIVE(debug_code),
	AMX_DECLARE_NATIVE(debug_line),
	AMX_DECLARE_NATIVE(debug_file),
	AMX_DECLARE_NATIVE(debug_file_s),
	AMX_DECLARE_NATIVE(debug_num_symbols),
	AMX_DECLARE_NATIVE(debug_symbol),
	AMX_DECLARE_NATIVE(debug_func),
	AMX_DECLARE_NATIVE(debug_var),
	AMX_DECLARE_NATIVE(debug_symbol_kind),
	AMX_DECLARE_NATIVE(debug_symbol_class),
	AMX_DECLARE_NATIVE(debug_symbol_tag),
	AMX_DECLARE_NATIVE(debug_symbol_func),
	AMX_DECLARE_NATIVE(debug_symbol_name),
	AMX_DECLARE_NATIVE(debug_symbol_name_s),
	AMX_DECLARE_NATIVE(debug_symbol_addr),
	AMX_DECLARE_NATIVE(debug_symbol_range),
	AMX_DECLARE_NATIVE(debug_symbol_line),
	AMX_DECLARE_NATIVE(debug_symbol_file),
	AMX_DECLARE_NATIVE(debug_symbol_file_s),
	AMX_DECLARE_NATIVE(debug_symbol_in_scope),
	AMX_DECLARE_NATIVE(debug_symbol_set),
	AMX_DECLARE_NATIVE(debug_symbol_get),
	AMX_DECLARE_NATIVE(debug_symbol_set_safe),
	AMX_DECLARE_NATIVE(debug_symbol_get_safe),
	AMX_DECLARE_NATIVE(debug_symbol_call),
	AMX_DECLARE_NATIVE(debug_symbol_call_list),
	AMX_DECLARE_NATIVE(debug_symbol_indirect_call),
	AMX_DECLARE_NATIVE(debug_symbol_indirect_call_list),
	AMX_DECLARE_NATIVE(debug_symbol_try_call),
	AMX_DECLARE_NATIVE(debug_symbol_try_call_list),
	AMX_DECLARE_NATIVE(debug_symbol_variables),
};

int RegisterDebugNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
