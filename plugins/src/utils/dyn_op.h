#ifndef DYN_OP_H_INCLUDED
#define DYN_OP_H_INCLUDED

#include "dyn_object.h"
#include "../variants.h"
#include "../strings.h"
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
	strings::cell_string operator()(bool obj) const;
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
struct tag_info;

template <>
struct tag_info<cell>
{
	static constexpr const char *tag_name = "";
	static constexpr const char format_spec = 'i';

	static bool check_tag(const std::string &tag)
	{
		return tag == tag_name;
	}

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
struct tag_info<bool>
{
	static constexpr const char *tag_name = "bool";
	static constexpr const char format_spec = 'i';

	static bool check_tag(const std::string &tag)
	{
		return tag == tag_name;
	}

	static bool conv_to(cell v)
	{
		return !!v;
	}

	static cell conv_from(bool v)
	{
		return v ? 1 : 0;
	}
};

template <>
struct tag_info<float>
{
	static constexpr const char *tag_name = "Float";
	static constexpr const char format_spec = 'f';

	static bool check_tag(const std::string &tag)
	{
		return tag == tag_name;
	}

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
struct tag_info<strings::cell_string*>
{
	static constexpr const char *tag_name = "String";
	static constexpr const char format_spec = 'S';

	static bool check_tag(const std::string &tag)
	{
		return tag == tag_name || tag == "GlobalString";
	}

	static strings::cell_string *conv_to(cell v)
	{
		return reinterpret_cast<strings::cell_string*>(v);
	}

	static cell conv_from(strings::cell_string *v)
	{
		return reinterpret_cast<cell>(v);
	}
};

template <>
struct tag_info<dyn_object*>
{
	static constexpr const char *tag_name = "Variant";
	static constexpr const char format_spec = 'V';

	static bool check_tag(const std::string &tag)
	{
		return tag == tag_name || tag == "GlobalVariant";
	}

	static dyn_object *conv_to(cell v)
	{
		return reinterpret_cast<dyn_object*>(v);
	}

	static cell conv_from(dyn_object *v)
	{
		return reinterpret_cast<cell>(v);
	}
};

#endif
