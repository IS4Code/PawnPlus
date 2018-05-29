#include "amxinfo.h"
#include "modules/tags.h"
#include <unordered_map>

static std::unordered_map<AMX*, amx::ptr> amx_map;

amx::ptr amx::load(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		return it->second;
	}
	auto ptr = amx::ptr(new ptr_info(amx));
	amx_map.insert(std::make_pair(amx, ptr));
	return ptr;
}

void amx::unload(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		it->second->invalidate();
		amx_map.erase(it);
	}
}

void amx::register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
{
	auto ptr = load(amx);
	for(int i = 0; nativelist[i].name != nullptr && (i < number || number == -1); i++)
	{
		ptr->natives.insert(std::make_pair(nativelist[i].name, nativelist[i].func));
	}
}

AMX_NATIVE amx::find_native(AMX *amx, const char *name)
{
	return amx::find_native(amx, std::string(name));
}

AMX_NATIVE amx::find_native(AMX *amx, const std::string &name)
{
	auto ptr = load(amx);

	auto &natives = ptr->natives;

	auto it = natives.find(name);
	if(it != natives.end())
	{
		return it->second;
	}
	int index;
	if(!amx_FindNative(amx, name.c_str(), &index))
	{
		auto amxhdr = (AMX_HEADER*)amx->base;
		auto func = (AMX_FUNCSTUB*)((unsigned char*)amxhdr+ amxhdr->natives + index* amxhdr->defsize);
		auto f = reinterpret_cast<AMX_NATIVE>(func->address);
		natives.insert(std::make_pair(name, f));
		return f;
	}
	return nullptr;
}
