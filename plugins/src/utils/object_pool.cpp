#include "object_pool.h"
#include <string>
#include <algorithm>

/*template <class ObjType>
auto object_pool<ObjType>::add(object_ptr obj, bool temp) -> object_ptr
{
	if(temp)
	{
		tmp_object_list.emplace_back(obj);
	} else {
		object_list.emplace_back(obj);
	}
	return obj;
}*/

template <class ObjType>
auto object_pool<ObjType>::add(bool temp) -> object_ptr
{
	if(temp)
	{
		tmp_object_list.emplace_back(new ObjType());
		return tmp_object_list.back().get();
	} else {
		object_list.emplace_back(new ObjType());
		return object_list.back().get();
	}
}

template <class ObjType>
auto object_pool<ObjType>::add(ObjType &&obj, bool temp) -> object_ptr
{
	if(temp)
	{
		tmp_object_list.emplace_back(new ObjType(std::move(obj)));
		return tmp_object_list.back().get();
	} else {
		object_list.emplace_back(new ObjType(std::move(obj)));
		return object_list.back().get();
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
	auto it = find_in_list(obj, tmp_object_list);
	if(it != tmp_object_list.end())
	{
		auto ptr = std::move(*it);
		tmp_object_list.erase(it);
		object_list.emplace_back(std::move(ptr));
		return true;
	}
	return false;
}

template <class ObjType>
bool object_pool<ObjType>::move_to_local(const_object_ptr obj)
{
	auto it = find_in_list(obj, object_list);
	if(it != object_list.end())
	{
		auto ptr = std::move(*it);
		object_list.erase(it);
		tmp_object_list.emplace_back(std::move(ptr));
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
bool object_pool<ObjType>::free(object_ptr obj)
{
	auto it = find_in_list(obj, object_list);
	if(it != object_list.end())
	{
		object_list.erase(it);
		return true;
	}
	it = find_in_list(obj, tmp_object_list);
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
	
	auto it = find_in_list(obj, tmp_object_list);
	if(it != tmp_object_list.end())
	{
		return it->get();
	}
	it = find_in_list(obj, object_list);
	if(it != object_list.end())
	{
		return it->get();
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
bool object_pool<ObjType>::is_valid(const_object_ptr obj) const
{
	return find_in_list(obj, tmp_object_list) != tmp_object_list.end() || find_in_list(obj, object_list) != object_list.end();
}

template <class ObjType>
auto object_pool<ObjType>::find_in_list(const_object_ptr obj, list_type &list) const -> typename list_type::iterator
{
	return std::find_if(list.begin(), list.end(), [=](std::unique_ptr<ObjType> &ptr) { return ptr.get() == obj; });
}

template <class ObjType>
auto object_pool<ObjType>::find_in_list(const_object_ptr obj, const list_type &list) const -> typename list_type::const_iterator
{
	return std::find_if(list.begin(), list.end(), [=](const std::unique_ptr<ObjType> &ptr) { return ptr.get() == obj; });
}

template class object_pool<std::basic_string<cell>>;
