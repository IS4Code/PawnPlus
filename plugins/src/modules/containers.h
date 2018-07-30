#ifndef CONTAINERS_H_INCLUDED
#define CONTAINERS_H_INCLUDED

#include "objects/object_pool.h"
#include "objects/dyn_object.h"
#include "utils/shared_id_set_pool.h"
#include "fixes/linux.h"

#include "sdk/amx/amx.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <exception>
#include <typeinfo>

template <class Type>
class collection_base
{
protected:
	Type data;
	int revision = 0;
public:
	typedef typename Type::iterator iterator;
	typedef typename Type::const_iterator const_iterator;
	typedef typename Type::reference reference;
	typedef typename Type::const_reference const_reference;
	typedef typename Type::value_type value_type;

	iterator begin()
	{
		return data.begin();
	}

	iterator end()
	{
		return data.end();
	}

	const_iterator cbegin() const
	{
		return data.cbegin();
	}

	const_iterator cend() const
	{
		return data.cend();
	}

	size_t size() const
	{
		return data.size();
	}

	void clear()
	{
		data.clear();
		++revision;
	}

	iterator erase(const_iterator position)
	{
		auto it = data.erase(position);
		++revision;
		return it;
	}

	iterator erase(const_iterator first, const_iterator last)
	{
		auto it = data.erase(first, last);
		++revision;
		return it;
	}

	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		data.insert(first, last);
		++revision;
	}

	int get_revision() const
	{
		return revision;
	}
};

class list_t : public collection_base<std::vector<dyn_object>>
{
public:
	typedef typename std::vector<dyn_object>::reverse_iterator reverse_iterator;
	reverse_iterator rbegin()
	{
		return data.rbegin();
	}
	reverse_iterator rend()
	{
		return data.rend();
	}

	dyn_object &operator[](size_t index)
	{
		return data[index];
	}

	void push_back(dyn_object &&value)
	{
		data.push_back(std::move(value));
		++revision;
	}

	void push_back(const dyn_object &value)
	{
		data.push_back(value);
		++revision;
	}

	template <class InputIterator>
	iterator insert(const_iterator position, InputIterator first, InputIterator last)
	{
		auto it = data.insert(position, first, last);
		++revision;
		return it;
	}

	iterator insert(const_iterator position, dyn_object &&value)
	{
		auto it = data.insert(position, std::move(value));
		++revision;
		return it;
	}

	iterator insert(const_iterator position, const dyn_object &value)
	{
		auto it = data.insert(position, value);
		++revision;
		return it;
	}

	bool insert_dyn(const_iterator position, const std::type_info &type, void *value, iterator &result)
	{
		if(type == typeid(dyn_object))
		{
			result = insert(position, std::move(*reinterpret_cast<dyn_object*>(value)));
			return true;
		}
		return false;
	}

	bool insert_dyn(const_iterator position, const std::type_info &type, const void *value, iterator &result)
	{
		if(type == typeid(dyn_object))
		{
			result = insert(position, *reinterpret_cast<const dyn_object*>(value));
			return true;
		}
		return false;
	}
};

class map_t : public collection_base<std::unordered_map<dyn_object, dyn_object>>
{
public:
	dyn_object &operator[](const dyn_object &key)
	{
		return data[key];
	}

	std::pair<iterator, bool> insert(std::pair<dyn_object, dyn_object> &&value)
	{
		auto pair = data.insert(std::move(value));
		if(pair.second)
		{
			++revision;
		}
		return pair;
	}

	template <class InputIterator>
	void insert(InputIterator first, InputIterator last)
	{
		data.insert(first, last);
		++revision;
	}

	iterator find(const dyn_object &key)
	{
		return data.find(key);
	}

	size_t erase(const dyn_object &key)
	{
		size_t size = data.erase(key);
		++revision;
		return size;
	}
	iterator erase(const_iterator position)
	{
		return collection_base<std::unordered_map<dyn_object, dyn_object>>::erase(position);
	}

	bool insert_dyn(const_iterator position, const std::type_info &type, void *value, iterator &result)
	{
		return false;
	}

	bool insert_dyn(const_iterator position, const std::type_info &type, const void *value, iterator &result)
	{
		return false;
	}
};

class dyn_iterator
{
public:
	virtual bool expired() const
	{
		return true;
	}

	virtual bool valid() const
	{
		return false;
	}

	virtual bool move_next()
	{
		return false;
	}

	virtual bool move_previous()
	{
		return false;
	}

	virtual bool set_to_first()
	{
		return false;
	}

	virtual bool set_to_last()
	{
		return false;
	}

	virtual bool reset()
	{
		return false;
	}

	virtual bool erase()
	{
		return false;
	}

	virtual std::unique_ptr<dyn_iterator> clone() const
	{
		return nullptr;
	}

	template <class Type>
	bool extract(Type *&value) const
	{
		return extract_dyn(typeid(Type), reinterpret_cast<void*&>(value));
	}

	virtual bool operator==(const dyn_iterator &obj) const
	{
		return false;
	}

	template <class Type>
	bool insert(Type &&value)
	{
		return insert_dyn(typeid(Type), &value);
	}

	template <class Type>
	bool insert(const Type &value)
	{
		return insert_dyn(typeid(Type), &value);
	}

	int &operator[](size_t index) const
	{
		static int unused;
		return unused;
	}

	virtual ~dyn_iterator() = default;

protected:
	virtual bool extract_dyn(const std::type_info &type, void *&value) const
	{
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, void *value)
	{
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, const void *value)
	{
		return false;
	}
};

template <class Base>
class iterator_impl : public dyn_iterator
{
protected:
	typedef typename Base::iterator iterator;
	typedef typename Base::value_type value_type;
	std::weak_ptr<Base> _source;
	int _revision;
	iterator _position;
	bool _inside;

	std::shared_ptr<Base> lock_same()
	{
		if(auto source = _source.lock())
		{
			if(source->get_revision() == _revision)
			{
				return source;
			}else if(!_inside)
			{
				_position = source->end();
				_revision = source->get_revision();
				return source;
			}
		}
		return nullptr;
	}

	std::shared_ptr<Base> lock_same() const
	{
		if(auto source = _source.lock())
		{
			if(source->get_revision() == _revision)
			{
				return source;
			}
		}
		return nullptr;
	}

public:
	iterator_impl()
	{

	}

	iterator_impl(const std::shared_ptr<Base> source) : iterator_impl(source, source->begin())
	{

	}

	iterator_impl(const std::shared_ptr<Base> source, iterator position) : _source(source), _revision(source->get_revision()), _position(position)
	{
		_inside = position != source->end();
	}

	iterator_impl(const iterator_impl<Base> &iter) : _source(iter._source), _revision(iter._revision), _position(iter._position), _inside(iter._inside)
	{
		
	}

	virtual bool expired() const override
	{
		return _source.expired();
	}

	virtual bool valid() const override
	{
		if(auto source = lock_same())
		{
			return _position != source->end();
		}
		return false;
	}

	virtual bool move_next() override
	{
		if(auto source = lock_same())
		{
			if(_position == source->end())
			{
				return false;
			}else{
				++_position;
			}
			if(_position != source->end())
			{
				return true;
			}
			_inside = false;
		}
		return false;
	}

	virtual bool move_previous() override
	{
		if(auto source = lock_same())
		{
			if(_position == source->end())
			{
				return false;
			}else if(_position == source->begin())
			{
				_position = source->end();
				_inside = false;
				return false;
			}
			--_position;
			return true;
		}
		return false;
	}

	virtual bool set_to_first() override
	{
		if(auto source = _source.lock())
		{
			_revision = source->get_revision();
			_position = source->begin();
			if(_position != source->end())
			{
				_inside = true;
				return true;
			}
		}
		return false;
	}

	virtual bool set_to_last() override
	{
		if(auto source = _source.lock())
		{
			_revision = source->get_revision();
			_position = source->end();
			if(_position != source->begin())
			{
				--_position;
				_inside = true;
				return true;
			}
		}
		return false;
	}

	virtual bool reset() override
	{
		if(auto source = _source.lock())
		{
			_revision = source->get_revision();
			_position = source->end();
			_inside = false;
			return true;
		}
		return false;
	}

	virtual bool erase() override
	{
		if(auto source = lock_same())
		{
			if(_position != source->end())
			{
				_position = source->erase(_position);
				_revision = source->get_revision();
			}
			return true;
		}
		return false;
	}

	virtual std::unique_ptr<dyn_iterator> clone() const
	{
		return std::make_unique<iterator_impl<Base>>();
	}

	virtual bool operator==(const dyn_iterator &obj) const
	{
		auto other = dynamic_cast<const iterator_impl<Base>*>(&obj);
		if(other != nullptr)
		{
			return !_source.owner_before(other->_source) && !other->_source.owner_before(_source) && _revision == other->_revision && _position == other->_position && _inside == other->_inside;
		}
		return false;
	}

protected:
	virtual bool extract_dyn(const std::type_info &type, void *&value) const
	{
		if(type == typeid(value_type) && valid())
		{
			value = &*_position;
			return true;
		}
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, void *value) override
	{
		if(auto source = lock_same())
		{
			if(source->insert_dyn(_position, type, value, _position))
			{
				_revision = source->get_revision();
				return true;
			}
		}
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, const void *value) override
	{
		if(auto source = lock_same())
		{
			if(source->insert_dyn(_position, type, value, _position))
			{
				_revision = source->get_revision();
				return true;
			}
		}
		return false;
	}
};

typedef iterator_impl<list_t> list_iterator_t;
typedef iterator_impl<map_t> map_iterator_t;

extern aux::shared_id_set_pool<list_t> list_pool;
extern aux::shared_id_set_pool<map_t> map_pool;
extern object_pool<dyn_iterator> iter_pool;

#endif
