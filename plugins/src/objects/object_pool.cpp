#include "object_pool.h"
#include "dyn_object.h"
#include <string>

template <class ObjType>
auto object_pool<ObjType>::add(bool temp) -> object_ptr
{
	if(temp)
	{
		return tmp_object_list.add();
	}else{
		return object_list.add();
	}
}

template <class ObjType>
auto object_pool<ObjType>::add(ObjType &&obj, bool temp) -> object_ptr
{
	if(temp)
	{
		return tmp_object_list.add(std::move(obj));
	}else{
		return object_list.add(std::move(obj));
	}
}

template <class ObjType>
cell object_pool<ObjType>::get_address(AMX *amx, const_object_ptr obj) const
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(obj) - reinterpret_cast<cell>(data);
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
bool object_pool<ObjType>::move_to_global(const_object_ptr obj)
{
	auto it = tmp_object_list.find(obj);
	if(it != tmp_object_list.end())
	{
		std::unique_ptr<ObjType> ptr = tmp_object_list.extract(it);
		object_list.add(std::move(ptr));
		return true;
	}
	return false;
}

template <class ObjType>
bool object_pool<ObjType>::move_to_local(const_object_ptr obj)
{
	auto it = object_list.find(obj);
	if(it != object_list.end())
	{
		std::unique_ptr<ObjType> ptr = object_list.extract(it);
		tmp_object_list.add(std::move(ptr));
		return true;
	}
	return false;
}

template <class ObjType>
void object_pool<ObjType>::set_cache(const_object_ptr obj)
{
	inner_cache[&obj->operator[](0)] = obj;
}

template <class ObjType>
auto object_pool<ObjType>::find_cache(const_inner_ptr ptr) -> object_ptr
{
	auto it = inner_cache.find(ptr);
	if(it != inner_cache.end())
	{
		return const_cast<object_ptr>(it->second);
	}
	return nullptr;
}

template <class ObjType>
bool object_pool<ObjType>::remove(object_ptr obj)
{
	auto it = object_list.find(obj);
	if(it != object_list.end())
	{
		object_list.erase(it);
		return true;
	}
	it = tmp_object_list.find(obj);
	if(it != tmp_object_list.end())
	{
		tmp_object_list.erase(it);
		return true;
	}
	return false;
}

template <class ObjType>
auto object_pool<ObjType>::get(AMX *amx, cell addr) -> object_ptr
{
	object_ptr obj = reinterpret_cast<object_ptr>((amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + addr);
	
	auto it = tmp_object_list.find(obj);
	if(it != tmp_object_list.end())
	{
		return *it;
	}
	it = object_list.find(obj);
	if(it != object_list.end())
	{
		return *it;
	}
	return nullptr;
}

template <class ObjType>
void object_pool<ObjType>::clear_tmp()
{
	inner_cache.clear();
	tmp_object_list.clear();
}

template <class ObjType>
bool object_pool<ObjType>::contains(const_object_ptr obj) const
{
	return tmp_object_list.contains(obj) || object_list.contains(obj);
}

template <class ObjType>
size_t object_pool<ObjType>::local_size() const
{
	return tmp_object_list.size();
}

template <class ObjType>
size_t object_pool<ObjType>::global_size() const
{
	return object_list.size();
}

template class object_pool<std::basic_string<cell>>;
template class object_pool<dyn_object>;
