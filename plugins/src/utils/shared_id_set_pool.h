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

		typedef typename std::unordered_map<Type*, std::shared_ptr<Type>>::iterator iterator;
		typedef typename std::unordered_map<Type*, std::shared_ptr<Type>>::const_iterator const_iterator;

	public:
		const std::shared_ptr<Type> &add(std::shared_ptr<Type> &&value)
		{
			auto ptr = value.get();
			return data.emplace(ptr, std::move(value)).first->second;
		}

		const std::shared_ptr<Type> &add()
		{
			return add(std::make_shared<Type>());
		}

		const std::shared_ptr<Type> &add(Type&& value)
		{
			return add(std::make_shared<Type>(std::move(value)));
		}

		/*const std::shared_ptr<Type> &add(std::unique_ptr<Type> &&value)
		{
			return add(std::shared_ptr<Type>(value.release()));
		}*/

		template <class... Args>
		const std::shared_ptr<Type> &emplace(Args &&... args)
		{
			return add(std::make_shared<Type>(std::forward<Args>(args)...));
		}

		template <class NewType, class... Args>
		const std::shared_ptr<Type> &emplace_derived(Args &&... args)
		{
			return add(std::make_shared<NewType>(std::forward<Args>(args)...));
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

		iterator begin()
		{
			return data.begin();
		}

		iterator end()
		{
			return data.end();
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
			return data.erase(it);
		}

		std::shared_ptr<Type> extract(iterator it)
		{
			auto ptr = std::move(it->second);
			data.erase(it);
			return ptr;
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
