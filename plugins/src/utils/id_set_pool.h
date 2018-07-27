#ifndef ID_SET_POOL_H_INCLUDED
#define ID_SET_POOL_H_INCLUDED

#include "fixes/linux.h"
#include "sdk/amx/amx.h"
#include <memory>
#include <unordered_set>
#include <exception>

namespace aux
{
	template <class Type>
	class id_set_pool
	{
		std::unordered_set<Type*> data;

		Type *add(Type *value)
		{
			data.insert(value);
			return value;
		}

		typedef typename std::unordered_set<Type*>::iterator iterator;
		typedef typename std::unordered_set<Type*>::const_iterator const_iterator;

	public:
		Type *add(std::unique_ptr<Type> &&value)
		{
			return add(value.release());
		}

		Type *add()
		{
			return add(new Type());
		}

		Type *add(Type&& value)
		{
			return add(new Type(std::move(value)));
		}

		size_t size() const
		{
			return data.size();
		}

		void clear()
		{
			for(auto ptr : data)
			{
				delete ptr;
			}
			data.clear();
		}

		iterator begin()
		{
			return data.begin();
		}

		iterator end()
		{
			return data.end();
		}

		bool remove(Type *value)
		{
			auto it = data.find(value);
			if(it != data.end())
			{
				delete value;
				data.erase(it);
				return true;
			}
			return false;
		}

		bool get_by_id(cell id, Type *&value)
		{
			value = reinterpret_cast<Type*>(id);
			if(data.find(value) != data.end())
			{
				return true;
			}
			return false;
		}

		cell get_id(const Type *value) const
		{
			return reinterpret_cast<cell>(value);
		}

		bool contains(const Type *value) const
		{
			return data.find(const_cast<Type*>(value)) != data.cend();
		}

		iterator find(const Type *value)
		{
			return data.find(const_cast<Type*>(value));
		}

		const_iterator find(const Type *value) const
		{
			return data.find(const_cast<Type*>(value));
		}

		iterator erase(iterator it)
		{
			auto ptr = *it;
			delete ptr;
			return data.erase(it);
		}

		std::unique_ptr<Type> extract(iterator it)
		{
			Type *ptr = *it;
			data.erase(it);
			return std::unique_ptr<Type>(ptr);
		}

		id_set_pool()
		{

		}

		id_set_pool(id_set_pool<Type> &&obj) : data(std::move(obj.data))
		{
			obj.data.clear();
		}

		id_set_pool<Type> &operator=(id_set_pool<Type> &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
				obj.data.clear();
			}
			return *this;
		}

		~id_set_pool()
		{
			clear();
		}
	};
}

#endif
