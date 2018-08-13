#ifndef NATIVES_H_INCLUDED
#define NATIVES_H_INCLUDED

#include "main.h"
#include "sdk/amx/amx.h"

#define AMX_DECLARE_NATIVE(Name) {#Name, Natives::Name}

int RegisterConfigNatives(AMX *amx);
int RegisterPawnNatives(AMX *amx);
int RegisterStringsNatives(AMX *amx);
int RegisterTasksNatives(AMX *amx);
int RegisterThreadNatives(AMX *amx);
int RegisterVariantNatives(AMX *amx);
int RegisterListNatives(AMX *amx);
int RegisterMapNatives(AMX *amx);
int RegisterIterNatives(AMX *amx);
int RegisterTagNatives(AMX *amx);
int RegisterAmxNatives(AMX *amx);
int RegisterLinkedListNatives(AMX *amx);

inline int RegisterNatives(AMX *amx)
{
	RegisterConfigNatives(amx);
	RegisterPawnNatives(amx);
	RegisterStringsNatives(amx);
	RegisterTasksNatives(amx);
	RegisterThreadNatives(amx);
	RegisterVariantNatives(amx);
	RegisterListNatives(amx);
	RegisterMapNatives(amx);
	RegisterIterNatives(amx);
	RegisterTagNatives(amx);
	RegisterAmxNatives(amx);
	RegisterLinkedListNatives(amx);
	return AMX_ERR_NONE;
}

#endif
