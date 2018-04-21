#ifndef PLUGINAPI_H_INCLUDED
#define PLUGINAPI_H_INCLUDED

struct PLUGIN_INFO
{
	int func_count;
	void *func_list;
};

void InitPluginApi(void *ppData[256]);
void RegisterPlugin(const char *name, const PLUGIN_INFO *info);
int FindPlugin(const char *name, PLUGIN_INFO *info);

#endif
