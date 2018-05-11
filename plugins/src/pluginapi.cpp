#include "pluginapi.h"
#include <unordered_map>
#include <string>

std::unordered_map<std::string, PLUGIN_INFO> plugin_map;

void MyRegisterPlugin(const char *name, const PLUGIN_INFO *info)
{
	plugin_map[name] = *info;
}

int MyFindPlugin(const char *name, PLUGIN_INFO *info)
{
	auto it = plugin_map.find(name);
	if(it != plugin_map.end())
	{
		*info = it->second;
		return 1;
	}
	return 0;
}

void(*SharedRegisterPlugin)(const char *name, const PLUGIN_INFO *info) = nullptr;
int(*SharedFindPlugin)(const char *name, PLUGIN_INFO *info) = nullptr;

void RegisterPlugin(const char *name, const PLUGIN_INFO *info)
{
	if(SharedRegisterPlugin != nullptr) SharedRegisterPlugin(name, info);
	else MyRegisterPlugin(name, info);
}

int FindPlugin(const char *name, PLUGIN_INFO *info)
{
	if(SharedFindPlugin != nullptr) return SharedFindPlugin(name, info);
	else return MyFindPlugin(name, info);
}

void InitPluginApi(void *ppData[256])
{
	constexpr const long long checkMagic0 = 0x7e7bf7d569204c6f;
	constexpr const long long checkMagic1 = 0x9a6ec459a0af3110;

	auto codeCheck = reinterpret_cast<long long *>(ppData + 0xF0);
	auto &func1 = ppData[0xF8];
	auto &func2 = ppData[0xF9];

	if(codeCheck[0] == checkMagic0 && codeCheck[1] == checkMagic1 && func1 && func2)
	{
		SharedRegisterPlugin = reinterpret_cast<decltype(SharedRegisterPlugin)>(ppData[8]);
		SharedFindPlugin = reinterpret_cast<decltype(SharedFindPlugin)>(ppData[9]);
	}else{
		codeCheck[0] = checkMagic0;
		codeCheck[1] = checkMagic1;
		func1 = reinterpret_cast<void*>(&MyRegisterPlugin);
		func2 = reinterpret_cast<void*>(&MyFindPlugin);
	}
}
