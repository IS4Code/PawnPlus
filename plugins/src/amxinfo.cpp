#include "amxinfo.h"
#include "modules/tags.h"
#include <unordered_map>

struct amx_info
{
	std::unordered_map<std::string, AMX_NATIVE> natives;
};

static std::unordered_map<AMX*, amx_info> amx_map;

void amx::load(AMX *amx)
{
	auto &info = amx_map[amx];
}

void amx::unload(AMX *amx)
{
	amx_map.erase(amx);
}

void amx::register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
{
	auto &info = amx_map[amx];
	for(int i = 0; nativelist[i].name != nullptr && (i < number || number == -1); i++)
	{
		info.natives.insert(std::make_pair(nativelist[i].name, nativelist[i].func));
	}
}

AMX_NATIVE amx::find_native(AMX *amx, const char *name)
{
	return amx::find_native(amx, std::string(name));
}

AMX_NATIVE amx::find_native(AMX *amx, const std::string &name)
{
	auto &info = amx_map[amx];
	auto it = info.natives.find(name);
	if(it != info.natives.end())
	{
		return it->second;
	}
	int index;
	if(!amx_FindNative(amx, name.c_str(), &index))
	{
		auto amxhdr = (AMX_HEADER*)amx->base;
		auto func = (AMX_FUNCSTUB*)((unsigned char*)amxhdr+ amxhdr->natives + index* amxhdr->defsize);
		auto f = reinterpret_cast<AMX_NATIVE>(func->address);
		info.natives.insert(std::make_pair(name, f));
		return f;
	}
	return nullptr;
}
