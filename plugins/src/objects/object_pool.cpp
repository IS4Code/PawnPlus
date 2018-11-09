#include "object_pool.h"
#include "dyn_object.h"
#include "modules/strings.h"
#include "modules/containers.h"

template <class ObjType>
typename object_pool<ObjType>::ref_container_null object_pool<ObjType>::null_ref = nullptr;

template <class ObjType>
struct ref_ctor_simple
{
	static typename object_pool<ObjType>::object_ptr add(typename object_pool<ObjType>::list_type &list)
	{
		return *list.add();
	}

	static typename object_pool<ObjType>::object_ptr add(typename object_pool<ObjType>::list_type &list, typename object_pool<ObjType>::ref_container &&obj)
	{
		return *list.add(std::move(obj));
	}
};

template <class ObjType>
struct ref_ctor_virtual
{
	static typename object_pool<ObjType>::object_ptr add(typename object_pool<ObjType>::list_type &list)
	{
		return object_pool<ObjType>::null_ref;
	}

	static typename object_pool<ObjType>::object_ptr add(typename object_pool<ObjType>::list_type &list, typename object_pool<ObjType>::ref_container &&obj)
	{
		return object_pool<ObjType>::null_ref;
	}
};

template <class ObjType>
auto object_pool<ObjType>::add() -> object_ptr
{
	typedef typename std::conditional<std::has_virtual_destructor<ObjType>::value, ref_ctor_virtual<ObjType>, ref_ctor_simple<ObjType>>::type ctor;
	return ctor::add(local_object_list);
}

template <class ObjType>
auto object_pool<ObjType>::add(ref_container &&obj) -> object_ptr
{
	typedef typename std::conditional<std::has_virtual_destructor<ObjType>::value, ref_ctor_virtual<ObjType>, ref_ctor_simple<ObjType>>::type ctor;
	return ctor::add(local_object_list, std::move(obj));
}

template <class ObjType>
auto object_pool<ObjType>::add(std::unique_ptr<ref_container> &&obj) -> object_ptr
{
	return *local_object_list.add(std::move(obj));
}

template <class ObjType>
cell object_pool<ObjType>::get_address(AMX *amx, const_object_ptr obj) const
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(&obj) - reinterpret_cast<cell>(data);
}

template <class ObjType>
cell object_pool<ObjType>::get_inner_address(AMX *amx, const_object_ptr obj) const
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(&obj->operator[](0)) - reinterpret_cast<cell>(data);
}

template <class ObjType>
bool object_pool<ObjType>::is_null_address(AMX *amx, cell addr) const
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(data) == -addr;
}

template <class ObjType>
bool object_pool<ObjType>::acquire_ref(object_ptr obj)
{
	bool local = obj.local();
	if(obj.acquire())
	{
		if(local)
		{
			auto it = local_object_list.find(&obj);
			if(it != local_object_list.end())
			{
				auto ptr = local_object_list.extract(it);
				global_object_list.add(std::move(ptr));
			}
		}
		return true;
	}
	return false;
}

template <class ObjType>
bool object_pool<ObjType>::release_ref(object_ptr obj)
{
	if(obj.release())
	{
		if(obj.local())
		{
			auto it = global_object_list.find(&obj);
			if(it != global_object_list.end())
			{
				auto ptr = global_object_list.extract(it);
				local_object_list.add(std::move(ptr));
			}
		}
		return true;
	}
	return false;
}

template <class ObjType>
void object_pool<ObjType>::set_cache(const_object_ptr obj)
{
	inner_cache[&obj->operator[](0)] = &obj;
}

template <class ObjType>
auto object_pool<ObjType>::find_cache(const_inner_ptr ptr) -> object_ptr
{
	auto it = inner_cache.find(ptr);
	if(it != inner_cache.end())
	{
		return const_cast<object_ptr>(*it->second);
	}
	return null_ref;
}

template <class ObjType>
bool object_pool<ObjType>::remove(object_ptr obj)
{
	auto it = global_object_list.find(&obj);
	if(it != global_object_list.end())
	{
		global_object_list.erase(it);
		return true;
	}
	it = local_object_list.find(&obj);
	if(it != local_object_list.end())
	{
		local_object_list.erase(it);
		return true;
	}
	return false;
}

template <class ObjType>
bool object_pool<ObjType>::remove_by_id(cell id)
{
	auto obj = reinterpret_cast<ref_container*>(id);
	auto it = global_object_list.find(obj);
	if(it != global_object_list.end())
	{
		global_object_list.erase(it);
		return true;
	}
	it = local_object_list.find(obj);
	if(it != local_object_list.end())
	{
		local_object_list.erase(it);
		return true;
	}
	return false;
}

template <class ObjType>
auto object_pool<ObjType>::get(AMX *amx, cell addr) -> object_ptr
{
	auto obj = reinterpret_cast<ref_container*>((amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + addr);
	
	auto it = local_object_list.find(obj);
	if(it != local_object_list.end())
	{
		return **it;
	}
	it = global_object_list.find(obj);
	if(it != global_object_list.end())
	{
		return **it;
	}
	return null_ref;
}

template <class ObjType>
void object_pool<ObjType>::clear()
{
	inner_cache.clear();
	auto tmp = std::move(local_object_list);
	tmp.clear();
	auto list = std::move(global_object_list);
	list.clear();
}

template <class ObjType>
void object_pool<ObjType>::clear_tmp()
{
	inner_cache.clear();
	auto tmp = std::move(local_object_list);
	tmp.clear();
}

template <class ObjType>
bool object_pool<ObjType>::get_by_id(cell id, ref_container *&obj)
{
	obj = reinterpret_cast<ref_container*>(id);

	if(local_object_list.find(obj) != local_object_list.end())
	{
		return true;
	}
	if(global_object_list.find(obj) != global_object_list.end())
	{
		return true;
	}
	return false;
}

template <class ObjType>
bool object_pool<ObjType>::get_by_id(cell id, ObjType *&obj)
{
	auto ptr = reinterpret_cast<ref_container*>(id);

	if(local_object_list.find(ptr) != local_object_list.end())
	{
		obj = *ptr;
		return true;
	}
	if(global_object_list.find(ptr) != global_object_list.end())
	{
		obj = *ptr;
		return true;
	}
	return false;
}

template <class ObjType>
cell object_pool<ObjType>::get_id(const_object_ptr obj) const
{
	return reinterpret_cast<cell>(&obj);
}

template <class ObjType>
size_t object_pool<ObjType>::local_size() const
{
	return local_object_list.size();
}

template <class ObjType>
size_t object_pool<ObjType>::global_size() const
{
	return global_object_list.size();
}

template class object_pool<strings::cell_string>;
template class object_pool<dyn_object>;
template class object_pool<dyn_iterator>;
