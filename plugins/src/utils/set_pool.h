#ifndef SET_POOL_H_INCLUDED
#define SET_POOL_H_INCLUDED

#include "fixes/linux.h"
#include <memory>
#include <unordered_set>
#include <exception>

namespace aux
{
	template <class Type>
	class set_pool
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

		void erase(iterator it)
		{
			data.erase(it);
		}

		std::unique_ptr<Type> extract(iterator it)
		{
			Type *ptr = *it;
			data.erase(it);
			return std::unique_ptr<Type>(ptr);
		}

		set_pool()
		{

		}

		set_pool(set_pool<Type> &&obj) : data(std::move(obj.data))
		{
			obj.data.clear();
		}

		set_pool<Type> &operator=(set_pool<Type> &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
				obj.data.clear();
			}
			return *this;
		}

		~set_pool()
		{
			clear();
		}
	};
}

#endif
