#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

#include "fixes/linux.h"
#include <memory>
#include <vector>
#include <exception>

namespace aux
{
	template <class Type>
	class pool
	{
		std::vector<std::unique_ptr<Type>> data;

		size_t add(Type *value)
		{
			for(size_t i = 0; i < data.size(); i++)
			{
				if(data[i] == nullptr)
				{
					data[i] = std::unique_ptr<Type>(value);
					return i;
				}
			}
			size_t index = data.size();
			data.push_back(std::unique_ptr<Type>(value));
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

		Type *get(size_t index)
		{
			if(index >= data.size() || data[index] == nullptr) return nullptr;
			return data[index].get();
		}

		void clear()
		{
			data.clear();
		}

		pool()
		{

		}

		pool(const pool<Type> &obj)
		{
			for(auto &ptr : obj.data)
			{
				if(ptr == nullptr)
				{
					data.push_back(nullptr);
				}else{
					data.push_back(std::make_unique<Type>(*ptr));
				}
			}
		}

		pool<Type> &operator=(const pool<Type> &obj)
		{
			if(this != &obj)
			{
				data.clear();
				for(auto &ptr : obj.data)
				{
					if(ptr == nullptr)
					{
						data.push_back(nullptr);
					}else{
						data.push_back(std::make_unique<Type>(*ptr));
					}
				}
			}
			return *this;
		}
	};
}

#endif
