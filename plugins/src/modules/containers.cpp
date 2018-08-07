#include "containers.h"

aux::shared_id_set_pool<list_t> list_pool;
aux::shared_id_set_pool<map_t> map_pool;
object_pool<dyn_iterator> iter_pool;



auto list_t::rbegin() -> reverse_iterator
{
	return data.rbegin();
}
auto list_t::rend() -> reverse_iterator
{
	return data.rend();
}

dyn_object &list_t::operator[](size_t index)
{
	return data[index];
}

void list_t::push_back(dyn_object &&value)
{
	data.push_back(std::move(value));
	++revision;
}

void list_t::push_back(const dyn_object &value)
{
	data.push_back(value);
	++revision;
}

auto list_t::insert(iterator position, dyn_object &&value) -> iterator
{
	auto it = data.insert(position, std::move(value));
	++revision;
	return it;
}

auto list_t::insert(iterator position, const dyn_object &value) -> iterator
{
	auto it = data.insert(position, value);
	++revision;
	return it;
}

bool list_t::insert_dyn(iterator position, const std::type_info &type, void *value, iterator &result)
{
	if(type == typeid(dyn_object))
	{
		result = insert(position, std::move(*reinterpret_cast<dyn_object*>(value)));
		return true;
	}
	return false;
}

bool list_t::insert_dyn(iterator position, const std::type_info &type, const void *value, iterator &result)
{
	if(type == typeid(dyn_object))
	{
		result = insert(position, *reinterpret_cast<const dyn_object*>(value));
		return true;
	}
	return false;
}



dyn_object &map_t::operator[](const dyn_object &key)
{
	return data[key];
}

auto map_t::insert(std::pair<dyn_object, dyn_object> &&value) -> std::pair<iterator, bool>
{
	auto pair = data.insert(std::move(value));
	if(pair.second)
	{
		++revision;
	}
	return pair;
}

auto map_t::find(const dyn_object &key) -> iterator
{
	return data.find(key);
}

size_t map_t::erase(const dyn_object &key)
{
	size_t size = data.erase(key);
	++revision;
	return size;
}

auto map_t::erase(iterator position) -> iterator
{
	return collection_base<std::unordered_map<dyn_object, dyn_object>>::erase(position);
}

bool map_t::insert_dyn(iterator position, const std::type_info &type, void *value, iterator &result)
{
	return false;
}

bool map_t::insert_dyn(iterator position, const std::type_info &type, const void *value, iterator &result)
{
	return false;
}



bool dyn_iterator::expired() const
{
	return true;
}

bool dyn_iterator::valid() const
{
	return false;
}

bool dyn_iterator::move_next()
{
	return false;
}

bool dyn_iterator::move_previous()
{
	return false;
}

bool dyn_iterator::set_to_first()
{
	return false;
}

bool dyn_iterator::set_to_last()
{
	return false;
}

bool dyn_iterator::reset()
{
	return false;
}

bool dyn_iterator::erase()
{
	return false;
}

std::unique_ptr<dyn_iterator> dyn_iterator::clone() const
{
	return std::make_unique<dyn_iterator>();
}

size_t dyn_iterator::get_hash() const
{
	return 0;
}

bool dyn_iterator::operator==(const dyn_iterator &obj) const
{
	return false;
}

int &dyn_iterator::operator[](size_t index) const
{
	static int unused;
	return unused;
}

bool dyn_iterator::extract_dyn(const std::type_info &type, void *&value) const
{
	return false;
}

bool dyn_iterator::insert_dyn(const std::type_info &type, void *value)
{
	return false;
}

bool dyn_iterator::insert_dyn(const std::type_info &type, const void *value)
{
	return false;
}
