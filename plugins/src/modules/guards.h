#ifndef GUARDS_H_INCLUDED
#define GUARDS_H_INCLUDED

#include "objects/dyn_object.h"
#include "sdk/amx/amx.h"

namespace guards
{
	size_t count(AMX *amx);
	dyn_object *add(AMX *amx, dyn_object &&obj);
	bool contains(AMX *amx, const dyn_object *obj);
	bool free(AMX *amx, dyn_object *obj);
}

#endif
