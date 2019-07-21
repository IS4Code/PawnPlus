#ifndef GUARDS_H_INCLUDED
#define GUARDS_H_INCLUDED

#include "modules/containers.h"
#include "objects/dyn_object.h"
#include "sdk/amx/amx.h"

namespace guards
{
	size_t count(AMX *amx);
	handle_t *add(AMX *amx, dyn_object &&obj);
	cell get_id(AMX *amx, const handle_t *obj);
	bool get_by_id(AMX *amx, cell id, handle_t *&obj);
	bool contains(AMX *amx, const handle_t *obj);
	bool free(AMX *amx, handle_t *obj);
}

namespace amx_guards
{
	size_t count(AMX *amx);
	handle_t *add(AMX *amx, dyn_object &&obj);
	cell get_id(AMX *amx, const handle_t *obj);
	bool get_by_id(AMX *amx, cell id, handle_t *&obj);
	bool contains(AMX *amx, const handle_t *obj);
	bool free(AMX *amx, handle_t *obj);
}

#endif
