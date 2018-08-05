#define _SCL_SECURE_NO_WARNINGS
#include "dyn_object.h"
#include "modules/containers.h"
#include "../fixes/linux.h"
#include <cmath>
#include <string>
#include <cstring>
#include <type_traits>
#include <algorithm>

bool memequal(void const* ptr1, void const* ptr2, size_t size)
{
	return !std::memcmp(ptr1, ptr2, size);
}

dyn_object::dyn_object() : rank(1), array_data(nullptr), tag(tags::find_tag(tags::tag_cell))
{

}

dyn_object::dyn_object(AMX *amx, cell value, cell tag_id) : rank(0), cell_value(value), tag(tags::find_tag(amx, tag_id))
{
	init_op();
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell tag_id) : rank(1), tag(tags::find_tag(amx, tag_id))
{
	if(arr != nullptr)
	{
		array_data = new cell[size + 1];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
	}else{
		array_data = new cell[size + 1]();
	}
	array_data[0] = size + 1;
	init_op();
}

void find_array_end(AMX *amx, const cell *&ptr)
{
	if(!*ptr)
	{
		ptr++;
	}else{
		auto data = amx->base + ((AMX_HEADER*)amx->base)->dat;
		auto dat = (cell*)(data);
		auto hlw = (cell*)(data + amx->hlw);
		auto hea = (cell*)(data + amx->hea);
		auto stk = (cell*)(data + amx->stk);
		auto stp = (cell*)(data + amx->stp);

		bool on_dat = ptr >= dat && ptr < hlw;
		bool on_heap = ptr >= hlw && ptr < hea;
		bool on_stack = ptr >= stk && ptr < stp;

		while(true)
		{
			if(on_dat && ptr >= hlw) break;
			if(on_heap && ptr >= hea) break;
			if(on_stack && ptr >= stp) break;

			ptr++;
			if(!*ptr)
			{
				ptr++;
				break;
			}
		}
	}
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell tag_id) : rank(2), tag(tags::find_tag(amx, tag_id))
{
	if(arr != nullptr)
	{
		const cell *last = arr;
		if(size > 0)
		{
			last += size - 1;
		}else{
			last = (cell*)((char*)last + *last);
			last--;
		}
		last = (cell*)((char*)last + *last);
		if(size2 > 0)
		{
			last += size2;
		}else{
			find_array_end(amx, last);
		}
		cell length = last - arr;
		array_data = new cell[length + 1];
		std::memcpy(array_data + 1, arr, length * sizeof(cell));
		array_data[0] = length + 1;
	}else{
		cell length = size + size * size2;
		array_data = new cell[length + 1]();
		for(cell i = 0; i < size; i++)
		{
			array_data[1 + i] = (size + i * size2 - i) * sizeof(cell);
		}
		array_data[0] = length + 1;
	}
	init_op();
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, cell tag_id) : rank(3), tag(tags::find_tag(amx, tag_id))
{
	if(arr != nullptr)
	{
		const cell *last = arr;
		last = (cell*)((char*)last + *last);
		last = (cell*)((char*)last + *last);
		last--;
		last = (cell*)((char*)last + *last);
		if(size3 > 0)
		{
			last += size3;
		}else{
			find_array_end(amx, last);
		}
		cell length = last - arr;
		array_data = new cell[length + 1];
		std::memcpy(array_data + 1, arr, length * sizeof(cell));
		array_data[0] = length + 1;
	}else{
		cell length = size + size * size2 + size * size2 * size3;
		array_data = new cell[length + 1]();
		for(cell i = 0; i < size; i++)
		{
			array_data[1 + i] = (size + i * size2 - i) * sizeof(cell);
			for(cell j = 0; j < size2; j++)
			{
				cell ofs = size + i * size + j;
				array_data[1 + ofs] = (size + size * size2 + i * size2 * size3 + j * size2 - ofs) * sizeof(cell);
			}
		}
		array_data[0] = length + 1;
	}
	init_op();
}

dyn_object::dyn_object(AMX *amx, const cell *str) : rank(1), tag(tags::find_tag(tags::tag_char))
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
	std::memcpy(array_data + 1, str, size * sizeof(cell));
	array_data[size + 1] = 0;
}

dyn_object::dyn_object(cell value, tag_ptr tag) : dyn_object(value, tag, true)
{

}

dyn_object::dyn_object(cell value, tag_ptr tag, bool assign) : rank(0), cell_value(value), tag(tag)
{
	if(assign)
	{
		init_op();
	}
}

dyn_object::dyn_object(const cell *arr, cell size, tag_ptr tag) : rank(1), tag(tag)
{
	if(arr != nullptr)
	{
		array_data = new cell[size + 1];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
	} else {
		array_data = new cell[size + 1]();
	}
	array_data[0] = size + 1;
	init_op();
}

dyn_object::dyn_object(const dyn_object &obj) : dyn_object(obj, true)
{

}

dyn_object::dyn_object(const dyn_object &obj, bool assign) : rank(obj.rank), tag(obj.tag)
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
	if(assign)
	{
		assign_op();
	}
}

dyn_object::dyn_object(dyn_object &&obj) : rank(obj.rank), tag(obj.tag)
{
	if(is_array())
	{
		array_data = obj.array_data;
	}else{
		cell_value = obj.cell_value;
	}
	obj.array_data = nullptr;
	obj.rank = 1;
}

bool dyn_object::tag_assignable(AMX *amx, cell tag_id) const
{
	return tag_assignable(tags::find_tag(amx, tag_id));
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
	switch(rank)
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
		auto dim = rank;
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

bool array_bounds(const cell *data, cell &begin, cell &end, cell data_begin, cell data_end, cell index)
{
	if(index < 0) return false;
	cell size = end - begin;
	if(index >= size) return false;
	cell next = begin + index + 1;
	if(next == data_begin)
	{
		end = data_end;
	}else{
		end = next + data[next] / sizeof(cell);
	}
	begin = begin + index + data[begin + index] / sizeof(cell);
	return true;
}

bool dyn_object::get_cell(cell index, cell &value) const
{
	return get_cell(&index, 1, value);
}

bool dyn_object::get_cell(const cell *indices, cell num_indices, cell &value) const
{
	auto addr = get_cell_addr(indices, num_indices);
	if(addr)
	{
		value = *addr;
		assign_op(&value, 1);
		return true;
	}
	return false;
}

bool dyn_object::set_cell(cell index, cell value)
{
	return set_cell(&index, 1, value);
}

bool dyn_object::set_cell(const cell *indices, cell num_indices, cell value)
{
	auto addr = get_cell_addr(indices, num_indices);
	if(addr)
	{
		const auto &ops = tag->get_ops();
		ops.collect(tag, addr, 1);
		*addr = value;
		ops.assign(tag, addr, 1);
		return true;
	}
	return false;
}

cell dyn_object::get_array(cell *arr, cell maxsize) const
{
	return get_array(nullptr, 0, arr, maxsize);
}

cell dyn_object::get_array(const cell *indices, cell num_indices, cell *arr, cell maxsize) const
{
	const cell *block;
	cell begin, end;

	if(!is_array())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
		{
			block = &cell_value;
			begin = 0;
			end = 1;
		}
		return 0;
	}else{
		if(empty()) return 0;

		block = array_data + 1;
		cell data_begin = this->begin() - block, data_end = this->end() - block;
		begin = 0;
		end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
		for(cell i = 0; i < num_indices; i++)
		{
			cell index = indices[i];
			if(begin >= data_begin)
			{
				cell size = end - begin;
				if(index >= size) return false;
				begin += index;
				end = begin + 1;
			}else if(!array_bounds(block, begin, end, data_begin, data_end, index))
			{
				return false;
			}
		}
		if(end > begin)
		{
			while(begin < data_begin)
			{
				begin += block[begin] / sizeof(cell);
			}
			while(end < data_begin)
			{
				end += block[end] / sizeof(cell);
			}
			if(end == data_begin)
			{
				end = data_end;
			}
		}
	}
	cell size = end - begin;
	if(maxsize > size) maxsize = size;
	std::memcpy(arr, block + begin, maxsize * sizeof(cell));
	assign_op(arr, maxsize);
	return size;
}

cell *dyn_object::get_cell_addr(const cell *indices, cell num_indices)
{
	if(!is_array())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
		{
			return &cell_value;
		}
		return nullptr;
	}
	if(empty()) return nullptr;

	cell *block = array_data + 1;
	cell data_begin = begin() - block, data_end = end() - block;
	cell begin = 0, end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
	for(cell i = 0; i < num_indices; i++)
	{
		cell index = indices[i];
		if(begin >= data_begin)
		{
			cell size = end - begin;
			if(index >= size) return nullptr;
			begin += index;
			end = begin + 1;
		}else if(!array_bounds(block, begin, end, data_begin, data_end, index))
		{
			return nullptr;
		}
	}
	while(begin < data_begin)
	{
		if(!array_bounds(block, begin, end, data_begin, data_end, 0))
		{
			return nullptr;
		}
	}
	if(end <= begin) return nullptr;
	return &block[begin];
}

const cell *dyn_object::get_cell_addr(const cell *indices, cell num_indices) const
{
	if(!is_array())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
		{
			return &cell_value;
		}
		return nullptr;
	}
	if(empty()) return nullptr;

	const cell *block = array_data + 1;
	cell data_begin = begin() - block, data_end = end() - block;
	cell begin = 0, end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
	for(cell i = 0; i < num_indices; i++)
	{
		cell index = indices[i];
		if(begin >= data_begin)
		{
			cell size = end - begin;
			if(index >= size) return nullptr;
			begin += index;
			end = begin + 1;
		}else if(!array_bounds(block, begin, end, data_begin, data_end, index))
		{
			return nullptr;
		}
	}
	while(begin < data_begin)
	{
		if(!array_bounds(block, begin, end, data_begin, data_end, 0))
		{
			return nullptr;
		}
	}
	if(end <= begin) return nullptr;
	return &block[begin];
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

		/*cell begin = array_start() - 1;
		assign_op(addr + begin, data_size() - begin);*/
		return amx_addr;
	}else{
		cell value = cell_value;
		assign_op(&value, 1);
		return value;
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

		assign_op();
	}
}

bool dyn_object::is_array() const
{
	return rank > 0;
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

cell dyn_object::array_size() const
{
	if(is_array())
	{
		return end() - begin();
	} else {
		return 1;
	}
}

cell dyn_object::get_size() const
{
	return get_size(nullptr, 0);
}

cell dyn_object::get_size(const cell *indices, cell num_indices) const
{
	if(!is_array()) return std::all_of(indices, indices + num_indices, [](const cell &i){return i == 0;}) ? 1 : 0;
	if(empty()) return 0;

	const cell *block = array_data + 1;
	cell data_begin = begin() - block, data_end = end() - block;
	cell begin = 0, end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
	bool cells = false;
	for(cell i = 0; i < num_indices; i++)
	{
		cell index = indices[i];
		if(cells)
		{
			if(index > 0) return 0;
		}else if(begin >= data_begin)
		{
			cell size = end - begin;
			if(index >= size) return 0;
			cells = true;
			continue;
		}else if(!array_bounds(block, begin, end, data_begin, data_end, index))
		{
			return 0;
		}
	}
	if(cells) return 1;
	return end - begin;
}

char dyn_object::get_specifier() const
{
	return tag->get_ops().format_spec(tag, is_array());
}

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t dyn_object::get_hash() const
{
	size_t hash = 0;
	if(empty()) return 0;

	const tag_operations &ops = tag->get_ops();
	for(auto it = begin(); it != end(); it++)
	{
		hash_combine(hash, ops.hash(tag, *it));
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

void dyn_object::free() const
{
	const auto &ops = tag->get_ops();
	for(auto it = begin(); it != end(); it++)
	{
		ops.free(tag, *it);
	}
}

dyn_object dyn_object::clone() const
{
	dyn_object copy(*this);
	const auto &ops = tag->get_ops();
	for(auto it = copy.begin(); it != copy.end(); it++)
	{
		*it = ops.clone(tag, *it);
	}
	return copy;
}

dyn_object dyn_object::call_op(op_type type, cell *args, size_t numargs, bool wrap) const
{
	dyn_object result = dyn_object(*this, false);
	const auto &ops = tag->get_ops();
	cell *args_copy = new cell[numargs + 1];
	std::memcpy(args_copy + 1, args, numargs * sizeof(cell));
	numargs += 1;
	for(cell &c : result)
	{
		args_copy[0] = c;
		c = ops.call_op(tag, type, args_copy, numargs);
	}
	delete[] args_copy;
	if(!wrap)
	{
		result.tag = tags::find_tag(tags::tag_cell);
	}
	return result;
}

bool dyn_object::init_op()
{
	if(!empty())
	{
		const auto &ops = tag->get_ops();
		cell *begin = this->begin();
		return ops.init(tag, begin, end() - begin);
	}
	return false;
}

bool dyn_object::assign_op()
{
	if(!empty())
	{
		cell *begin = this->begin();
		return assign_op(begin, end() - begin);
	}
	return false;
}

bool dyn_object::assign_op(cell *start, cell size) const
{
	const auto &ops = tag->get_ops();
	return ops.assign(tag, start, size);
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
	if(rank != obj.rank) return false;
	if(empty() || obj.empty()) return false;
	if(is_array())
	{
		cell ofs = array_start();
		if(ofs != obj.array_start()) return false;
		if(!memequal(array_data, obj.array_data, ofs * sizeof(cell))) return false;
	}
	return true;
}

bool dyn_object::operator==(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || !struct_compatible(obj)) return false;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	if(end2 - begin2 != end1 - begin1)
	{
		return false;
	}else{
		const auto &ops = tag->get_ops();
		return ops.eq(tag, begin1, begin2, end1 - begin1);
	}
}

bool dyn_object::operator!=(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || !struct_compatible(obj)) return true;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	if(end2 - begin2 != end1 - begin1)
	{
		return true;
	}else{
		const auto &ops = tag->get_ops();
		return ops.neq(tag, begin1, begin2, end1 - begin1);
	}
}

template <bool(tag_operations::*OpFunc)(tag_ptr, cell, cell) const>
bool dyn_object::operator_log_func(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || !struct_compatible(obj)) return false;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	if(end2 - begin2 != end1 - begin1)
	{
		return true;
	}else{
		const auto &ops = tag->get_ops();
		cell size = end1 - begin1;
		for(cell i = 0; i < size; i++)
		{
			if(!(ops.*OpFunc)(tag, begin1[i], begin2[i]))
			{
				return false;
			}
		}
		return true;
	}
}

bool dyn_object::operator<(const dyn_object &obj) const
{
	return operator_log_func<&tag_operations::lt>(obj);
}

bool dyn_object::operator>(const dyn_object &obj) const
{
	return operator_log_func<&tag_operations::gt>(obj);
}

bool dyn_object::operator<=(const dyn_object &obj) const
{
	return operator_log_func<&tag_operations::lte>(obj);
}

bool dyn_object::operator>=(const dyn_object &obj) const
{
	return operator_log_func<&tag_operations::gte>(obj);
}

bool dyn_object::operator!() const
{
	if(empty()) return true;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const auto &ops = tag->get_ops();
	cell size = end1 - begin1;
	for(cell i = 0; i < size; i++)
	{
		if(!ops.not(tag, begin1[i])) return false;
	}
	return true;
}


template <cell (tag_operations::*OpFunc)(tag_ptr, cell, cell) const>
dyn_object dyn_object::operator_func(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || empty() || obj.empty()) return dyn_object();
	dyn_object result;
	const auto &ops = tag->get_ops();
	if(is_array() && obj.is_array())
	{
		if(!struct_compatible(obj)) return dyn_object();

		result = dyn_object(*this, false);
		auto it = obj.begin();
		for(cell &c : result)
		{
			if(it == obj.end()) break;
			c = (ops.*OpFunc)(tag, c, *it);
			it++;
		}
	}else if(is_array())
	{
		result = dyn_object(*this, false);
		for(cell &c : result)
		{
			c = (ops.*OpFunc)(tag, c, obj.cell_value);
		}
	}else if(obj.is_array())
	{
		result = dyn_object(obj, false);
		for(cell &c : result)
		{
			c = (ops.*OpFunc)(tag, cell_value, c);
		}
	}else{
		result = dyn_object((ops.*OpFunc)(tag, cell_value, obj.cell_value), tag, false);
	}
	return result;
}

dyn_object dyn_object::operator+(const dyn_object &obj) const
{
	return operator_func<&tag_operations::add>(obj);
}

dyn_object dyn_object::operator-(const dyn_object &obj) const
{
	return operator_func<&tag_operations::sub>(obj);
}

dyn_object dyn_object::operator*(const dyn_object &obj) const
{
	return operator_func<&tag_operations::mul>(obj);
}

dyn_object dyn_object::operator/(const dyn_object &obj) const
{
	return operator_func<&tag_operations::div>(obj);
}

dyn_object dyn_object::operator%(const dyn_object &obj) const
{
	return operator_func<&tag_operations::mod>(obj);
}

template <cell(tag_operations::*OpFunc)(tag_ptr, cell) const>
dyn_object dyn_object::operator_func() const
{
	dyn_object result = dyn_object(*this, false);
	const auto &ops = tag->get_ops();
	for(cell &c : result)
	{
		c = (ops.*OpFunc)(tag, c);
	}
	return result;
}

dyn_object dyn_object::operator-() const
{
	return operator_func<&tag_operations::neg>();
}

std::basic_string<cell> dyn_object::to_string() const
{
	const auto &ops = tag->get_ops();
	if(is_array())
	{
		const cell *begin = this->begin();
		return ops.to_string(tag, begin, end() - begin);
	}else{
		return ops.to_string(tag, cell_value);
	}
}

dyn_object &dyn_object::operator=(const dyn_object &obj)
{
	if(this == &obj) return *this;
	if(is_array() && array_data != nullptr)
	{
		delete[] array_data;
	}
	rank = obj.rank;
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
	assign_op();
	return *this;
}

dyn_object &dyn_object::operator=(dyn_object &&obj)
{
	if(this == &obj) return *this;
	if(is_array() && array_data != nullptr)
	{
		delete[] array_data;
	}
	rank = obj.rank;
	tag = obj.tag;
	if(is_array())
	{
		array_data = obj.array_data;
	}else{
		cell_value = obj.cell_value;
	}
	obj.array_data = nullptr;
	obj.rank = 1;
	return *this;
}

dyn_object::~dyn_object()
{
	if(!empty())
	{
		const tag_operations &ops = tag->get_ops();
		if(is_array())
		{
			cell *begin = this->begin();
			cell *end = this->end();
			cell *data = array_data;
			rank = 1;
			array_data = nullptr;
			ops.collect(tag, begin, end - begin);
			delete[] data;
		}else{
			cell value = cell_value;
			rank = 1;
			array_data = nullptr;
			ops.collect(tag, &value, 1);
		}
	}else{
		rank = 1;
		array_data = nullptr;
	}
}
