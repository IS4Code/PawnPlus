#ifndef OBJECT_POOL_H_INCLUDED
#define OBJECT_POOL_H_INCLUDED

#include "utils/id_set_pool.h"
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

	class ref_container_virtual_null : public ref_container_virtual
	{
	public:
		ref_container_virtual_null(std::nullptr_t) : ref_container_virtual(nullptr)
		{

		}

		virtual ObjType *get() override
		{
			return nullptr;
		}

		virtual const ObjType *get() const override
		{
			return nullptr;
		}
	};

	typedef typename std::conditional<std::has_virtual_destructor<ObjType>::value, ref_container_virtual, ref_container_simple>::type ref_container;
	typedef typename std::conditional<std::has_virtual_destructor<ObjType>::value, ref_container_virtual_null, ref_container>::type ref_container_null;

	typedef ref_container &object_ptr;
	typedef const ref_container &const_object_ptr;
	typedef decltype(&static_cast<ObjType*>(nullptr)->operator[](0)) inner_ptr;
	typedef decltype(&static_cast<const ObjType*>(nullptr)->operator[](0)) const_inner_ptr;

	typedef aux::id_set_pool<ref_container> list_type;
	static ref_container_null null_ref;
private:

	list_type global_object_list;
	list_type local_object_list;
	std::unordered_map<const_inner_ptr, const ref_container*> inner_cache;
	
public:
	object_ptr add();
	object_ptr add(ref_container &&obj);
	object_ptr add(std::unique_ptr<ref_container> &&obj);
	cell get_address(AMX *amx, const_object_ptr obj) const;
	cell get_inner_address(AMX *amx, const_object_ptr obj) const;
	bool is_null_address(AMX *amx, cell addr) const;
	bool acquire_ref(object_ptr obj);
	bool release_ref(object_ptr obj);
	void set_cache(const_object_ptr obj);
	object_ptr find_cache(const_inner_ptr ptr);
	bool remove(object_ptr obj);
	bool remove_by_id(cell id);
	void clear();
	void clear_tmp();
	bool get_by_id(cell id, ref_container *&obj);
	bool get_by_id(cell id, ObjType *&obj);
	cell get_id(const_object_ptr obj) const;
	object_ptr get(AMX *amx, cell addr);
	size_t local_size() const;
	size_t global_size() const;
};

#endif
