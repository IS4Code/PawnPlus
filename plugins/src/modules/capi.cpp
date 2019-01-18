#include "capi.h"
#include "tags.h"
#include "variants.h"
#include "containers.h"
#include "strings.h"

struct func_ptr
{
	void *ptr;

	func_ptr(std::nullptr_t) : ptr(nullptr)
	{

	}

	template <class Ret, class... Args>
	func_ptr(Ret(*func)(Args...)) : ptr(reinterpret_cast<void*>(func))
	{

	}
};

static func_ptr tag_functions[] = {
	+[]/*find_tag_name*/(const char *name) -> const void*
	{
		return tags::find_tag(name);
	},
	+[]/*find_tag_id*/(cell tag_uid) -> const void*
	{
		return tags::find_tag(tag_uid);
	},
	nullptr
};

static func_ptr dyn_object_functions[] = {
	+[]/*new_dyn_object_empty*/() -> void*
	{
		return new dyn_object();
	},
	+[]/*new_dyn_object_copy*/(const void *obj) -> void*
	{
		return new dyn_object(*static_cast<const dyn_object*>(obj));
	},
	+[]/*new_dyn_object_move*/(void *obj) -> void*
	{
		return new dyn_object(std::move(*static_cast<dyn_object*>(obj)));
	},
	+[]/*new_dyn_object_cell*/(cell value, const void *tag) -> void*
	{
		return new dyn_object(value, static_cast<tag_ptr>(tag));
	},
	+[]/*new_dyn_object_arr*/(const cell *arr, cell size, const void *tag) -> void*
	{
		return new dyn_object(arr, size, static_cast<tag_ptr>(tag));
	},
	+[]/*new_dyn_object_str*/(const cell *str) -> void*
	{
		return new dyn_object(str);
	},
	+[]/*delete_dyn_object*/(void *obj) -> void
	{
		delete static_cast<dyn_object*>(obj);
	},
	+[]/*assign_dyn_object_copy*/(void *self, const void *obj) -> void
	{
		*static_cast<dyn_object*>(self) = *static_cast<const dyn_object*>(obj);
	},
	+[]/*assign_dyn_object_move*/(void *self, void *obj) -> void
	{
		*static_cast<dyn_object*>(self) = std::move(*static_cast<dyn_object*>(obj));
	},
	+[]/*dyn_object_get_tag*/(const void *obj) -> const void*
	{
		return static_cast<const dyn_object*>(obj)->get_tag();
	},
	+[]/*dyn_object_get_size*/(const void *obj) -> cell
	{
		return static_cast<const dyn_object*>(obj)->get_size();
	},
	+[]/*dyn_object_is_array*/(const void *obj) -> cell
	{
		return static_cast<const dyn_object*>(obj)->is_array();
	},
	+[]/*dyn_object_get_cell*/(const void *obj, cell index) -> cell
	{
		cell val;
		if(static_cast<const dyn_object*>(obj)->get_cell(index, val))
		{
			return val;
		}
		return 0;
	},
	+[]/*dyn_object_get_array*/(const void *obj, cell *arr, cell maxsize) -> cell
	{
		return static_cast<const dyn_object*>(obj)->get_array(arr, maxsize);
	},
	+[]/*dyn_object_acquire*/(void *obj) -> void
	{
		static_cast<const dyn_object*>(obj)->acquire();
	},
	+[]/*dyn_object_release*/(void *obj) -> void
	{
		static_cast<const dyn_object*>(obj)->release();
	},
	nullptr
};

static func_ptr list_functions[] = {
	+[]/*new_list*/() -> void*
	{
		return &*list_pool.add();
	},
	+[]/*delete_list*/(void *list) -> void
	{
		list_pool.remove(static_cast<list_t*>(list));
	},
	+[]/*list_get_id*/(void *list) -> cell
	{
		return list_pool.get_id(static_cast<list_t*>(list));
	},
	+[]/*list_from_id*/(cell id) -> void*
	{
		list_t *ptr;
		if(list_pool.get_by_id(id, ptr))
		{
			return ptr;
		}
		return nullptr;
	},
	+[]/*list_get_size*/(const void *list) -> cell
	{
		return static_cast<const list_t*>(list)->size();
	},
	+[]/*list_get*/(void *list, cell index) -> void*
	{
		return &((*static_cast<list_t*>(list))[index]);
	},
	+[]/*list_add_copy*/(void *list, const void *obj) -> void
	{
		static_cast<list_t*>(list)->push_back(*static_cast<const dyn_object*>(obj));
	},
	+[]/*list_add_move*/(void *list, void *obj) -> void
	{
		static_cast<list_t*>(list)->push_back(std::move(*static_cast<dyn_object*>(obj)));
	},
	+[]/*list_clear*/(void *list) -> void
	{
		static_cast<list_t*>(list)->clear();
	},
	nullptr
};

static func_ptr linked_list_functions[] = {
	+[]/*new_linked_list*/() -> void*
	{
		return &*linked_list_pool.add();
	},
	+[]/*delete_linked_list*/(void *linked_list) -> void
	{
		linked_list_pool.remove(static_cast<linked_list_t*>(linked_list));
	},
	+[]/*linked_list_get_id*/(void *linked_list) -> cell
	{
		return linked_list_pool.get_id(static_cast<linked_list_t*>(linked_list));
	},
	+[]/*linked_list_from_id*/(cell id) -> void*
	{
		linked_list_t *ptr;
		if(linked_list_pool.get_by_id(id, ptr))
		{
			return ptr;
		}
		return nullptr;
	},
	+[]/*linked_list_get_size*/(const void *linked_list) -> cell
	{
		return static_cast<const linked_list_t*>(linked_list)->size();
	},
	+[]/*linked_list_get*/(void *linked_list, cell index) -> void*
	{
		return &((*static_cast<linked_list_t*>(linked_list))[index]);
	},
	+[]/*linked_list_add_copy*/(void *linked_list, const void *obj) -> void
	{
		static_cast<linked_list_t*>(linked_list)->push_back(*static_cast<const dyn_object*>(obj));
	},
	+[]/*linked_list_add_move*/(void *linked_list, void *obj) -> void
	{
		static_cast<linked_list_t*>(linked_list)->push_back(std::move(*static_cast<dyn_object*>(obj)));
	},
	+[]/*linked_list_clear*/(void *linked_list) -> void
	{
		static_cast<linked_list_t*>(linked_list)->clear();
	},
	nullptr
};

static func_ptr map_functions[] = {
	+[]/*new_map*/() -> void*
	{
		return &*map_pool.add();
	},
	+[]/*delete_map*/(void *map) -> void
	{
		map_pool.remove(static_cast<map_t*>(map));
	},
	+[]/*map_get_id*/(void *map) -> cell
	{
		return map_pool.get_id(static_cast<map_t*>(map));
	},
	+[]/*map_from_id*/(cell id) -> void*
	{
		map_t *ptr;
		if(map_pool.get_by_id(id, ptr))
		{
			return ptr;
		}
		return nullptr;
	},
	+[]/*map_get_size*/(const void *map) -> cell
	{
		return static_cast<const map_t*>(map)->size();
	},
	+[]/*map_get*/(void *map, const void *key) -> void*
	{
		return &((*static_cast<map_t*>(map))[*static_cast<const dyn_object*>(key)]);
	},
	+[]/*map_find*/(void *map, const void *key) -> void*
	{
		map_t &mapobj = *static_cast<map_t*>(map);
		auto it = mapobj.find(*static_cast<const dyn_object*>(key));
		if(it != mapobj.end())
		{
			return &it->second;
		}
		return nullptr;
	},
	+[]/*map_add_copy*/(void *map, const void *key, const void *value) -> cell
	{
		return static_cast<map_t*>(map)->insert(std::make_pair(*static_cast<const dyn_object*>(key), *static_cast<const dyn_object*>(value))).second;
	},
	+[]/*map_add_move*/(void *map, const void *key, void *value) -> cell
	{
		return static_cast<map_t*>(map)->insert(std::make_pair(*static_cast<const dyn_object*>(key), std::move(*static_cast<dyn_object*>(value)))).second;
	},
	+[]/*map_clear*/(void *map) -> void
	{
		static_cast<map_t*>(map)->clear();
	},
	+[]/*map_is_ordered*/(void *map) -> cell
	{
		return static_cast<map_t*>(map)->ordered();
	},
	+[]/*map_set_ordered*/(void *map, cell ordered) -> void
	{
		static_cast<map_t*>(map)->set_ordered(ordered);
	},
	nullptr
};

static void *func_table[] = {
	&tag_functions,
	&dyn_object_functions,
	&list_functions,
	&linked_list_functions,
	&map_functions,
	nullptr
};

void ***get_api_table()
{
	return reinterpret_cast<void***>(&func_table);
}
