#define _SCL_SECURE_NO_WARNINGS
#include "dyn_object.h"
#include "errors.h"
#include "main.h"
#include "modules/containers.h"
#include "../fixes/linux.h"
#include <cmath>
#include <string>
#include <cstring>
#include <type_traits>
#include <algorithm>
#include <functional>

bool memequal(void const* ptr1, void const* ptr2, size_t size)
{
	return !std::memcmp(ptr1, ptr2, size);
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell tag_id) : rank(1), tag(tags::find_tag(amx, tag_id))
{
	if(size < 0)
	{
		amx_LogicError(errors::out_of_range, "size");
	}
	if(arr != nullptr)
	{
		array_data = new cell[size + 2];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
		array_data[size + 1] = 0;
	}else{
		array_data = new cell[size + 2]();
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
		auto data = amx_GetData(amx);
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

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell tag_id) : dyn_object(amx, arr, size, size2, tags::find_tag(amx, tag_id))
{

}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, tag_ptr tag) : rank(2), tag(tag)
{
	if(size < 0)
	{
		amx_LogicError(errors::out_of_range, "size");
	}
	if(size2 < 0)
	{
		amx_LogicError(errors::out_of_range, "size2");
	}
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
		array_data = new cell[length + 2];
		std::memcpy(array_data + 1, arr, length * sizeof(cell));
		array_data[length + 1] = 0;
		array_data[0] = length + 1;
	}else{
		cell length = size + size * size2;
		array_data = new cell[length + 2]();
		for(cell i = 0; i < size; i++)
		{
			array_data[1 + i] = (size + i * size2 - i) * sizeof(cell);
		}
		array_data[0] = length + 1;
	}
	init_op();
}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, cell tag_id) : dyn_object(amx, arr, size, size2, size3, tags::find_tag(amx, tag_id))
{

}

dyn_object::dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, tag_ptr tag) : rank(3), tag(tag)
{
	if(size < 0)
	{
		amx_LogicError(errors::out_of_range, "size");
	}
	if(size2 < 0)
	{
		amx_LogicError(errors::out_of_range, "size2");
	}
	if(size3 < 0)
	{
		amx_LogicError(errors::out_of_range, "size3");
	}
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
		array_data = new cell[length + 2];
		std::memcpy(array_data + 1, arr, length * sizeof(cell));
		array_data[length + 1] = 0;
		array_data[0] = length + 1;
	}else{
		cell length = size + size * size2 + size * size2 * size3;
		array_data = new cell[length + 2]();
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

dyn_object::dyn_object(const cell *str) : rank(1), tag(tags::find_tag(tags::tag_char))
{
	if(str == nullptr || !str[0])
	{
		array_data = new cell[3]{2, 0, 0};
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
	array_data = new cell[size + 3];
	array_data[0] = size + 2;
	std::memcpy(array_data + 1, str, size * sizeof(cell));
	array_data[size + 2] = 0;
	array_data[size + 1] = 0;
}

dyn_object::dyn_object(cell value, tag_ptr tag, bool assign) noexcept : rank(0), cell_value(value), tag(tag)
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
		array_data = new cell[size + 2];
		std::memcpy(array_data + 1, arr, size * sizeof(cell));
		array_data[size + 1] = 0;
	}else{
		array_data = new cell[size + 2]();
	}
	array_data[0] = size + 1;
	init_op();
}

dyn_object::dyn_object(const dyn_object &obj, bool assign) : rank(obj.rank), tag(obj.tag)
{
	if(rank > 0)
	{
		if(obj.array_data != nullptr)
		{
			cell size = obj.data_size();
			array_data = new cell[size + 1];
			std::memcpy(array_data, obj.array_data, size * sizeof(cell));
			array_data[size] = 0;
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

bool dyn_object::tag_assignable(tag_ptr test_tag) const noexcept
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
	if(is_cell() || is_null())
	{
		return 0;
	}else{
		cell *b = array_data + 1;
		auto dim = rank;
		while(dim > 1)
		{
			b = (cell*)((char*)b + *b);
			dim--;
		}
		return b - array_data;
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

cell dyn_object::get_cell(const cell *indices, cell num_indices) const
{
	auto addr = get_cell_addr(indices, num_indices);
	if(!addr)
	{
		amx_LogicError(errors::out_of_range, "offset");
	}
	cell value = *addr;
	assign_op(&value, 1);
	return value;
}

void dyn_object::set_cell(const cell *indices, cell num_indices, cell value)
{
	auto addr = get_cell_addr(indices, num_indices);
	if(!addr)
	{
		amx_LogicError(errors::out_of_range, "offset");
	}
	if(!is_array())
	{
		amx_LogicError(errors::operation_not_supported, "variant");
	}
	const auto &ops = tag->get_ops();
	ops.collect(tag, addr, 1);
	*addr = value;
	ops.assign(tag, addr, 1);
}

cell *dyn_object::get_array(const cell *indices, cell num_indices, cell &size)
{
	return const_cast<cell*>(static_cast<const dyn_object*>(this)->get_array(indices, num_indices, size));
}

const cell *dyn_object::get_array(const cell *indices, cell num_indices, cell &size) const
{
	const cell *block;
	cell begin, end;

	if(is_cell())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
		{
			size = 1;
			return &cell_value;
		}else if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0 || i == 1; }))
		{
			size = 0;
			return &cell_value + 1;
		}else{
			amx_LogicError(errors::out_of_range, "offset");
		}
	}else{
		if(empty())
		{
			if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
			{
				size = 0;
				return nullptr;
			}else{
				amx_LogicError(errors::out_of_range, "offset");
			}
		}

		block = array_data + 1;
		cell data_begin = this->begin() - block, data_end = this->end() - block;
		begin = 0;
		end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
		for(cell i = 0; i < num_indices; i++)
		{
			cell index = indices[i];
			if(index < 0)
			{
				amx_LogicError(errors::out_of_range, "offset");
			}
			if(begin >= data_begin)
			{
				cell size = end - begin;
				if(index > size)
				{
					amx_LogicError(errors::out_of_range, "offset");
				}
				begin += index;
			}else if(!array_bounds(block, begin, end, data_begin, data_end, index))
			{
				amx_LogicError(errors::out_of_range, "offset");
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
	size = end - begin;
	return block + begin;
}

cell dyn_object::get_array(const cell *indices, cell num_indices, cell *arr, cell maxsize) const
{
	cell size;
	const cell *addr = get_array(indices, num_indices, size);
	if(maxsize > size) maxsize = size;
	std::memcpy(arr, addr, maxsize * sizeof(cell));
	assign_op(arr, maxsize);
	return maxsize;
}

cell *dyn_object::get_cell_addr(const cell *indices, cell num_indices)
{
	return const_cast<cell*>(static_cast<const dyn_object*>(this)->get_cell_addr(indices, num_indices));
}

const cell *dyn_object::get_cell_addr(const cell *indices, cell num_indices) const
{
	if(is_cell())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i) {return i == 0; }))
		{
			return &cell_value;
		}
		return nullptr;
	}else if(empty())
	{
		return nullptr;
	}

	const cell *block = array_data + 1;
	cell data_begin = begin() - block, data_end = end() - block;
	cell begin = 0, end = rank >= 2 ? block[0] / sizeof(cell) : data_end;
	for(cell i = 0; i < num_indices; i++)
	{
		cell index = indices[i];
		if(index < 0)
		{
			return nullptr;
		}
		if(begin >= data_begin)
		{
			cell size = end - begin;
			if(index >= size)
			{
				return nullptr;
			}
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
	if(end <= begin)
	{
		return nullptr;
	}
	return &block[begin];
}

cell dyn_object::set_cells(const cell *indices, cell num_indices, const cell *values, cell maxsize)
{
	cell size;
	cell *addr = get_array(indices, num_indices, size);
	if(addr)
	{
		if(!is_array())
		{
			amx_LogicError(errors::operation_not_supported, "variant");
		}
		const auto &ops = tag->get_ops();
		if(maxsize > size)
		{
			maxsize = size;
		}
		ops.collect(tag, addr, maxsize);
		std::memcpy(addr, values, maxsize * sizeof(cell));
		ops.assign(tag, addr, maxsize);
		return maxsize;
	}
	return 0;
}

cell dyn_object::store(AMX *amx) const
{
	if(is_cell())
	{
		cell value = cell_value;
		assign_op(&value, 1);
		return value;
	}else{
		cell size = data_size() - 1;
		cell amx_addr, *addr;
		amx_AllotSafe(amx, size, &amx_addr, &addr);
		std::memcpy(addr, array_data + 1, size * sizeof(cell));

		cell begin = array_start() - 1;
		assign_op(addr + begin, size - begin);
		return amx_addr;
	}
}

void dyn_object::load(AMX *amx, cell amx_addr)
{
	if(is_array())
	{
		cell size = data_size() - 1;
		cell *addr = amx_GetAddrSafe(amx, amx_addr);
		std::memcpy(array_data + 1, addr, size * sizeof(cell));

		assign_op();
	}
}

cell *dyn_object::begin()
{
	if(is_cell())
	{
		return &cell_value;
	}else{
		return &array_data[array_start()];
	}
}

cell *dyn_object::end()
{
	if(is_cell())
	{
		return &cell_value + 1;
	}else{
		return array_data + data_size();
	}
}

cell *dyn_object::data_begin()
{
	if(is_cell())
	{
		return &cell_value;
	}else{
		return array_data + 1;
	}
}

const cell *dyn_object::begin() const
{
	if(is_cell())
	{
		return &cell_value;
	}else{
		return &array_data[array_start()];
	}
}

const cell *dyn_object::end() const
{
	if(is_cell())
	{
		return &cell_value + 1;
	}else{
		return array_data + data_size();
	}
}

const cell *dyn_object::data_begin() const
{
	if(is_cell())
	{
		return &cell_value;
	}else{
		return array_data + 1;
	}
}

cell dyn_object::array_size() const
{
	if(is_cell())
	{
		return 1;
	}else{
		return end() - begin();
	}
}

cell dyn_object::get_size(const cell *indices, cell num_indices) const
{
	if(is_cell())
	{
		if(std::all_of(indices, indices + num_indices, [](const cell &i){return i == 0;}))
		{
			return 1;
		}
		amx_LogicError(errors::out_of_range, "offset");
	}
	if(empty())
	{
		if(num_indices == 0)
		{
			return 0;
		}else{
			amx_LogicError(errors::out_of_range, "offset");
		}
	}

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
			amx_LogicError(errors::out_of_range, "offset");
		}
	}
	if(cells) return 1;
	return end - begin;
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

void dyn_object::acquire() const
{
	if(!empty())
	{
		const auto &ops = tag->get_ops();
		for(auto it = begin(); it != end(); it++)
		{
			ops.acquire(tag, *it);
		}
	}
}

std::weak_ptr<const void> dyn_object::handle() const
{
	std::weak_ptr<const void> handle;
	if(!empty())
	{
		const auto &ops = tag->get_ops();
		bool first = true;
		for(auto it = begin(); it != end(); it++)
		{
			if(first)
			{
				handle = ops.handle(tag, *it);
				first = false;
			}else{
				auto handle2 = ops.handle(tag, *it);
				if(handle.owner_before(handle2) || handle2.owner_before(handle))
				{
					return {};
				}
			}
		}
	}
	return handle;
}

void dyn_object::release() const
{
	if(!empty())
	{
		const auto &ops = tag->get_ops();
		for(auto it = begin(); it != end(); it++)
		{
			ops.release(tag, *it);
		}
	}
}

dyn_object dyn_object::clone() const
{
	dyn_object copy(*this);
	if(!empty())
	{
		const auto &ops = tag->get_ops();
		for(auto it = copy.begin(); it != copy.end(); it++)
		{
			*it = ops.clone(tag, *it);
		}
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
		cell *begin = this->begin();
		return init_op(begin, end() - begin);
	}
	return false;
}

bool dyn_object::init_op(cell *start, cell size) const
{
	const auto &ops = tag->get_ops();
	return ops.init(tag, start, size);
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

bool dyn_object::collect_op()
{
	if(!empty())
	{
		cell *begin = this->begin();
		return collect_op(begin, end() - begin);
	}
	return false;
}

bool dyn_object::collect_op(cell *start, cell size) const
{
	const auto &ops = tag->get_ops();
	return ops.collect(tag, start, size);
}

bool dyn_object::struct_compatible(const dyn_object &obj) const noexcept
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

bool dyn_object::operator==(const dyn_object &obj) const noexcept
{
	if(!tag_compatible(obj) || !struct_compatible(obj)) return false;
	if(empty()) return true;
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

bool dyn_object::operator!=(const dyn_object &obj) const noexcept
{
	if(!tag_compatible(obj) || !struct_compatible(obj)) return true;
	if(empty()) return false;
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

bool dyn_object::operator<(const dyn_object &obj) const noexcept
{
	cell this_tag = tag->find_top_base()->uid;
	cell obj_tag = obj.tag->find_top_base()->uid;
	if(this_tag < obj_tag) return true;
	if(this_tag > obj_tag) return false;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	const auto &ops = tag->get_ops();
	while(true)
	{
		if(begin2 == end2)
		{
			return false;
		}else if(begin1 == end1)
		{
			return true;
		}else if(ops.lt(tag, *begin1, *begin2))
		{
			return true;
		}else if((begin1 + 1 == end1 && begin2 + 1 == end2) || ops.lt(tag, *begin2, *begin1))
		{
			return false;
		}
		begin1++;
		begin2++;
	}
	return false;
}

bool dyn_object::operator>(const dyn_object &obj) const noexcept
{
	cell this_tag = tag->find_top_base()->uid;
	cell obj_tag = obj.tag->find_top_base()->uid;
	if(this_tag < obj_tag) return false;
	if(this_tag > obj_tag) return true;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	const auto &ops = tag->get_ops();
	while(true)
	{
		if(begin1 == end1 && begin2 == end2)
		{
			return false;
		}else if(begin1 == end1)
		{
			return false;
		}else if(begin2 == end2)
		{
			return true;
		}else if(ops.gt(tag, *begin1, *begin2))
		{
			return true;
		}else if((begin1 + 1 == end1 && begin2 + 1 == end2) || ops.gt(tag, *begin2, *begin1))
		{
			return false;
		}
		begin1++;
		begin2++;
	}
	return false;
}

bool dyn_object::operator<=(const dyn_object &obj) const noexcept
{
	cell this_tag = tag->find_top_base()->uid;
	cell obj_tag = obj.tag->find_top_base()->uid;
	if(this_tag < obj_tag) return true;
	if(this_tag > obj_tag) return false;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	const auto &ops = tag->get_ops();
	while(true)
	{
		if(begin1 == end1 && begin2 == end2)
		{
			return true;
		}else if(begin1 == end1)
		{
			return true;
		}else if(begin2 == end2)
		{
			return false;
		}else if(!ops.lte(tag, *begin1, *begin2))
		{
			return false;
		}
		begin1++;
		begin2++;
	}
	return true;
}

bool dyn_object::operator>=(const dyn_object &obj) const noexcept
{
	cell this_tag = tag->find_top_base()->uid;
	cell obj_tag = obj.tag->find_top_base()->uid;
	if(this_tag < obj_tag) return false;
	if(this_tag > obj_tag) return true;
	const cell *begin1 = begin();
	const cell *end1 = end();
	const cell *begin2 = obj.begin();
	const cell *end2 = obj.end();
	const auto &ops = tag->get_ops();
	while(true)
	{
		if(begin1 == end1 && begin2 == end2)
		{
			return true;
		}else if(begin1 == end1)
		{
			return false;
		}else if(begin2 == end2)
		{
			return true;
		}else if(!ops.gte(tag, *begin1, *begin2))
		{
			return false;
		}
		begin1++;
		begin2++;
	}
	return true;
}

bool dyn_object::operator!() const noexcept
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
	if(!tag_compatible(obj) || empty() || obj.empty()) amx_LogicError(errors::operation_not_supported, "variant");
	dyn_object result;
	const auto &ops = tag->get_ops();
	if(is_array() && obj.is_array())
	{
		if(!struct_compatible(obj)) amx_LogicError(errors::operation_not_supported, "variant");

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

template <class OpType>
dyn_object dyn_object::operator_cell_func(const dyn_object &obj) const
{
	if(!tag_compatible(obj) || !tag_assignable(tags::find_tag(tags::tag_cell)) || empty() || obj.empty()) amx_LogicError(errors::operation_not_supported, "variant");
	dyn_object result;
	OpType op;
	if(is_array() && obj.is_array())
	{
		if(!struct_compatible(obj)) amx_LogicError(errors::operation_not_supported, "variant");

		result = dyn_object(*this, false);
		auto it = obj.begin();
		for(cell &c : result)
		{
			if(it == obj.end()) break;
			c = op(c, *it);
			it++;
		}
	}else if(is_array())
	{
		result = dyn_object(*this, false);
		for(cell &c : result)
		{
			c = op(c, obj.cell_value);
		}
	}else if(obj.is_array())
	{
		result = dyn_object(obj, false);
		for(cell &c : result)
		{
			c = op(cell_value, c);
		}
	}else{
		result = dyn_object(op(cell_value, obj.cell_value), tag, false);
	}
	return result;
}

dyn_object dyn_object::operator&(const dyn_object &obj) const
{
	return operator_cell_func<std::bit_and<cell>>(obj);
}

dyn_object dyn_object::operator|(const dyn_object &obj) const
{
	return operator_cell_func<std::bit_or<cell>>(obj);
}

dyn_object dyn_object::operator^(const dyn_object &obj) const
{
	return operator_cell_func<std::bit_xor<cell>>(obj);
}

template <class Type>
struct bit_right_shift
{
	Type operator()(const Type &a, const Type &b)
	{
		return a >> b;
	}
};

template <class Type>
struct bit_left_shift
{
	Type operator()(const Type &a, const Type &b)
	{
		return a << b;
	}
};

dyn_object dyn_object::operator>>(const dyn_object &obj) const
{
	return operator_cell_func<bit_right_shift<cell>>(obj);
}

dyn_object dyn_object::operator<<(const dyn_object &obj) const
{
	return operator_cell_func<bit_left_shift<cell>>(obj);
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

dyn_object dyn_object::inc() const
{
	return operator_func<&tag_operations::inc>();
}

dyn_object dyn_object::dec() const
{
	return operator_func<&tag_operations::dec>();
}

template <class OpType>
dyn_object dyn_object::operator_cell_func() const
{
	if(!tag_assignable(tags::find_tag(tags::tag_cell)) || empty()) amx_LogicError(errors::operation_not_supported, "variant");
	dyn_object result = dyn_object(*this, false);
	OpType op;
	for(cell &c : result)
	{
		c = op(c);
	}
	return result;
}

dyn_object dyn_object::operator+() const
{
	if(!tag_assignable(tags::find_tag(tags::tag_cell)) || empty()) amx_LogicError(errors::operation_not_supported, "variant");
	return dyn_object(*this, false);
}

dyn_object dyn_object::operator~() const
{
	return operator_cell_func<std::bit_not<cell>>();
}

std::basic_string<cell> dyn_object::to_string() const
{
	const auto &ops = tag->get_ops();
	if(is_cell())
	{
		return ops.to_string(tag, cell_value);
	}else{
		const cell *begin = this->begin();
		return ops.to_string(tag, begin, end() - begin);
	}
}

dyn_object &dyn_object::operator=(const dyn_object &obj)
{
	if(this == &obj) return *this;
	collect_op();
	if(is_array())
	{
		delete[] array_data;
	}
	rank = obj.rank;
	tag = obj.tag;
	if(rank > 0)
	{
		if(obj.array_data != nullptr)
		{
			cell size = obj.data_size();
			array_data = new cell[size + 1];
			std::memcpy(array_data, obj.array_data, size * sizeof(cell));
			array_data[size] = 0;
		}else{
			array_data = nullptr;
		}
	}else{
		cell_value = obj.cell_value;
	}
	assign_op();
	return *this;
}

dyn_object &dyn_object::operator=(dyn_object &&obj) noexcept
{
	if(this == &obj) return *this;
	collect_op();
	if(is_array())
	{
		delete[] array_data;
	}
	rank = obj.rank;
	tag = obj.tag;
	if(rank > 0)
	{
		array_data = obj.array_data;
	}else{
		cell_value = obj.cell_value;
	}
	obj.array_data = nullptr;
	obj.rank = 1;
	return *this;
}

void dyn_object::swap(dyn_object &other) noexcept
{
	if(this != &other)
	{
		if(is_cell())
		{
			if(other.is_cell())
			{
				std::swap(cell_value, other.cell_value);
			}else{
				cell c = cell_value;
				array_data = other.array_data;
				other.cell_value = c;
			}
		}else if(other.is_cell())
		{
			cell c = other.cell_value;
			other.array_data = array_data;
			cell_value = c;
		}else{
			std::swap(array_data, other.array_data);
		}
		std::swap(rank, other.rank);
		std::swap(tag, other.tag);
	}
}

dyn_object::~dyn_object()
{
	if(!is_null())
	{
		if(is_array())
		{
			cell *begin = this->begin();
			cell *end = this->end();
			cell *data = array_data;
			rank = 1;
			array_data = nullptr;
			collect_op(begin, end - begin);
			delete[] data;
		}else{
			cell value = cell_value;
			rank = 1;
			array_data = nullptr;
			collect_op(&value, 1);
		}
	}
}
