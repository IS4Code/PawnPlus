#include "dyn_object.h"
#include "dyn_op.h"
#include "../fixes/linux.h"
#include <cmath>

using cell_string = strings::cell_string;

dyn_object::dyn_object() : is_array(true), array_size(0), array_value(nullptr), tag(tags::find_tag(tags::tag_cell))
{

}

dyn_object::dyn_object(AMX *amx, cell value, cell tag_id) : is_array(false), cell_value(value), tag(tags::find_tag(amx, tag_id))
{

}

dyn_object::dyn_object(AMX *amx, cell *arr, cell size, cell tag_id) : is_array(true), array_size(size), tag(tags::find_tag(amx, tag_id))
{
	array_value = std::make_unique<cell[]>(size);
	std::memcpy(array_value.get(), arr, size * sizeof(cell));
}

dyn_object::dyn_object(AMX *amx, cell *str) : is_array(true), tag(tags::find_tag(tags::tag_char))
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

dyn_object::dyn_object(cell value, tag_ptr tag) : is_array(false), cell_value(value), tag(tag)
{

}

dyn_object::dyn_object(cell *arr, cell size, tag_ptr tag) : is_array(true), array_size(size), tag(tag)
{
	array_value = std::make_unique<cell[]>(size);
	if(arr != nullptr)
	{
		std::memcpy(array_value.get(), arr, size * sizeof(cell));
	}
}

dyn_object::dyn_object(const dyn_object &obj) : is_array(obj.is_array), tag(obj.tag)
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

dyn_object::dyn_object(dyn_object &&obj) : is_array(obj.is_array), tag(obj.tag)
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
	tag_ptr test_tag = tags::find_tag(amx, tag_id);
	if(test_tag->uid == tags::tag_cell)
	{
		if(!tag->strong()) return true;
	}
	return tag->inherits_from(test_tag);
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
	if(tag->uid == tags::tag_cell) return 0x80000000;

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
			if(tag->name == tagname)
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

template <class TagType>
bool dyn_object::get_specifier_tagged(char &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	if(is_array)
	{
		result = tag_traits<TagType[]>::format_spec;
	}else{
		result = tag_traits<TagType>::format_spec;
	}
	return true;
}

char dyn_object::get_specifier() const
{
	char result = tag_traits<cell>::format_spec;
	bool ok = get_specifier_tagged<cell>(result) || get_specifier_tagged<char>(result) || get_specifier_tagged<float>(result) || get_specifier_tagged<cell_string*>(result) || get_specifier_tagged<dyn_object*>(result);
	if(!ok && is_array)
	{
		return tag_traits<cell[]>::format_spec;
	}
	return result;
}

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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

	hash_combine(hash, tag->find_top_base());
	return hash;
}

bool dyn_object::empty() const
{
	return is_array ? array_size == 0 : false;
}

bool dyn_object::tag_check(const dyn_object &obj) const
{
	return tag->same_base(obj.tag);
}

cell &dyn_object::operator[](cell index)
{
	if(is_array)
	{
		return array_value[index];
	}else{
		return (&cell_value)[index];
	}
}

const cell &dyn_object::operator[](cell index) const
{
	if(is_array)
	{
		return array_value[index];
	}else{
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

template <template <class T> class OpType, class TagType>
bool dyn_object::operator_func_tagged(const dyn_object &obj, dyn_object &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	if(is_array && obj.is_array)
	{
		if(array_size != obj.array_size) return true;

		result = {nullptr, array_size, tags::find_tag(tag::tag_uid)};
		OpType<TagType> op;
		for(cell i = 0; i < array_size; i++)
		{
			TagType val = op(tag::conv_to(array_value[i]), tag::conv_to(obj.array_value[i]));
			result.array_value[i] = tag::conv_from(val);
		}
	}else if(is_array)
	{
		result = {nullptr, array_size, tags::find_tag(tag::tag_uid)};
		OpType<TagType> op;
		TagType sval = tag::conv_to(obj.cell_value);
		for(cell i = 0; i < array_size; i++)
		{
			TagType val = op(tag::conv_to(array_value[i]), sval);
			result.array_value[i] = tag::conv_from(val);
		}
	}else if(obj.is_array)
	{
		result = {nullptr, obj.array_size, tags::find_tag(tag::tag_uid)};
		OpType<TagType> op;
		TagType sval = tag::conv_to(cell_value);
		for(cell i = 0; i < obj.array_size; i++)
		{
			TagType val = op(sval, tag::conv_to(obj.array_value[i]));
			result.array_value[i] = tag::conv_from(val);
		}
	}else{
		OpType<TagType> op;
		TagType val = op(tag::conv_to(cell_value), tag::conv_to(obj.cell_value));
		result = {tag::conv_from(val), tags::find_tag(tag::tag_uid)};
	}
	return true;
}

template <template <class T> class OpType>
dyn_object dyn_object::operator_func(const dyn_object &obj) const
{
	if(!tag_check(obj)) return dyn_object();
	dyn_object result;
	operator_func_tagged<OpType, cell>(obj, result) || operator_func_tagged<OpType, float>(obj, result) || operator_func_tagged<OpType, cell_string*>(obj, result) || operator_func_tagged<OpType, dyn_object*>(obj, result);
	return result;
}

template <template <class T> class OpType, class TagType>
bool dyn_object::operator_func_tagged(dyn_object &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	if(is_array)
	{
		result = {nullptr, array_size, tag::tag_name};
		OpType<TagType> op;
		for(cell i = 0; i < array_size; i++)
		{
			TagType val = op(tag::conv_to(array_value[i]));
			result.array_value[i] = tag::conv_from(val);
		}
	}else{
		OpType<TagType> op;
		TagType val = op(tag::conv_to(cell_value));
		result = {tag::conv_from(val), tag::tag_name};
	}
	return true;
}

template <template <class T> class OpType>
dyn_object dyn_object::operator_func() const
{
	dyn_object result;
	operator_func_tagged<OpType, cell>(result) || operator_func_tagged<OpType, float>(result) || operator_func_tagged<OpType, cell_string*>(result) || operator_func_tagged<OpType, dyn_object*>(result);
	return result;
}

template <class TagType>
bool dyn_object::to_string_tagged(cell_string &str) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	op_strval<TagType> op;

	str = {};
	if(is_array)
	{
		str.append({'{'});
		for(cell i = 0; i < array_size; i++)
		{
			if(i > 0)
			{
				str.append({',', ' '});
			}
			str.append(op(tag_traits<TagType>::conv_to(array_value[i])));
		}
		str.append({'}'});
	}else{
		str.append(op(tag_traits<TagType>::conv_to(cell_value)));
	}
	return true;
}

template <>
bool dyn_object::to_string_tagged<char[]>(cell_string &str) const
{
	if(!tag->inherits_from(tag_traits<char[]>::tag_uid)) return false;

	if(is_array)
	{
		str = cell_string(array_value.get());
	}else{
		str = cell_string(1, cell_value);
	}
	return true;
}

cell_string dyn_object::to_string() const
{
	cell_string str;
	bool ok = to_string_tagged<cell>(str) || to_string_tagged<bool>(str) || to_string_tagged<float>(str) || to_string_tagged<char[]>(str) || to_string_tagged<char>(str) || to_string_tagged<cell_string*>(str) || to_string_tagged<dyn_object*>(str);
	if(ok) return str;
	op_strval<cell> op;
	str.append(strings::convert(tag->name));
	str.append({':'});
	if(is_array)
	{
		str.append({'{'});
		for(cell i = 0; i < array_size; i++)
		{
			if(i > 0)
			{
				str.append({',', ' '});
			}
			str.append(op(tag_traits<cell>::conv_to(array_value[i])));
		}
		str.append({'}'});
	}else{
		str.append(op(tag_traits<cell>::conv_to(cell_value)));
	}
	return str;
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
	tag = obj.tag;
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
	tag = obj.tag;
	if(is_array)
	{
		array_size = obj.array_size;
		array_value = std::move(obj.array_value);
	}else{
		cell_value = obj.cell_value;
	}
	return *this;
}
