#ifndef NATIVES_H_INCLUDED
#define NATIVES_H_INCLUDED

#include "sdk/amx/amx.h"

#define AMX_DECLARE_NATIVE(Name) {#Name, Natives::Name}

int RegisterConfigNatives(AMX *amx);
int RegisterPawnNatives(AMX *amx);
int RegisterStringsNatives(AMX *amx);
int RegisterTasksNatives(AMX *amx);
int RegisterThreadNatives(AMX *amx);

inline int RegisterNatives(AMX *amx)
{
	RegisterConfigNatives(amx);
	RegisterPawnNatives(amx);
	RegisterStringsNatives(amx);
	RegisterTasksNatives(amx);
	RegisterThreadNatives(amx);
	return AMX_ERR_NONE;
}

#endif
