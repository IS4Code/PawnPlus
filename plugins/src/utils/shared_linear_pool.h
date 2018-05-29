#ifndef SHARED_LINEAR_POOL_H_INCLUDED
#define SHARED_LINEAR_POOL_H_INCLUDED

#include "fixes/linux.h"
#include <memory>
#include <vector>
#include <exception>

namespace aux
{
	template <class Type>
	class shared_linear_pool
	{
		std::vector<std::shared_ptr<Type>> data;

		size_t add(Type *value)
		{
			for(size_t i = 0; i < data.size(); i++)
			{
				if(data[i] == nullptr)
				{
					data[i] = std::shared_ptr<Type>(value);
					return i;
				}
			}
			size_t index = data.size();
			data.push_back(std::shared_ptr<Type>(value));
			return index;
		}

	public:
		size_t add()
		{
			return add(new Type());
		}

		size_t add(Type&& value)
		{
			return add(new Type(std::move(value)));
		}

		size_t size() const
		{
			return data.size();
		}

		bool remove(size_t index)
		{
			if(index >= data.size() || data[index] == nullptr) return false;
			if(index == data.size() - 1)
			{
				data.erase(data.begin() + index);
			}else{
				data[index] = nullptr;
			}
			return true;
		}

		std::shared_ptr<Type> at(size_t index)
		{
			if(index >= data.size() || data[index] == nullptr) return nullptr;
			return data[index];
		}

		void clear()
		{
			data.clear();
		}

		shared_linear_pool()
		{

		}

		shared_linear_pool(shared_linear_pool<Type> &&obj) : data(std::move(obj.data))
		{
			obj.data.clear();
		}

		shared_linear_pool<Type> &operator=(shared_linear_pool<Type> &&obj)
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
