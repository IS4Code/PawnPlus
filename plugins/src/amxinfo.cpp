#include "amxinfo.h"
#include "modules/tags.h"
#include <unordered_map>

struct natives_extra : public amx::extra
{
	natives_extra(AMX *amx) : extra(amx)
	{

	}

	natives_extra(AMX *amx, const std::unordered_map<std::string, AMX_NATIVE> &natives) : extra(amx), natives(natives)
	{

	}

	std::unordered_map<std::string, AMX_NATIVE> natives;

	virtual std::unique_ptr<extra> clone() override
	{
		return std::unique_ptr<extra>(new natives_extra(_amx, natives));
	}
};

static std::unordered_map<AMX*, std::shared_ptr<amx::instance>> amx_map;

bool amx::valid(AMX *amx)
{
	return amx_map.find(amx) != amx_map.end();
}

void amx::call_all(void(*func)(void *cookie, AMX *amx), void *cookie)
{
	for(const auto &pair : amx_map)
	{
		if(pair.second->valid())
		{
			func(cookie, pair.first);
		}
	}
}

// Nothing should be loaded from the AMX here, since it may not even be initialized yet
const amx::object &amx::load_lock(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		return it->second;
	}
	return amx_map.emplace(amx, std::make_shared<instance>(amx)).first->second;
}

const amx::object &amx::clone_lock(AMX *amx, AMX *new_amx)
{
	const auto &obj = load_lock(amx);
	return amx_map.emplace(new_amx, std::make_shared<instance>(*obj, new_amx)).first->second;
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
	const auto &obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;
	for(int i = 0; nativelist[i].name != nullptr && (i < number || number == -1); i++)
	{
		natives.insert(std::make_pair(nativelist[i].name, nativelist[i].func));
	}
}

AMX_NATIVE amx::try_decode_native(const char *str)
{
	if(str[0] == 0x1B && str[1] && str[2] && str[3] && str[4] && str[5] && !str[6])
	{
		auto ustr = reinterpret_cast<const unsigned char *>(str);
		uintptr_t ptr =
			static_cast<uintptr_t>(ustr[1] - 1) +
			static_cast<uintptr_t>(ustr[2] - 1) * 255 +
			static_cast<uintptr_t>(ustr[3] - 1) * 65025 +
			static_cast<uintptr_t>(ustr[4] - 1) * 16581375 +
			static_cast<uintptr_t>(ustr[5] - 1) * 4228250625;
		return reinterpret_cast<AMX_NATIVE>(ptr);
	}
	return nullptr;
}

AMX_NATIVE amx::find_native(AMX *amx, const char *name)
{
	auto decoded = try_decode_native(name);
	if(decoded != nullptr)
	{
		return decoded;
	}
	return amx::find_native(amx, std::string(name));
}

AMX_NATIVE amx::find_native(AMX *amx, const std::string &name)
{
	auto decoded = try_decode_native(name.c_str());
	if(decoded != nullptr)
	{
		return decoded;
	}
	const auto &obj = load_lock(amx);
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

size_t amx::num_natives(AMX *amx)
{
	const auto &obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;
	return natives.size();
}
