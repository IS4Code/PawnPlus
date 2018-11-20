#ifndef SHARED_ID_SET_POOL_H_INCLUDED
#define SHARED_ID_SET_POOL_H_INCLUDED

#include "fixes/linux.h"
#include "sdk/amx/amx.h"
#include <memory>
#include <unordered_map>

namespace aux
{
	template <class Type>
	class shared_id_set_pool
	{
		std::unordered_map<Type*, std::shared_ptr<Type>> data;

		std::shared_ptr<Type> add(std::shared_ptr<Type> &&value)
		{
			auto ptr = value.get();
			return data[ptr] = std::move(value);
		}

	public:
		std::shared_ptr<Type> add()
		{
			return add(std::make_shared<Type>());
		}

		std::shared_ptr<Type> add(Type&& value)
		{
			return add(std::make_shared<Type>(std::move(value)));
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
				std::shared_ptr<Type> orig(std::move(it->second));
				data.erase(it);
				return true;
			}
			return false;
		}

		void clear()
		{
			data.clear();
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

		bool get_by_id(cell id, std::shared_ptr<Type> &value)
		{
			auto it = data.find(reinterpret_cast<Type*>(id));
			if(it != data.end())
			{
				value = it->second;
				return true;
			}
			return false;
		}

		cell get_id(const Type *value) const
		{
			return reinterpret_cast<cell>(value);
		}

		cell get_id(const std::shared_ptr<Type> &value) const
		{
			return reinterpret_cast<cell>(value.get());
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

		shared_id_set_pool()
		{

		}

		shared_id_set_pool(shared_id_set_pool<Type> &&obj) : data(std::move(obj.data))
		{
			obj.data.clear();
		}

		shared_id_set_pool<Type> &operator=(shared_id_set_pool<Type> &&obj)
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
