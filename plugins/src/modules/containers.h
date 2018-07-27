#ifndef CONTAINERS_H_INCLUDED
#define CONTAINERS_H_INCLUDED

#include "objects/dyn_object.h"
#include "utils/shared_id_set_pool.h"

#include <vector>
#include <unordered_map>

typedef std::vector<dyn_object> list_t;
typedef std::unordered_map<dyn_object, dyn_object> map_t;

extern aux::shared_id_set_pool<list_t> list_pool;
extern aux::shared_id_set_pool<map_t> map_pool;

#endif
