#ifndef DYN_OP_H_INCLUDED
#define DYN_OP_H_INCLUDED

#include "dyn_object.h"
#include "pools.h"
#include "modules/variants.h"
#include "modules/strings.h"
#include "modules/tags.h"
#include "sdk/amx/amx.h"
#include <string>

template <class T>
struct op_add
{
	T operator()(T a, T b) const
	{
		return {};
	}
};

template <>
struct op_add<cell>
{
	cell operator()(cell a, cell b) const;
};

template <>
struct op_add<float>
{
	float operator()(float a, float b) const;
};

template <>
struct op_add<strings::cell_string*>
{
	strings::cell_string *operator()(strings::cell_string *a, strings::cell_string *b) const;
};

template <>
struct op_add<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b) const;
};

template <class T>
struct op_sub
{
	T operator()(T a, T b) const
	{
		return {};
	}
};

template <>
struct op_sub<cell>
{
	cell operator()(cell a, cell b) const;
};

template <>
struct op_sub<float>
{
	float operator()(float a, float b) const;
};

template <>
struct op_sub<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b) const;
};


template <class T>
struct op_mul
{
	T operator()(T a, T b) const
	{
		return {};
	}
};

template <>
struct op_mul<cell>
{
	cell operator()(cell a, cell b) const;
};

template <>
struct op_mul<float>
{
	float operator()(float a, float b) const;
};

template <>
struct op_mul<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b) const;
};


template <class T>
struct op_div
{
	T operator()(T a, T b) const
	{
		return {};
	}
};

template <>
struct op_div<cell>
{
	cell operator()(cell a, cell b) const;
};

template <>
struct op_div<float>
{
	float operator()(float a, float b) const;
};

template <>
struct op_div<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b) const;
};

template <class T>
struct op_mod
{
	T operator()(T a, T b) const
	{
		return {};
	}
};

template <>
struct op_mod<cell>
{
	cell operator()(cell a, cell b) const;
};

template <>
struct op_mod<float>
{
	float operator()(float a, float b) const;
};

template <>
struct op_mod<strings::cell_string*>
{
	strings::cell_string *operator()(strings::cell_string *a, strings::cell_string *b) const;
};

template <>
struct op_mod<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b) const;
};


template <class T>
struct op_strval
{
	strings::cell_string operator()(T obj) const
	{
		return {};
	}
};

template <>
struct op_strval<cell>
{
	strings::cell_string operator()(cell obj) const;
};

template <>
struct op_strval<bool>
{
	strings::cell_string operator()(cell obj) const;
};

template <>
struct op_strval<char>
{
	strings::cell_string operator()(cell obj) const;
};

template <>
struct op_strval<float>
{
	strings::cell_string operator()(float obj) const;
};

template <>
struct op_strval<strings::cell_string*>
{
	strings::cell_string operator()(strings::cell_string *obj) const;
};

template <>
struct op_strval<dyn_object*>
{
	strings::cell_string operator()(dyn_object *obj) const;
};

template <class TagType>
struct tag_traits;

template <>
struct tag_traits<cell>
{
	static constexpr const cell tag_uid = tags::tag_cell;
	static constexpr const char format_spec = 'i';

	static cell conv_to(cell v)
	{
		return v;
	}

	static cell conv_from(cell v)
	{
		return v;
	}
};

template <class TagType>
struct tag_traits<TagType[]>
{
	static constexpr const cell tag_uid = tag_traits<TagType>::tag_uid;
	static constexpr const char format_spec = 'a';

	static auto conv_to(cell v) -> decltype(tag_traits<TagType>::conv_to(v))
	{
		return tag_traits<TagType>::conv_to(v);
	}

	template <class Type>
	static cell conv_from(Type v)
	{
		return tag_traits<TagType>::conv_from(v);
	}
};

template <>
struct tag_traits<bool>
{
	static constexpr const cell tag_uid = tags::tag_bool;
	static constexpr const char format_spec = 'i';

	static cell conv_to(cell v)
	{
		return v;
	}

	static cell conv_from(cell v)
	{
		return v;
	}
};

template <>
struct tag_traits<char[]>
{
	static constexpr const cell tag_uid = tags::tag_char;
	static constexpr const char format_spec = 's';

	static cell conv_to(cell v)
	{
		return v;
	}

	static cell conv_from(cell v)
	{
		return v;
	}
};

template <>
struct tag_traits<char>
{
	static constexpr const cell tag_uid = tags::tag_char;
	static constexpr const char format_spec = 'c';

	static cell conv_to(cell v)
	{
		return v;
	}

	static cell conv_from(cell v)
	{
		return v;
	}
};

template <>
struct tag_traits<float>
{
	static constexpr const cell tag_uid = tags::tag_float;
	static constexpr const char format_spec = 'f';

	static float conv_to(cell v)
	{
		return amx_ctof(v);
	}

	static cell conv_from(float v)
	{
		return amx_ftoc(v);
	}
};

template <>
struct tag_traits<strings::cell_string*>
{
	static constexpr const cell tag_uid = tags::tag_string;
	static constexpr const char format_spec = 'S';

	static strings::cell_string *conv_to(cell v)
	{
		return reinterpret_cast<strings::cell_string*>(v);
	}

	static cell conv_from(strings::cell_string *v)
	{
		return reinterpret_cast<cell>(v);
	}

	static void free(strings::cell_string *v)
	{
		strings::pool.remove(v);
	}

	static strings::cell_string *clone(strings::cell_string *v)
	{
		return strings::pool.clone(v);
	}
};

template <>
struct tag_traits<dyn_object*>
{
	static constexpr const cell tag_uid = tags::tag_variant;
	static constexpr const char format_spec = 'v';

	static dyn_object *conv_to(cell v)
	{
		return reinterpret_cast<dyn_object*>(v);
	}

	static cell conv_from(dyn_object *v)
	{
		return reinterpret_cast<cell>(v);
	}

	static void free(dyn_object *v)
	{
		if(!variants::pool.contains(v)) return;
		v->free();
		variants::pool.remove(v);
	}

	static dyn_object *clone(dyn_object *v)
	{
		return variants::pool.clone(v);
	}
};

template <>
struct tag_traits<list_t*>
{
	static constexpr const cell tag_uid = tags::tag_list;
	static constexpr const char format_spec = 'l';

	static list_t *conv_to(cell v)
	{
		return reinterpret_cast<list_t*>(v);
	}

	static cell conv_from(list_t *v)
	{
		return reinterpret_cast<cell>(v);
	}

	static void free(list_t *v)
	{
		if(!list_pool.contains(v)) return;
		for(auto &obj : *v)
		{
			obj.free();
		}
		list_pool.remove(v);
	}

	static list_t *clone(list_t *v)
	{
		if(!list_pool.contains(v)) return nullptr;
		list_t *l = list_pool.add();
		for(auto &&obj : *v)
		{
			l->push_back(obj.clone());
		}
		return l;
	}
};

template <>
struct tag_traits<map_t*>
{
	static constexpr const cell tag_uid = tags::tag_map;
	static constexpr const char format_spec = 'm';

	static map_t *conv_to(cell v)
	{
		return reinterpret_cast<map_t*>(v);
	}

	static cell conv_from(map_t *v)
	{
		return reinterpret_cast<cell>(v);
	}

	static void free(map_t *v)
	{
		if(!map_pool.contains(v)) return;
		for(auto &pair : *v)
		{
			pair.first.free();
			pair.second.free();
		}
		map_pool.remove(v);
	}

	static map_t *clone(map_t *v)
	{
		if(!map_pool.contains(v)) return nullptr;
		map_t *m = map_pool.add();
		for(auto &&pair : *v)
		{
			m->insert(std::make_pair(pair.first.clone(), pair.second.clone()));
		}
		return m;
	}
};

#endif
