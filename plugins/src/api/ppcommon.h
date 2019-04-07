#ifndef PPCOMMON_H_INCLUDED
#define PPCOMMON_H_INCLUDED

#include "sdk/amx/amx.h"

#include <stdexcept>

namespace pp
{
	constexpr uint16_t hook_magic = 31497;
	constexpr uint16_t hook_flags_initial = 44572;
	constexpr uint16_t hook_flags_final = 17562;

	bool load();

	class api_table
	{
		void **_ptr;
		size_t _size;

		void load(void **ptr);

		friend bool pp::load();

	protected:
		template <class Func>
		Func *get(size_t idx) const
		{
			if(idx >= _size) throw std::logic_error("the requested function is not provided");
			return reinterpret_cast<Func*>(_ptr[idx]);
		}

	public:
		size_t size() const
		{
			return _size;
		}
	};

	class main_table : public api_table
	{
	public:
		cell version() const
		{
			return get<cell()>(0)();
		}

		void collect() const
		{
			return get<void()>(1)();
		}

		void *register_on_collect(void(*func)()) const
		{
			return get<void*(void(*func)())>(2)(func);
		}

		void unregister_on_collect(void *id) const
		{
			return get<void(void *id)>(3)(id);
		}

		cell add_addon(const char *name, void *ptr)
		{
			return get<cell(const char *name, void *ptr)>(4)(name, ptr);
		}

		cell remove_addon(const char *name)
		{
			return get<cell(const char *name)>(5)(name);
		}

		void *get_addon(const char *name)
		{
			return get<void*(const char *name)>(6)(name);
		}
	};

	class tag_table : public api_table
	{
	public:
		const void *find_name(const char *name) const
		{
			return get<const void*(const char *name)>(0)(name);
		}

		const void *find_uid(cell tag_uid) const
		{
			return get<const void*(cell tag_uid)>(1)(tag_uid);
		}

		cell get_uid(const void *tag) const
		{
			return get<cell(const void *tag)>(2)(tag);
		}

		bool set_op(const void *tag, cell optype, cell(*handler)(void *cookie, const void *tag, cell *args, cell numargs), void *cookie) const
		{
			return get<cell(const void *tag, cell optype, cell(*handler)(void *cookie, const void *tag, cell *args, cell numargs), void *cookie)>(3)(tag, optype, handler, cookie);
		}
	};

	class dyn_object_table : public api_table
	{
	public:
		void *create_empty() const
		{
			return get<void*()>(0)();
		}

		void *create_copy(const void *obj) const
		{
			return get<void*(const void *obj)>(1)(obj);
		}

		void *create_move(void *obj) const
		{
			return get<void*(void *obj)>(2)(obj);
		}

		void *create_cell(cell value, const void *tag) const
		{
			return get<void*(cell value, const void *tag)>(3)(value, tag);
		}

		void *create_arr(const cell *arr, cell size, const void *tag) const
		{
			return get<void*(const cell *arr, cell size, const void *tag)>(4)(arr, size, tag);
		}

		void *create_str(const cell *str) const
		{
			return get<void*(const cell *str)>(5)(str);
		}

		void remove(void *obj) const
		{
			return get<void(void *obj)>(6)(obj);
		}

		void assign_copy(void *self, const void *obj) const
		{
			return get<void(void *self, const void *obj)>(7)(self, obj);
		}

		void assign_move(void *self, void *obj) const
		{
			return get<void(void *self, void *obj)>(8)(self, obj);
		}

		const void *dyn_object_get_tag(const void *obj) const
		{
			return get<const void*(const void *obj)>(9)(obj);
		}
		
		cell dyn_object_get_size(const void *obj) const
		{
			return get<cell(const void *obj)>(10)(obj);
		}
		
		bool dyn_object_is_array(const void *obj) const
		{
			return get<cell(const void *obj)>(11)(obj);
		}
		
		cell dyn_object_get_cell(const void *obj, cell index) const
		{
			return get<cell(const void *obj, cell index)>(12)(obj, index);
		}
		
		cell dyn_object_get_array(const void *obj, cell *arr, cell maxsize) const
		{
			return get<cell(const void *obj, cell *arr, cell maxsize)>(13)(obj, arr, maxsize);
		}
		
		void dyn_object_acquire(void *obj) const
		{
			return get<void(void *obj)>(14)(obj);
		}
		
		void dyn_object_release(void *obj) const
		{
			return get<void(void *obj)>(15)(obj);
		}

		cell dyn_object_get_rank(const void *obj) const
		{
			return get<cell(const void *obj)>(16)(obj);
		}
	};

	class list_table : public api_table
	{
	public:
		void *create() const
		{
			return get<void*()>(0)();
		}

		void remove(void *list) const
		{
			return get<void(void *list)>(1)(list);
		}

		cell get_id(void *list) const
		{
			return get<cell(void *list)>(2)(list);
		}

		void *from_id(cell id) const
		{
			return get<void*(cell id)>(3)(id);
		}
		
		size_t get_size(const void *list) const
		{
			return get<cell(const void *list)>(4)(list);
		}

		void *item(void *list, cell index) const
		{
			return get<void*(void *list, cell index)>(5)(list, index);
		}

		void add_copy(void *list, const void *obj) const
		{
			return get<void(void *list, const void *obj)>(6)(list, obj);
		}

		void add_move(void *list, void *obj) const
		{
			return get<void(void *list, void *obj)>(7)(list, obj);
		}

		void clear(void *list) const
		{
			return get<void(void *list)>(8)(list);
		}
	};

	class linked_list_table : public api_table
	{
	public:
		void *create() const
		{
			return get<void*()>(0)();
		}

		void remove(void *linked_list) const
		{
			return get<void(void *linked_list)>(1)(linked_list);
		}

		cell get_id(void *linked_list) const
		{
			return get<cell(void *linked_list)>(2)(linked_list);
		}

		void *from_id(cell id) const
		{
			return get<void*(cell id)>(3)(id);
		}

		size_t get_size(const void *linked_list) const
		{
			return get<cell(const void *linked_list)>(4)(linked_list);
		}

		void *item(void *linked_list, cell index) const
		{
			return get<void*(void *linked_list, cell index)>(5)(linked_list, index);
		}

		void add_copy(void *linked_list, const void *obj) const
		{
			return get<void(void *linked_list, const void *obj)>(6)(linked_list, obj);
		}

		void add_move(void *linked_list, void *obj) const
		{
			return get<void(void *linked_list, void *obj)>(7)(linked_list, obj);
		}

		void clear(void *linked_list) const
		{
			return get<void(void *linked_list)>(8)(linked_list);
		}
	};

	class map_table : public api_table
	{
	public:
		void *create() const
		{
			return get<void*()>(0)();
		}

		void remove(void *map) const
		{
			return get<void(void *map)>(1)(map);
		}
		
		cell get_id(void *map) const
		{
			return get<cell(void *map)>(2)(map);
		}
		
		void *from_id(cell id) const
		{
			return get<void*(cell id)>(3)(id);
		}
		
		size_t get_size(const void *map) const
		{
			return get<cell(const void *map)>(4)(map);
		}
		
		void *item(void *map, const void *key) const
		{
			return get<void*(void *map, const void *key)>(5)(map, key);
		}
		
		void *find(void *map, const void *key) const
		{
			return get<void*(void *map, const void *key)>(6)(map, key);
		}
		
		bool add_copy(void *map, const void *key, const void *value) const
		{
			return get<cell(void *map, const void *key, const void *value)>(7)(map, key, value);
		}
		
		bool add_move(void *map, const void *key, void *value) const
		{
			return get<cell(void *map, const void *key, void *value)>(8)(map, key, value);
		}
		
		void clear(void *map) const
		{
			return get<void(void *map)>(9)(map);
		}
		
		bool is_ordered(void *map) const
		{
			return get<cell(void *map)>(10)(map);
		}

		void set_ordered(void *map, bool ordered) const
		{
			return get<void(void *map, cell ordered)>(11)(map, ordered);
		}
	};

	class string_table : public api_table
	{
	public:
		void *create()
		{
			return get<void*()>(0)();
		}
		
		void remove(void *str)
		{
			return get<void(void *str)>(1)(str);
		}

		cell get_id(void *str)
		{
			return get<cell(void *str)>(2)(str);
		}

		void *from_id(cell id)
		{
			return get<void*(cell id)>(3)(id);
		}

		bool acquire(void *str)
		{
			return get<cell(void *str)>(4)(str);
		}

		bool release(void *str)
		{
			return get<cell(void *str)>(5)(str);
		}

		cell get_size(const void *str)
		{
			return get<cell(const void *str)>(6)(str);
		}

		cell *get_data(void *str)
		{
			return get<cell*(void *str)>(7)(str);
		}

		cell create(const char *c, cell len)
		{
			return get<cell(const char *c, cell len)>(8)(c, len);
		}

		void append(void *str, const cell *data, cell len)
		{
			return get<void(void *str, const cell *data, cell len)>(9)(str, data, len);
		}
	};


	class variant_table : public api_table
	{
	public:
		void *create()
		{
			return get<void*()>(0)();
		}

		void remove(void *var)
		{
			return get<void(void *var)>(1)(var);
		}

		cell get_id(void *var)
		{
			return get<cell(void *var)>(2)(var);
		}

		void *from_id(cell id)
		{
			return get<void*(cell id)>(3)(id);
		}

		bool acquire(void *var)
		{
			return get<cell(void *var)>(4)(var);
		}

		bool release(void *var)
		{
			return get<cell(void *var)>(5)(var);
		}

		cell get_size(const void *var)
		{
			return get<cell(const void *var)>(6)(var);
		}

		void *get_object(void *var)
		{
			return get<void*(void *var)>(7)(var);
		}
	};

	extern main_table main;
	extern tag_table tag;
	extern dyn_object_table dyn_object;
	extern list_table list;
	extern linked_list_table linked_list;
	extern map_table map;
	extern string_table string;
	extern variant_table variant;
}

#endif
