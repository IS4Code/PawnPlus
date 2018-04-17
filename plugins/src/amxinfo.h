#ifndef AMXINFO_H_INCLUDED
#define AMXINFO_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>

namespace amx
{
	void load(AMX *amx);
	void unload(AMX *amx);
	void register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
	AMX_NATIVE find_native(AMX *amx, const char *name);
	AMX_NATIVE find_native(AMX *amx, const std::string &name);
}

#endif
