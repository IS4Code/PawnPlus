#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

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

		bool remove(size_t index)
		{
			if(index >= data.size() || data[index] == nullptr) return false;
			data[index] = nullptr;
			return true;
		}

		Type *get(size_t index)
		{
			if(index >= data.size() || data[index] == nullptr) return nullptr;
			return data[index].get();
		}
	};
}

#endif
