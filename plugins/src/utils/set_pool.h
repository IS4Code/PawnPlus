#ifndef SET_POOL_H_INCLUDED
#define SET_POOL_H_INCLUDED

#include "fixes/linux.h"
#include <memory>
#include <unordered_set>
#include <exception>

namespace aux
{
	namespace impl
	{
		template <class Type>
		struct conditional_delete
		{
			bool del;
			conditional_delete(bool del) : del(del)
			{

			}

			void operator()(Type *p) const
			{
				if(del) delete p;
			}
		};
	}

	template <class Type>
	class set_pool
	{
		typedef std::unique_ptr<Type, impl::conditional_delete<Type>> unique_ptr;

		std::unordered_set<unique_ptr> data;

		Type *add(Type *value)
		{
			data.insert(unique_ptr(value, true));
			return value;
		}

	public:
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
			data.clear();
		}

		auto begin()
		{
			return data.begin();
		}

		auto end()
		{
			return data.end();
		}

		bool remove(Type *value)
		{
			auto it = data.find(unique_ptr(value, false));
			if(it != data.end())
			{
				data.erase(it);
				return true;
			}
			return false;
		}

		bool contains(Type *value)
		{
			return data.find(unique_ptr(value, false)) != data.end();
		}

		set_pool()
		{

		}

		/*set_pool(const set_pool<Type> &obj)
		{
			for(auto &ptr : obj.data)
			{
				data.insert(std::make_unique<Type>(*ptr));
			}
		}*/

		/*set_pool<Type> &operator=(const set_pool<Type> &obj)
		{
			if(this != &obj)
			{
				data.clear();
				for(auto &ptr : obj.data)
				{
					add(Type(*ptr));
				}
			}
			return *this;
		}*/

		set_pool(set_pool<Type> &&obj) : data(std::move(obj.data))
		{

		}

		set_pool<Type> &operator=(set_pool<Type> &&obj)
		{
			if(this != &obj)
			{
				data = std::move(obj.data);
			}
			return *this;
		}
	};
}

#endif
