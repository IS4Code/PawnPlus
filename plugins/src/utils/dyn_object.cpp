#include "dyn_object.h"
#include "object_pool.h"
#include "../fixes/linux.h"
#include <cmath>
#include "../strings.h"

dyn_object::dyn_object() : is_array(true), array_size(0), array_value(nullptr)
{

}

dyn_object::dyn_object(AMX *amx, cell value, cell tag_id) : is_array(false), cell_value(value), tag_name(find_tag(amx, tag_id))
{

}

dyn_object::dyn_object(AMX *amx, cell *arr, cell size, cell tag_id) : is_array(true), array_size(size), tag_name(find_tag(amx, tag_id))
{
	array_value = std::make_unique<cell[]>(size);
	std::memcpy(array_value.get(), arr, size * sizeof(cell));
}

dyn_object::dyn_object(AMX *amx, cell *str) : is_array(true), tag_name("char")
{
	if(str == nullptr || !str[0])
	{
		array_size = 1;
		array_value = std::make_unique<cell[]>(1);
		array_value[0] = 0;
		return;
	}
	int len;
	amx_StrLen(str, &len);
	size_t size;
	if(str[0] & 0xFF000000)
	{
		size = 1 + ((len - 1) / sizeof(cell));
	}else{
		size = len;
	}
	array_size = size + 1;
	array_value = std::make_unique<cell[]>(array_size);
	std::memcpy(array_value.get(), str, size * sizeof(cell));
	array_value[size] = 0;
}

dyn_object::dyn_object(cell value, const char *tag) : is_array(false), cell_value(value), tag_name(tag)
{

}

dyn_object::dyn_object(cell *arr, cell size, const char *tag) : is_array(true), array_size(size), tag_name(tag)
{
	array_value = std::make_unique<cell[]>(size);
	if(arr != nullptr)
	{
		std::memcpy(array_value.get(), arr, size * sizeof(cell));
	}
}

dyn_object::dyn_object(const dyn_object &obj) : is_array(obj.is_array), tag_name(obj.tag_name)
{
	if(is_array)
	{
		array_size = obj.array_size;
		array_value = std::make_unique<cell[]>(array_size);
		std::memcpy(array_value.get(), obj.array_value.get(), array_size * sizeof(cell));
	}else{
		cell_value = obj.cell_value;
	}
}

dyn_object::dyn_object(dyn_object &&obj) : is_array(obj.is_array), tag_name(std::move(obj.tag_name))
{
	if(is_array)
	{
		array_size = obj.array_size;
		array_value = std::move(obj.array_value);
	}else{
		cell_value = obj.cell_value;
	}
}

std::string dyn_object::find_tag(AMX *amx, cell tag_id) const
{
	tag_id &= 0x7FFFFFFF;

	int len;
	amx_NameLength(amx, &len);
	char *tagname = static_cast<char*>(alloca(len+1));

	int num;
	amx_NumTags(amx, &num);
	for(int i = 0; i < num; i++)
	{
		cell tag_id2;
		if(!amx_GetTag(amx, i, tagname, &tag_id2))
		{
			if(tag_id == tag_id2)
			{
				return std::string(tagname);
			}
		}
	}
	return std::string();
}

bool dyn_object::check_tag(AMX *amx, cell tag_id) const
{
	tag_id &= 0x7FFFFFFF;

	if(tag_id == 0) //weak tags
	{
		if(tag_name.empty()) return true;
		char c = tag_name[0];
		if(c < 'A' || c > 'Z') return true;
	}

	int len;
	amx_NameLength(amx, &len);
	char *tagname = static_cast<char*>(alloca(len+1));

	int num;
	amx_NumTags(amx, &num);
	for(int i = 0; i < num; i++)
	{
		cell tag_id2;
		if(!amx_GetTag(amx, i, tagname, &tag_id2) && tag_name == tagname)
		{
			if(
				tag_id == tag_id2 ||
				(tag_id == 0 && (tag_id2 & 0x40000000) == 0) ||
				(tag_name == "GlobalString" && find_tag(amx, tag_id) == "String") ||
				(tag_name == "GlobalVariant" && find_tag(amx, tag_id) == "Variant")
			)
			{
				return true;
			}
		}
	}
	return false;
}

bool dyn_object::get_cell(cell index, cell &value) const
{
	if(index < 0) return false;
	if(is_array)
	{
		if(index >= array_size) return false;
		value = array_value[index];
		return true;
	} else {
		if(index > 0) return false;
		value = cell_value;
		return true;
	}
}

cell dyn_object::get_array(cell *arr, cell maxsize) const
{
	if(is_array)
	{
		if(maxsize > array_size) maxsize = array_size;
		std::memcpy(arr, array_value.get(), maxsize * sizeof(cell));
		return maxsize;
	}else{
		*arr = cell_value;
		return 1;
	}
}

bool dyn_object::set_cell(cell index, cell value)
{
	if(index < 0) return false;
	if(is_array)
	{
		if(index >= array_size) return false;
		array_value[index] = value;
		return true;
	}
	return false;
}

cell dyn_object::get_tag(AMX *amx) const
{
	if(tag_name.empty()) return 0x80000000;

	int len;
	amx_NameLength(amx, &len);
	char *tagname = static_cast<char*>(alloca(len+1));

	int num;
	amx_NumTags(amx, &num);
	for(int i = 0; i < num; i++)
	{
		cell tag_id;
		if(!amx_GetTag(amx, i, tagname, &tag_id))
		{
			if(tag_name == tagname)
			{
				return tag_id | 0x80000000;
			}
		}
	}
	return 0;
}

cell dyn_object::store(AMX *amx) const
{
	if(is_array)
	{
		cell amx_addr, *addr;
		amx_Allot(amx, array_size, &amx_addr, &addr);
		std::memcpy(addr, array_value.get(), array_size * sizeof(cell));
		return amx_addr;
	}else{
		return cell_value;
	}
}

void dyn_object::load(AMX *amx, cell amx_addr)
{
	if(is_array)
	{
		cell *addr;
		amx_GetAddr(amx, amx_addr, &addr);
		std::memcpy(array_value.get(), addr, array_size * sizeof(cell));
	}
}

cell dyn_object::get_size() const
{
	if(is_array)
	{
		return array_size;
	}else{
		return 1;
	}
}

char dyn_object::get_specifier() const
{
	if(is_array)
	{
		if(tag_name == "char") return 's';
		if(array_size > 1) return 'a';
	}
	if(tag_name == "Float") return 'f';
	if(tag_name == "String" || tag_name == "GlobalString") return 'S';
	return 'i';
}

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
	std::hash<T> hasher;
	hash_combine(seed, hasher(v));
}

inline void hash_combine(size_t& seed, size_t h)
{
	seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t dyn_object::tag_hash() const
{
	std::hash<std::string> hasher;
	if(tag_name == "GlobalString") return hasher("String");
	if(tag_name == "GlobalVariant") return hasher("Variant");
	return hasher(tag_name);
}

size_t dyn_object::get_hash() const
{
	std::hash<cell> hasher;

	size_t hash = 0;
	if(is_array)
	{
		for(cell i = 0; i < array_size; i++)
		{
			hash_combine(hash, array_value[i]);
		}
	}else{
		hash = hasher(cell_value);
	}

	hash_combine(hash, tag_hash());
	return hash;
}

bool dyn_object::empty() const
{
	return is_array ? array_size == 0 : false;
}

bool dyn_object::tag_check(const dyn_object &obj) const
{
	if(tag_name == obj.tag_name) return true;
	return
		(tag_name == "GlobalString" && obj.tag_name == "String") ||
		(tag_name == "String" && obj.tag_name == "GlobalString") ||
		(tag_name == "GlobalVariant" && obj.tag_name == "Variant") ||
		(tag_name == "Variant" && obj.tag_name == "GlobalVariant");
}

cell &dyn_object::operator[](cell index)
{
	if(is_array)
	{
		return array_value[index];
	} else {
		return (&cell_value)[index];
	}
}

const cell &dyn_object::operator[](cell index) const
{
	if(is_array)
	{
		return array_value[index];
	} else {
		return (&cell_value)[index];
	}
}

bool operator==(const dyn_object &a, const dyn_object &b)
{
	if(a.is_array != b.is_array || !a.tag_check(b)) return a.empty() && b.empty();
	if(a.is_array)
	{
		if(a.array_size != b.array_size) return false;
		return !std::memcmp(a.array_value.get(), b.array_value.get(), a.array_size * sizeof(cell));
	}else{
		return a.cell_value == b.cell_value;
	}
}

template <class T>
struct op_add
{
	T operator()(T a, T b)
	{
		return {};
	}
};
template <>
struct op_add<cell>
{
	cell operator()(cell a, cell b)
	{
		return a + b;
	}
};
template <>
struct op_add<float>
{
	float operator()(float a, float b)
	{
		return a + b;
	}
};
template <>
struct op_add<strings::cell_string*>
{
	strings::cell_string *operator()(strings::cell_string *a, strings::cell_string *b)
	{
		if(a == nullptr && b == nullptr)
		{
			return strings::pool.add(true);
		}
		if(a == nullptr)
		{
			auto str = *b;
			return strings::pool.add(std::move(str), true);
		}
		if(b == nullptr)
		{
			auto str = *a;
			return strings::pool.add(std::move(str), true);
		}
		auto str = *a + *b;
		return strings::pool.add(std::move(str), true);
	}
};

namespace variants
{
	extern object_pool<dyn_object> pool;
}

template <>
struct op_add<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b)
	{
		if(a == nullptr || b == nullptr)
		{
			return nullptr;
		}
		auto var = *a + *b;
		if(var.empty()) return nullptr;
		return variants::pool.add(std::move(var), true);
	}
};
template <class T>
struct op_sub
{
	T operator()(T a, T b)
	{
		return {};
	}
};
template <>
struct op_sub<cell>
{
	cell operator()(cell a, cell b)
	{
		return a - b;
	}
};
template <>
struct op_sub<float>
{
	float operator()(float a, float b)
	{
		return a - b;
	}
};
template <>
struct op_sub<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b)
	{
		if(a == nullptr || b == nullptr)
		{
			return nullptr;
		}
		auto var = *a - *b;
		if(var.empty()) return nullptr;
		return variants::pool.add(std::move(var), true);
	}
};
template <class T>
struct op_mul
{
	T operator()(T a, T b)
	{
		return {};
	}
};
template <>
struct op_mul<cell>
{
	cell operator()(cell a, cell b)
	{
		return a * b;
	}
};
template <>
struct op_mul<float>
{
	float operator()(float a, float b)
	{
		return a * b;
	}
};
template <>
struct op_mul<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b)
	{
		if(a == nullptr || b == nullptr)
		{
			return nullptr;
		}
		auto var = *a * *b;
		if(var.empty()) return nullptr;
		return variants::pool.add(std::move(var), true);
	}
};
template <class T>
struct op_div
{
	T operator()(T a, T b)
	{
		return {};
	}
};
template <>
struct op_div<cell>
{
	cell operator()(cell a, cell b)
	{
		return a / b;
	}
};
template <>
struct op_div<float>
{
	float operator()(float a, float b)
	{
		return a / b;
	}
};
template <>
struct op_div<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b)
	{
		if(a == nullptr || b == nullptr)
		{
			return nullptr;
		}
		auto var = *a / *b;
		if(var.empty()) return nullptr;
		return variants::pool.add(std::move(var), true);
	}
};

template <class T>
struct op_mod
{
	T operator()(T a, T b)
	{
		return {};
	}
};

template <>
struct op_mod<cell>
{
	cell operator()(cell a, cell b)
	{
		return a % b;
	}
};
template <>
struct op_mod<float>
{
	float operator()(float a, float b)
	{
		return std::fmod(a, b);
	}
};
template <>
struct op_mod<strings::cell_string*>
{
	strings::cell_string *operator()(strings::cell_string *a, strings::cell_string *b)
	{
		if(a == nullptr && b == nullptr)
		{
			return strings::pool.add(true);
		}
		if(a == nullptr)
		{
			auto str = *b;
			return strings::pool.add(std::move(str), true);
		}
		if(b == nullptr)
		{
			auto str = *a;
			return strings::pool.add(std::move(str), true);
		}
		auto str = *a + *b;
		return strings::pool.add(std::move(str), true);
	}
};
template <>
struct op_mod<dyn_object*>
{
	dyn_object *operator()(dyn_object *a, dyn_object *b)
	{
		if(a == nullptr || b == nullptr)
		{
			return nullptr;
		}
		auto var = *a % *b;
		if(var.empty()) return nullptr;
		return variants::pool.add(std::move(var), true);
	}
};

template <class TagType>
struct tag_info;

template <>
struct tag_info<cell>
{
	static constexpr const char *tag_name = "";

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
struct tag_info<float>
{
	static constexpr const char *tag_name = "Float";

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

	static dyn_object *conv_to(cell v)
	{
		return reinterpret_cast<dyn_object*>(v);
	}

	static cell conv_from(dyn_object *v)
	{
		return reinterpret_cast<cell>(v);
	}
};

template <template <class T> class OpType, class TagType>
dyn_object dyn_object::operator_func_tagged(const dyn_object &obj) const
{
	typedef tag_info<TagType> tag;
	if(is_array && obj.is_array)
	{
		if(array_size != obj.array_size) return dyn_object();

		dyn_object result{nullptr, array_size, tag::tag_name};
		OpType<TagType> op;
		for(cell i = 0; i < array_size; i++)
		{
			TagType val = op(tag::conv_to(array_value[i]), tag::conv_to(obj.array_value[i]));
			result.array_value[i] = tag::conv_from(val);
		}
		return result;
	}else if(is_array)
	{
		dyn_object result{nullptr, array_size, tag::tag_name};
		OpType<TagType> op;
		TagType sval = tag::conv_to(obj.cell_value);
		for(cell i = 0; i < array_size; i++)
		{
			TagType val = op(tag::conv_to(array_value[i]), sval);
			result.array_value[i] = tag::conv_from(val);
		}
		return result;
	}else if(obj.is_array)
	{
		dyn_object result{nullptr, obj.array_size, tag::tag_name};
		OpType<TagType> op;
		TagType sval = tag::conv_to(cell_value);
		for(cell i = 0; i < obj.array_size; i++)
		{
			TagType val = op(sval, tag::conv_to(obj.array_value[i]));
			result.array_value[i] = tag::conv_from(val);
		}
		return result;
	}else{
		OpType<TagType> op;
		TagType val = op(tag::conv_to(cell_value), tag::conv_to(obj.cell_value));
		return dyn_object(tag::conv_from(val), tag::tag_name);
	}
	return dyn_object();
}

template <template <class T> class OpType>
dyn_object dyn_object::operator_func(const dyn_object &obj) const
{
	if(!tag_check(obj)) return dyn_object();
	if(tag_name.empty()) return operator_func_tagged<OpType, cell>(obj);
	if(tag_name == "Float") return operator_func_tagged<OpType, float>(obj);
	if(tag_name == "String" || tag_name == "GlobalString") return operator_func_tagged<OpType, strings::cell_string*>(obj);
	if(tag_name == "Variant" || tag_name == "GlobalVariant") return operator_func_tagged<OpType, dyn_object*>(obj);
	return dyn_object();
}

dyn_object dyn_object::operator+(const dyn_object &obj) const
{
	return operator_func<op_add>(obj);
}

dyn_object dyn_object::operator-(const dyn_object &obj) const
{
	return operator_func<op_sub>(obj);
}

dyn_object dyn_object::operator*(const dyn_object &obj) const
{
	return operator_func<op_mul>(obj);
}

dyn_object dyn_object::operator/(const dyn_object &obj) const
{
	return operator_func<op_div>(obj);
}

dyn_object dyn_object::operator%(const dyn_object &obj) const
{
	return operator_func<op_mod>(obj);
}

dyn_object &dyn_object::operator=(const dyn_object &obj)
{
	if(this == &obj) return *this;
	is_array = obj.is_array;
	tag_name = obj.tag_name;
	if(is_array)
	{
		array_size = obj.array_size;
		array_value = std::make_unique<cell[]>(array_size);
		std::memcpy(array_value.get(), obj.array_value.get(), array_size * sizeof(cell));
	}else{
		cell_value = obj.cell_value;
	}
	return *this;
}

dyn_object &dyn_object::operator=(dyn_object &&obj)
{
	if(this == &obj) return *this;
	is_array = obj.is_array;
	tag_name = std::move(obj.tag_name);
	if(is_array)
	{
		array_size = obj.array_size;
		array_value = std::move(obj.array_value);
	}else{
		cell_value = obj.cell_value;
	}
	return *this;
}
