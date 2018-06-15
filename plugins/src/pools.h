#ifndef POOLS_H_INCLUDED
#define POOLS_H_INCLUDED

#include "utils/set_pool.h"
#include "objects/dyn_object.h"
#include "modules/amxutils.h"
#include <vector>
#include <unordered_map>

typedef std::vector<dyn_object> list_t;
typedef std::unordered_map<dyn_object, dyn_object> map_t;

extern aux::set_pool<list_t> list_pool;
extern aux::set_pool<map_t> map_pool;
extern aux::set_pool<amx_var_info> amx_var_pool;

#endif
