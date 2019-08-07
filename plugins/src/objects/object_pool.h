#ifndef OBJECT_POOL_H_INCLUDED
#define OBJECT_POOL_H_INCLUDED

#include "main.h"
#include "utils/shared_id_set_pool.h"
#include "sdk/amx/amx.h"
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <memory>

template <class ObjType>
class object_pool
{
public:
	class ref_container_simple
	{
		ObjType object;
		unsigned int ref_count = 0;

	public:
		ref_container_simple()
		{

		}

		ref_container_simple(ObjType &&object) : object(std::move(object))
		{

		}

		template <class... Args>
		explicit ref_container_simple(Args &&... args) : object(std::forward<Args>(args)...)
		{

		}

		ref_container_simple(std::nullptr_t) : ref_count(-1)
		{

		}

		ref_container_simple(const ref_container_simple&) = delete;

		ref_container_simple(ref_container_simple &&obj) : object(std::move(obj.object)), ref_count(obj.ref_count)
		{
			obj.ref_count = -1;
		}

		ObjType *operator->()
		{
			return &object;
		}

		const ObjType *operator->() const
		{
			return &object;
		}

		ObjType &operator*()
		{
			return object;
		}

		const ObjType &operator*() const
		{
			return object;
		}

		operator ObjType*()
		{
			return &object;
		}

		operator const ObjType*() const
		{
			return &object;
		}

		operator bool() const
		{
			return ref_count != -1;
		}

		bool operator==(std::nullptr_t) const
		{
			return ref_count == -1;
		}

		bool operator!=(std::nullptr_t) const
		{
			return ref_count != -1;
		}

		ref_container_simple &operator=(const ref_container_simple&) = delete;

		ref_container_simple &operator=(ref_container_simple &&obj)
		{
			object = std::move(obj.object);
			ref_count = obj.ref_count;
			obj.ref_count = -1;
			return *this;
		}

		bool acquire()
		{
			if(ref_count == -1) return false;
			ref_count++;
			return true;
		}

		bool release()
		{
			if(ref_count == -1 || ref_count == 0) return false;
			ref_count--;
			return true;
		}

		bool local() const
		{
			return ref_count == 0;
		}
	};

	class ref_container_virtual
	{
		unsigned int ref_count = 0;

	public:
		ref_container_virtual()
		{

		}

		ref_container_virtual(std::nullptr_t) : ref_count(-1)
		{

		}

		ref_container_virtual(const ref_container_virtual&)
		{

		}

		virtual ObjType *get() = 0;
		virtual const ObjType *get() const = 0;

		ObjType *operator->()
		{
			return get();
		}

		const ObjType *operator->() const
		{
			return get();
		}

		ObjType &operator*()
		{
			return *get();
		}

		const ObjType &operator*() const
		{
			return *get();
		}

		operator ObjType*()
		{
			return get();
		}

		operator const ObjType*() const
		{
			return get();
		}

		operator bool() const
		{
			return ref_count != -1;
		}

		bool operator==(std::nullptr_t) const
		{
			return ref_count == -1;
		}

		bool operator!=(std::nullptr_t) const
		{
			return ref_count != -1;
		}

		ref_container_virtual &operator=(const ref_container_virtual&)
		{
			return *this;
		}

		virtual ~ref_container_virtual() = default;

		bool acquire()
		{
			if(ref_count == -1) return false;
			ref_count++;
			return true;
		}

		bool release()
		{
			if(ref_count == -1 || ref_count == 0) return false;
			ref_count--;
			return true;
		}

		bool local() const
		{
			return ref_count == 0;
		}
	};

	typedef typename std::conditional<std::has_virtual_destructor<ObjType>::value, ref_container_virtual, ref_container_simple>::type ref_container;

	typedef ref_container &object_ptr;
	typedef const ref_container &const_object_ptr;
	typedef decltype(&static_cast<ObjType*>(nullptr)->operator[](0)) inner_ptr;
	typedef decltype(&static_cast<const ObjType*>(nullptr)->operator[](0)) const_inner_ptr;

	typedef aux::shared_id_set_pool<ref_container> list_type;

private:
	list_type global_object_list;
	list_type local_object_list;
	std::unordered_map<const_inner_ptr, const ref_container*> inner_cache;

public:
	object_ptr add()
	{
		return *local_object_list.add();
	}

	object_ptr add(ObjType &&obj)
	{
		return *local_object_list.emplace(std::move(obj));
	}

	object_ptr add(ref_container &&obj)
	{
		return *local_object_list.add(std::move(obj));
	}

	object_ptr add(std::shared_ptr<ref_container> &&obj)
	{
		return *local_object_list.add(std::move(obj));
	}

	/*object_ptr add(std::unique_ptr<ref_container> &&obj)
	{
		return *local_object_list.add(std::move(obj));
	}*/

	template <class... Args>
	object_ptr emplace(Args &&...args)
	{
		return *local_object_list.emplace(std::forward<Args>(args)...);
	}

	template <class Type, class... Args>
	object_ptr emplace_derived(Args &&...args)
	{
		return *local_object_list.template emplace_derived<Type>(std::forward<Args>(args)...);
	}

	cell get_address(AMX *amx, const_object_ptr obj) const
	{
		unsigned char *data = amx_GetData(amx);
		return reinterpret_cast<cell>(&obj) - reinterpret_cast<cell>(data);
	}

	cell get_inner_address(AMX *amx, const_object_ptr obj) const
	{
		unsigned char *data = amx_GetData(amx);
		return reinterpret_cast<cell>(&obj->operator[](0)) - reinterpret_cast<cell>(data);
	}

	cell get_null_address(AMX *amx) const
	{
		unsigned char *data = amx_GetData(amx);
		return -reinterpret_cast<cell>(data);
	}

	bool is_null_address(AMX *amx, cell addr) const
	{
		if(addr >= 0 && addr < amx->stp)
		{
			return false;
		}
		unsigned char *data = amx_GetData(amx);
		return reinterpret_cast<cell>(data) == -addr;
	}

	bool acquire_ref(object_ptr obj)
	{
		bool local = obj.local();
		if(obj.acquire())
		{
			if(local)
			{
				auto it = local_object_list.find(&obj);
				if(it != local_object_list.end())
				{
					auto ptr = local_object_list.extract(it);
					global_object_list.add(std::move(ptr));
				}
			}
			return true;
		}
		return false;
	}

	bool release_ref(object_ptr obj)
	{
		if(obj.release())
		{
			if(obj.local())
			{
				auto it = global_object_list.find(&obj);
				if(it != global_object_list.end())
				{
					auto ptr = global_object_list.extract(it);
					local_object_list.add(std::move(ptr));
				}
			}
			return true;
		}
		return false;
	}

	void set_cache(const_object_ptr obj)
	{
		inner_cache[&obj->operator[](0)] = &obj;
	}

	bool find_cache(const_inner_ptr ptr, const ref_container *&obj)
	{
		auto it = inner_cache.find(ptr);
		if(it != inner_cache.end())
		{
			obj = it->second;
			return true;
		}
		return false;
	}

	bool remove(object_ptr obj)
	{
		auto it = global_object_list.find(&obj);
		if(it != global_object_list.end())
		{
			global_object_list.erase(it);
			return true;
		}
		it = local_object_list.find(&obj);
		if(it != local_object_list.end())
		{
			local_object_list.erase(it);
			return true;
		}
		return false;
	}

	bool remove_by_id(cell id)
	{
		auto obj = reinterpret_cast<ref_container*>(id);
		auto it = global_object_list.find(obj);
		if(it != global_object_list.end())
		{
			global_object_list.erase(it);
			return true;
		}
		it = local_object_list.find(obj);
		if(it != local_object_list.end())
		{
			local_object_list.erase(it);
			return true;
		}
		return false;
	}

	void clear()
	{
		inner_cache.clear();
		auto tmp = std::move(local_object_list);
		tmp.clear();
		auto list = std::move(global_object_list);
		list.clear();
	}

	void clear_tmp()
	{
		inner_cache.clear();
		auto tmp = std::move(local_object_list);
		tmp.clear();
	}

	bool get_by_id(cell id, ref_container *&obj)
	{
		obj = reinterpret_cast<ref_container*>(id);

		if(local_object_list.find(obj) != local_object_list.end())
		{
			return true;
		}
		if(global_object_list.find(obj) != global_object_list.end())
		{
			return true;
		}
		return false;
	}

	bool get_by_id(cell id, ObjType *&obj)
	{
		auto ptr = reinterpret_cast<ref_container*>(id);

		if(local_object_list.find(ptr) != local_object_list.end())
		{
			obj = *ptr;
			return true;
		}
		if(global_object_list.find(ptr) != global_object_list.end())
		{
			obj = *ptr;
			return true;
		}
		obj = reinterpret_cast<ObjType*>(id);
		return false;
	}

	bool get_by_id(cell id, std::shared_ptr<ref_container> &obj)
	{
		if(local_object_list.get_by_id(id, obj))
		{
			return true;
		}
		if(global_object_list.get_by_id(id, obj))
		{
			return true;
		}
		return false;
	}

	bool get_by_id(cell id, std::shared_ptr<ObjType> &obj)
	{
		std::shared_ptr<ref_container> ptr;
		if(get_by_id(id, ptr))
		{
			obj = std::shared_ptr<ObjType>(ptr, *ptr);
			return true;
		}
		return false;
	}

	bool get_by_id(cell id, std::shared_ptr<const ObjType> &obj)
	{
		std::shared_ptr<ref_container> ptr;
		if(get_by_id(id, ptr))
		{
			obj = std::shared_ptr<const ObjType>(ptr, *ptr);
			return true;
		}
		return false;
	}

	cell get_id(const_object_ptr obj) const
	{
		return reinterpret_cast<cell>(&obj);
	}

	std::shared_ptr<ref_container> get(object_ptr obj)
	{
		auto it = local_object_list.find(&obj);
		if(it != local_object_list.end())
		{
			return it->second;
		}
		it = global_object_list.find(&obj);
		if(it != global_object_list.end())
		{
			return it->second;
		}
		return {};
	}

	bool get_by_addr(AMX *amx, cell addr, ref_container *&obj)
	{
		obj = reinterpret_cast<ref_container*>(amx_GetData(amx) + addr);

		auto it = local_object_list.find(obj);
		if(it != local_object_list.end())
		{
			return true;
		}
		it = global_object_list.find(obj);
		if(it != global_object_list.end())
		{
			return true;
		}
		return false;
	}

	size_t local_size() const
	{
		return local_object_list.size();
	}

	size_t global_size() const
	{
		return global_object_list.size();
	}
};

#endif
