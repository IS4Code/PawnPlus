#include "amxinfo.h"
#include "modules/tags.h"
#include <unordered_map>

struct natives_extra : public amx::extra
{
	natives_extra(AMX *amx) : extra(amx)
	{

	}

	std::unordered_map<std::string, AMX_NATIVE> natives;
};

static std::unordered_map<AMX*, std::shared_ptr<amx::instance>> amx_map;

amx::handle amx::load(AMX *amx)
{
	return load_lock(amx);
}

// Nothing should be loaded from the AMX here, since it may not even be initialized yet
amx::object amx::load_lock(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		return it->second;
	}
	auto ptr = std::shared_ptr<instance>(new instance(amx));
	amx_map.insert(std::make_pair(amx, ptr));
	return ptr;
}

bool amx::unload(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		amx_map.erase(it);
		return true;
	}
	return false;
}

bool amx::invalidate(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		it->second->invalidate();
		return true;
	}
	return false;
}

void amx::register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
{
	auto obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;
	for(int i = 0; nativelist[i].name != nullptr && (i < number || number == -1); i++)
	{
		natives.insert(std::make_pair(nativelist[i].name, nativelist[i].func));
	}
}

AMX_NATIVE amx::find_native(AMX *amx, const char *name)
{
	return amx::find_native(amx, std::string(name));
}

AMX_NATIVE amx::find_native(AMX *amx, const std::string &name)
{
	auto obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;

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
