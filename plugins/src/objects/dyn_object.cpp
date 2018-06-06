#include "dyn_object.h"
#include "dyn_op.h"
#include "../fixes/linux.h"
#include <cmath>
#include <type_traits>
#include <algorithm>

using cell_string = strings::cell_string;

bool memequal(void const* ptr1, void const* ptr2, size_t size)
{
	return !std::memcmp(ptr1, ptr2, size);
}

dyn_object::dyn_object() : dimension(1), array_data(nullptr), tag(tags::find_tag(tags::tag_cell))
{

}

dyn_object::dyn_object(AMX *amx, cell value, cell tag_id) : dimension(0), cell_value(value), tag(tags::find_tag(amx, tag_id))
{

}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell tag_id) : dimension(1), tag(tags::find_tag(amx, tag_id))
{
	if(arr != nullptr)
	{
		array_data = new cell[size + 1];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
	}else{
		array_data = new cell[size + 1]();
	}
	array_data[0] = size + 1;
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell tag_id) : dimension(2), tag(tags::find_tag(amx, tag_id))
{
	if(arr != nullptr)
	{
		const cell *last = &arr[size - 1];
		last = (cell*)((char*)last + *last);
		if(size2 > 0)
		{
			last += size2;
		}else if(!*last)
		{
			last++;
		}else{
			auto data = amx->base + ((AMX_HEADER*)amx->base)->dat;
			auto dat = (cell*)(data);
			auto hlw = (cell*)(data + amx->hlw);
			auto hea = (cell*)(data + amx->hea);
			auto stk = (cell*)(data + amx->stk);
			auto stp = (cell*)(data + amx->stp);

			bool on_dat = arr >= dat && arr < hlw;
			bool on_heap = arr >= hlw && arr < hea;
			bool on_stack = arr >= stk && arr < stp;

			while(true)
			{
				if(on_dat && last >= hlw) break;
				if(on_heap && last >= hea) break;
				if(on_stack && last >= stp) break;

				last++;
				if(!*last)
				{
					last++;
					break;
				}
			}
		}
		cell length = last - arr;
		array_data = new cell[length + 1];
		std::memcpy(array_data + 1, arr, length * sizeof(cell));
		array_data[0] = length + 1;
	}else{
		cell length = size2 > 0 ? size + size * size2 : size;
		array_data = new cell[length + 1]();
		for(cell i = 1; i <= size; i++)
		{
			array_data[i] = size - i;
		}
		array_data[0] = length + 1;
	}
}

dyn_object::dyn_object(AMX *amx, const cell *str) : dimension(1), tag(tags::find_tag(tags::tag_char))
{
	if(str == nullptr || !str[0])
	{
		array_data = new cell[2]{2, 0};
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
	array_data = new cell[size + 2];
	array_data[0] = size + 2;
	std::memcpy(array_data + 2, str, size * sizeof(cell));
	array_data[size + 1] = 0;
}

dyn_object::dyn_object(cell value, tag_ptr tag) : dimension(0), cell_value(value), tag(tag)
{

}

dyn_object::dyn_object(const cell *arr, cell size, tag_ptr tag) : dimension(1), tag(tag)
{
	if(arr != nullptr)
	{
		array_data = new cell[size + 1];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
	} else {
		array_data = new cell[size + 1]();
	}
	array_data[0] = size + 1;
}

dyn_object::dyn_object(const dyn_object &obj) : dimension(obj.dimension), tag(obj.tag)
{
	if(is_array())
	{
		if(obj.array_data != nullptr)
		{
			cell size = obj.data_size();
			array_data = new cell[size];
			std::memcpy(array_data, obj.array_data, size * sizeof(cell));
		}else{
			array_data = nullptr;
		}
	}else{
		cell_value = obj.cell_value;
	}
}

dyn_object::dyn_object(dyn_object &&obj) : dimension(obj.dimension), tag(obj.tag)
{
	if(is_array())
	{
		array_data = obj.array_data;
		obj.array_data = nullptr;
	}else{
		cell_value = obj.cell_value;
	}
}

bool dyn_object::tag_assignable(AMX *amx, cell tag_id) const
{
	tag_ptr test_tag = tags::find_tag(amx, tag_id);
	if(test_tag->uid == tags::tag_cell)
	{
		if(!tag->strong()) return true;
	}
	return tag->inherits_from(test_tag);
}

bool dyn_object::tag_assignable(tag_ptr test_tag) const
{
	if(test_tag->uid == tags::tag_cell)
	{
		if(!tag->strong()) return true;
	}
	return tag->inherits_from(test_tag);
}

cell dyn_object::data_size() const
{
	switch(dimension)
	{
		case 0:
			return 1;
		default:
			return array_data == nullptr ? 0 : array_data[0];
	}
}

cell dyn_object::array_start() const
{
	if(is_array())
	{
		cell *b = array_data + 1;
		auto dim = dimension;
		while(dim > 1)
		{
			b = (cell*)((char*)b + *b);
			dim--;
		}
		return b - array_data;
	}else{
		return 0;
	}
}

bool dyn_object::get_cell(cell index, cell &value) const
{
	if(index < 0) return false;
	const cell *ptr = begin() + index;
	if(ptr >= end()) return false;
	value = *ptr;
	return true;
}

cell dyn_object::get_array(cell *arr, cell maxsize) const
{
	const cell *ptr = begin();
	cell length = end() - ptr;
	if(length > maxsize) length = maxsize;
	std::memcpy(arr, ptr, length * sizeof(cell));
	return length;
}

bool dyn_object::set_cell(cell index, cell value)
{
	if(index < 0) return false;
	cell *ptr = begin() + index;
	if(ptr >= end()) return false;
	*ptr = value;
	return true;
}

cell dyn_object::get_tag(AMX *amx) const
{
	return tag->get_id(amx);
}

cell dyn_object::store(AMX *amx) const
{
	if(is_array())
	{
		cell size = data_size() - 1;
		cell amx_addr, *addr;
		amx_Allot(amx, size, &amx_addr, &addr);
		std::memcpy(addr, array_data + 1, size * sizeof(cell));
		return amx_addr;
	}else{
		return cell_value;
	}
}

void dyn_object::load(AMX *amx, cell amx_addr)
{
	if(is_array())
	{
		cell size = data_size() - 1;
		cell *addr;
		amx_GetAddr(amx, amx_addr, &addr);
		std::memcpy(array_data + 1, addr, size * sizeof(cell));
	}
}

bool dyn_object::is_array() const
{
	return dimension > 0;
}

cell *dyn_object::begin()
{
	if(is_array())
	{
		return &array_data[array_start()];
	}else{
		return &cell_value;
	}
}

cell *dyn_object::end()
{
	if(is_array())
	{
		return array_data + data_size();
	}else{
		return &cell_value + 1;
	}
}

const cell *dyn_object::begin() const
{
	if(is_array())
	{
		return &array_data[array_start()];
	}else{
		return &cell_value;
	}
}

const cell *dyn_object::end() const
{
	if(is_array())
	{
		return array_data + data_size();
	}else{
		return &cell_value + 1;
	}
}

cell dyn_object::get_size() const
{
	if(is_array())
	{
		return end() - begin();
	}else{
		return 1;
	}
}

template <class TagType>
bool dyn_object::get_specifier_tagged(char &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	if(is_array())
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
	if(!ok && is_array())
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

namespace std
{
	template<>
	struct hash<cell_string>
	{
		size_t operator()(const cell_string &obj) const
		{
			size_t seed = 0;
			for(size_t i = 0; i < obj.size(); i++)
			{
				hash_combine(seed, obj[i]);
			}
			return seed;
		}
	};
}

template <class TagType>
bool dyn_object::get_hash_tagged(size_t &hash) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	for(auto it = begin(); it != end(); it++)
	{
		hash_combine(hash, *tag::conv_to(*it));
	}

	hash_combine(hash, tag::tag_uid);
	return true;
}

size_t dyn_object::get_hash() const
{
	size_t hash = 0;
	if(empty()) return 0;
	bool ok = get_hash_tagged<cell_string*>(hash) || get_hash_tagged<dyn_object*>(hash);
	if(ok) return hash;

	for(auto it = begin(); it != end(); it++)
	{
		hash_combine(hash, *it);
	}

	hash_combine(hash, tag->find_top_base());
	return hash;
}

bool dyn_object::empty() const
{
	return is_array() ? array_data == nullptr : false;
}

bool dyn_object::tag_compatible(const dyn_object &obj) const
{
	return tag->same_base(obj.tag);
}

tag_ptr dyn_object::get_tag() const
{
	return tag;
}

template <class TagType>
bool dyn_object::free_tagged() const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	for(auto it = begin(); it != end(); it++)
	{
		tag::free(tag::conv_to(*it));
	}

	return true;
}

void dyn_object::free() const
{
	free_tagged<cell_string*>() || free_tagged<dyn_object*>() ||
		free_tagged<std::vector<dyn_object>*>() || free_tagged<std::unordered_map<dyn_object, dyn_object>*>();
}

template <class TagType>
bool dyn_object::clone_tagged(dyn_object &copy) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	for(auto it = copy.begin(); it != copy.end(); it++)
	{
		auto val = tag::clone(tag::conv_to(*it));
		*it = tag::conv_from(val);
	}
	return true;
}

dyn_object dyn_object::clone() const
{
	dyn_object copy(*this);
	clone_tagged<cell_string*>(copy) || clone_tagged<dyn_object*>(copy) || clone_tagged<std::vector<dyn_object>*>(copy) || clone_tagged<std::unordered_map<dyn_object, dyn_object>*>(copy);
	return copy;
}

cell &dyn_object::operator[](cell index)
{
	return begin()[index];
}

const cell &dyn_object::operator[](cell index) const
{
	return begin()[index];
}

bool dyn_object::struct_compatible(const dyn_object &obj) const
{
	if(empty() && obj.empty()) return true;
	if(dimension != obj.dimension) return false;
	if(empty() || obj.empty()) return false;
	if(is_array())
	{
		cell ofs = array_start();
		if(ofs != obj.array_start()) return false;
		if(!memequal(array_data, obj.array_data, ofs * sizeof(cell))) return false;
	}
	return true;
}

template <class TagType>
bool dyn_object::operator_eq_tagged(const dyn_object &obj, bool &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	result = std::equal(begin(), end(), obj.begin(), obj.end(), [](cell v1, cell v2)
	{
		return *tag::conv_to(v1) == *tag::conv_to(v2);
	});
	return true;
}

bool operator==(const dyn_object &a, const dyn_object &b)
{
	if(!a.tag_compatible(b) || !a.struct_compatible(b)) return false;
	if(memequal(a.begin(), b.begin(), a.get_size() * sizeof(cell))) return true;
	bool result = false;
	a.operator_eq_tagged<cell_string*>(b, result) || a.operator_eq_tagged<dyn_object*>(b, result);
	return result;
}

bool operator!=(const dyn_object &a, const dyn_object &b)
{
	return !(a == b);
}

template <template <class T> class OpType, class TagType>
bool dyn_object::operator_func_tagged(const dyn_object &obj, dyn_object &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	OpType<TagType> op;
	if(is_array() && obj.is_array())
	{
		if(!struct_compatible(obj)) return true;

		result = dyn_object(*this);
		auto it = obj.begin();
		for(cell &c : result)
		{
			if(it == obj.end()) break;
			TagType val = op(tag::conv_to(c), tag::conv_to(*it));
			c = tag::conv_from(val);
			it++;
		}
	}else if(is_array())
	{
		TagType sval = tag::conv_to(obj.cell_value);

		result = dyn_object(*this);
		for(cell &c : result)
		{
			TagType val = op(tag::conv_to(c), sval);
			c = tag::conv_from(val);
		}
	}else if(obj.is_array())
	{
		TagType sval = tag::conv_to(cell_value);

		result = dyn_object(obj);
		for(cell &c : result)
		{
			TagType val = op(sval, tag::conv_to(c));
			c = tag::conv_from(val);
		}
	}else{
		TagType val = op(tag::conv_to(cell_value), tag::conv_to(obj.cell_value));
		result = {tag::conv_from(val), tags::find_tag(tag::tag_uid)};
	}
	return true;
}

template <template <class T> class OpType>
dyn_object dyn_object::operator_func(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || empty() || obj.empty()) return dyn_object();
	dyn_object result;
	operator_func_tagged<OpType, cell>(obj, result) || operator_func_tagged<OpType, float>(obj, result) || operator_func_tagged<OpType, cell_string*>(obj, result) || operator_func_tagged<OpType, dyn_object*>(obj, result);
	return result;
}

template <template <class T> class OpType, class TagType>
bool dyn_object::operator_func_tagged(dyn_object &result) const
{
	if(!tag->inherits_from(tag_traits<TagType>::tag_uid)) return false;
	typedef tag_traits<TagType> tag;

	OpType<TagType> op;
	result = dyn_object(*this);
	for(cell &c : result)
	{
		TagType val = op(tag::conv_to(c));
		c = tag::conv_from(c);
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
	op_strval<cell> cell_op;

	str = {};
	if(is_array())
	{
		str.append({'{'});
		bool first = true;
		for(const cell &c : *this)
		{
			if(first)
			{
				first = false;
			}else{
				str.append({',', ' '});
			}
			try{
				str.append(op(tag_traits<TagType>::conv_to(c)));
			}catch(int)
			{
				str.append(strings::convert(tag->name));
				str.append({':'});
				str.append(cell_op(tag_traits<cell>::conv_to(c)));
			}
		}
		str.append({'}'});
	}else{
		try{
			str.append(op(tag_traits<TagType>::conv_to(cell_value)));
		}catch(int)
		{
			str.append(strings::convert(tag->name));
			str.append({':'});
			str.append(cell_op(tag_traits<cell>::conv_to(cell_value)));
		}
	}
	return true;
}

template <>
bool dyn_object::to_string_tagged<char[]>(cell_string &str) const
{
	if(!tag->inherits_from(tag_traits<char[]>::tag_uid)) return false;

	if(dimension == 1)
	{
		str = cell_string(array_data + 1);
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
	if(is_array())
	{
		str.append({'{'});
		bool first = true;
		for(const cell &c : *this)
		{
			if(first)
			{
				first = false;
			}else{
				str.append({',', ' '});
			}
			str.append(op(tag_traits<cell>::conv_to(c)));
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
	if(is_array() && array_data != nullptr)
	{
		delete[] array_data;
	}
	dimension = obj.dimension;
	tag = obj.tag;
	if(is_array())
	{
		if(obj.array_data != nullptr)
		{
			cell size = obj.data_size();
			array_data = new cell[size];
			std::memcpy(array_data, obj.array_data, size * sizeof(cell));
		}else{
			array_data = nullptr;
		}
	}else{
		cell_value = obj.cell_value;
	}
	return *this;
}

dyn_object &dyn_object::operator=(dyn_object &&obj)
{
	if(this == &obj) return *this;
	if(is_array() && array_data != nullptr)
	{
		delete[] array_data;
	}
	dimension = obj.dimension;
	tag = obj.tag;
	if(is_array())
	{
		array_data = obj.array_data;
		obj.array_data = nullptr;
	} else {
		cell_value = obj.cell_value;
	}
	return *this;
}

dyn_object::~dyn_object()
{
	if(is_array() && array_data != nullptr)
	{
		delete[] array_data;
		array_data = nullptr;
	}
}
