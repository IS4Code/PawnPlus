#include "containers.h"

aux::shared_id_set_pool<list_t> list_pool;
aux::shared_id_set_pool<map_t> map_pool;
aux::shared_id_set_pool<linked_list_t> linked_list_pool;
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



dyn_object &linked_list_t::operator[](size_t index)
{
	auto it = data.begin();
	std::advance(it, index);
	return **it;
}

void linked_list_t::push_back(dyn_object &&value)
{
	data.push_back(std::make_shared<dyn_object>(std::move(value)));
	++revision;
}

void linked_list_t::push_back(const dyn_object &value)
{
	data.push_back(std::make_shared<dyn_object>(value));
	++revision;
}

auto linked_list_t::insert(iterator position, dyn_object &&value) -> iterator
{
	auto it = data.insert(position, std::make_shared<dyn_object>(std::move(value)));
	++revision;
	return it;
}

auto linked_list_t::insert(iterator position, const dyn_object &value) -> iterator
{
	auto it = data.insert(position, std::make_shared<dyn_object>(value));
	++revision;
	return it;
}

bool linked_list_t::insert_dyn(iterator position, const std::type_info &type, void *value, iterator &result)
{
	if(type == typeid(dyn_object))
	{
		result = insert(position, std::move(*reinterpret_cast<dyn_object*>(value)));
		return true;
	}
	return false;
}

bool linked_list_t::insert_dyn(iterator position, const std::type_info &type, const void *value, iterator &result)
{
	if(type == typeid(dyn_object))
	{
		result = insert(position, *reinterpret_cast<const dyn_object*>(value));
		return true;
	}
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



std::shared_ptr<linked_list_t> linked_list_iterator_t::lock_same() const
{
	if(auto source = _source.lock())
	{
		if(!_current.expired())
		{
			return source;
		}
	}
	return nullptr;
}

bool linked_list_iterator_t::expired() const
{
	return _source.expired();
}

bool linked_list_iterator_t::valid() const
{
	if(auto source = lock_same())
	{
		return _position != source->end();
	}
	return false;
}

bool linked_list_iterator_t::move_next()
{
	if(auto source = lock_same())
	{
		++_position;
		if(_position != source->end())
		{
			_current = *_position;
			return true;
		}else{
			_current.reset();
		}
	}
	return false;
}

bool linked_list_iterator_t::move_previous()
{
	if(auto source = lock_same())
	{
		if(_position == source->begin())
		{
			_position = source->end();
			_current.reset();
			return false;
		}
		--_position;
		_current = *_position;
		return true;
	}
	return false;
}

bool linked_list_iterator_t::set_to_first()
{
	if(auto source = _source.lock())
	{
		_position = source->begin();
		if(_position != source->end())
		{
			_current = *_position;
			return true;
		}else{
			_current.reset();
		}
	}
	return false;
}

bool linked_list_iterator_t::set_to_last()
{
	if(auto source = _source.lock())
	{
		_position = source->end();
		if(_position != source->begin())
		{
			--_position;
			_current = *_position;
			return true;
		}else{
			_current.reset();
		}
	}
	return false;
}

bool linked_list_iterator_t::reset()
{
	if(auto source = _source.lock())
	{
		_position = source->end();
		_current.reset();
		return true;
	}
	return false;
}

size_t linked_list_iterator_t::get_hash() const
{
	if(auto source = _source.lock())
	{
		if(!_current.expired())
		{
			return std::hash<decltype(&*_position)>()(&*_position);
		}else{
			return std::hash<linked_list_t*>()(source.get());
		}
	}
	return 0;
}

bool linked_list_iterator_t::erase()
{
	if(auto source = lock_same())
	{
		_position = source->erase(_position);
		if(_position != source->end())
		{
			_current = *_position;
		}else{
			_current.reset();
		}
		return true;
	}
	return false;
}

std::unique_ptr<dyn_iterator> linked_list_iterator_t::clone() const
{
	return std::make_unique<linked_list_iterator_t>(*this);
}

bool linked_list_iterator_t::operator==(const dyn_iterator &obj) const
{
	auto other = dynamic_cast<const linked_list_iterator_t*>(&obj);
	if(other != nullptr)
	{
		return !_source.owner_before(other->_source) && !other->_source.owner_before(_source) && _position == other->_position && !_current.owner_before(other->_current) && !other->_current.owner_before(_current);
	}
	return false;
}

bool linked_list_iterator_t::extract_dyn(const std::type_info &type, void *&value) const
{
	if(valid())
	{
		if(type == typeid(dyn_object))
		{
			value = &**_position;
			return true;
		}else if(type == typeid(value_type))
		{
			value = &*_position;
			return true;
		}
	}
	return false;
}

bool linked_list_iterator_t::insert_dyn(const std::type_info &type, void *value)
{
	if(auto source = lock_same())
	{
		if(source->insert_dyn(_position, type, value, _position))
		{
			_current = *_position;
			return true;
		}
	}
	return false;
}

bool linked_list_iterator_t::insert_dyn(const std::type_info &type, const void *value)
{
	if(auto source = lock_same())
	{
		if(source->insert_dyn(_position, type, value, _position))
		{
			_current = *_position;
			return true;
		}
	}
	return false;
}
