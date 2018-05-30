#ifndef SHARED_SET_POOL_H_INCLUDED
#define SHARED_SET_POOL_H_INCLUDED

#include "fixes/linux.h"
#include <memory>
#include <unordered_map>

namespace aux
{
	template <class Type>
	class shared_set_pool
	{
		std::unordered_map<Type*, std::shared_ptr<Type>> data;

		std::shared_ptr<Type> add(Type *value)
		{
			return data[value] = std::shared_ptr<Type>(value);
		}

	public:
		std::shared_ptr<Type> add()
		{
			return add(new Type());
		}

		std::shared_ptr<Type> add(Type&& value)
		{
			return add(new Type(std::move(value)));
		}

		size_t size() const
		{
			return data.size();
		}

		bool remove(Type *value)
		{
			auto it = data.find(value);
			if(it != data.end())
			{
				data.erase(it);
				return true;
			}
			return false;
		}

		void clear()
		{
			data.clear();
		}

		bool contains(const Type *value) const
		{
			return data.find(const_cast<Type*>(value)) != data.cend();
		}

		std::shared_ptr<Type> get(Type *value)
		{
			auto it = data.find(value);
			if(it != data.end())
			{
				return it->second;
			}
			return {};
		}

		shared_set_pool()
		{

		}

		shared_set_pool(shared_set_pool<Type> &&obj) : data(std::move(obj.data))
		{
			obj.data.clear();
		}

		shared_set_pool<Type> &operator=(shared_set_pool<Type> &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
				obj.data.clear();
			}
			return *this;
		}
	};
}

#endif
