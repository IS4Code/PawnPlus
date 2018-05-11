#include "capi.h"
#include "modules/strings.h"
#include <string>

void *dyn_string(const char *str, int temp)
{
	return strings::create(std::string(str), temp);
}

void *func_list[] = {
	reinterpret_cast<void*>(&dyn_string)
};

PLUGIN_INFO capi::info = {
	sizeof(func_list) / sizeof(*func_list),
	func_list
};
