#include "dyn_object.h"
#include "../fixes/linux.h"
#include <cmath>

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
		return a + b;
	}
};
template <class T>
struct op_sub
{
	T operator()(T a, T b)
	{
		return a - b;
	}
};
template <class T>
struct op_mul
{
	T operator()(T a, T b)
	{
		return a * b;
	}
};
template <class T>
struct op_div
{
	T operator()(T a, T b)
	{
		return a / b;
	}
};
template <class T>
struct op_mod
{
	T operator()(T a, T b)
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

template <template <class T> class OpType>
dyn_object dyn_object::operator_func(const dyn_object &obj) const
{
	if(is_array != obj.is_array || (is_array && array_size != obj.array_size) || !tag_check(obj)) return dyn_object();
	if(!is_array)
	{
		if(tag_name.empty())
		{
			OpType<cell> op;
			cell val = op(cell_value, obj.cell_value);
			return dyn_object(val, "");
		}
		if(tag_name == "Float")
		{
			OpType<float> op;
			float val = op(amx_ctof(cell_value), amx_ctof(obj.cell_value));
			return dyn_object(amx_ftoc(val), "Float");
		}
	}else{
		if(tag_name.empty())
		{
			dyn_object result{nullptr, array_size, ""};
			OpType<cell> op;
			for(cell i = 0; i < array_size; i++)
			{
				cell val = op(array_value[i], obj.array_value[i]);
				result.array_value[i] = val;
			}
			return result;
		}
		if(tag_name == "Float")
		{
			dyn_object result{nullptr, array_size, "Float"};
			OpType<float> op;
			for(cell i = 0; i < array_size; i++)
			{
				float val = op(amx_ctof(array_value[i]), amx_ctof(obj.array_value[i]));
				result.array_value[i] = amx_ftoc(val);
			}
			return result;
		}
	}
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
