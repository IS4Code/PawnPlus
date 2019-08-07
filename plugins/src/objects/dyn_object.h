#ifndef DYN_OBJECT_H_INCLUDED
#define DYN_OBJECT_H_INCLUDED

#include "modules/tags.h"
#include "sdk/amx/amx.h"
#include <memory>
#include <string>
#include <cstring>

class dyn_object
{
	unsigned char rank;
	union{
		cell cell_value;
		cell *array_data;
	};
	tag_ptr tag;

public:
	dyn_object() noexcept : rank(1), array_data(nullptr), tag(tags::find_tag(tags::tag_cell))
	{

	}

	dyn_object(AMX *amx, cell value, cell tag_id) noexcept : rank(0), cell_value(value), tag(tags::find_tag(amx, tag_id))
	{
		init_op();
	}

	dyn_object(AMX *amx, const cell *arr, cell size, cell tag_id);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell tag_id);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, cell tag_id);
	dyn_object(const cell *str);

	dyn_object(cell value, tag_ptr tag) noexcept : dyn_object(value, tag, true)
	{

	}

	dyn_object(const cell *arr, cell size, tag_ptr tag);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, tag_ptr tag);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, tag_ptr tag);

	dyn_object(const dyn_object &obj) : dyn_object(obj, true)
	{

	}

	dyn_object(const dyn_object &obj, tag_ptr new_tag) : dyn_object(obj, true)
	{
		tag = new_tag;
	}

	dyn_object(dyn_object &&obj) noexcept : rank(obj.rank), tag(obj.tag)
	{
		if(rank > 0)
		{
			array_data = obj.array_data;
		}else{
			cell_value = obj.cell_value;
		}
		obj.array_data = nullptr;
		obj.rank = 1;
	}

	bool tag_assignable(tag_ptr test_tag) const noexcept;
	bool struct_compatible(const dyn_object &obj) const noexcept;
	cell get_cell(const cell *indices, cell num_indices) const;
	void set_cell(const cell *indices, cell num_indices, cell value);
	cell set_cells(const cell *indices, cell num_indices, const cell *values, cell size);
	cell get_array(const cell *indices, cell num_indices, cell *arr, cell maxsize) const;
	cell *get_array(const cell *indices, cell num_indices, cell &size);
	const cell *get_array(const cell *indices, cell num_indices, cell &size) const;
	cell *get_cell_addr(const cell *indices, cell num_indices);
	const cell *get_cell_addr(const cell *indices, cell num_indices) const;
	cell store(AMX *amx) const;
	void load(AMX *amx, cell amx_addr);
	cell get_size(const cell *indices, cell num_indices) const;
	cell array_start() const;
	cell data_size() const;
	cell array_size() const;
	size_t get_hash() const;
	void acquire() const;
	void release() const;
	std::weak_ptr<const void> handle() const;
	dyn_object clone() const;
	dyn_object call_op(op_type type, cell *args, size_t numargs, bool wrap) const;

	bool tag_assignable(AMX *amx, cell tag_id) const
	{
		return tag_assignable(tags::find_tag(amx, tag_id));
	}

	bool tag_compatible(const dyn_object &obj) const
	{
		return (empty() && obj.empty()) || tag->same_base(obj.tag);
	}

	cell get_cell(cell index) const
	{
		return get_cell(&index, 1);
	}

	void set_cell(cell index, cell value)
	{
		set_cell(&index, 1, value);
	}

	cell set_cells(cell index, const cell *values, cell size)
	{
		return set_cells(&index, 1, values, size);
	}

	cell get_array(cell *arr, cell maxsize) const
	{
		return get_array(nullptr, 0, arr, maxsize);
	}
	
	tag_ptr get_tag() const
	{
		return tag;
	}

	cell get_tag(AMX *amx) const
	{
		return tag->get_id(amx);
	}

	cell get_size() const
	{
		return get_size(nullptr, 0);
	}

	bool empty() const
	{
		return rank > 0 ? array_data == nullptr || *array_data <= 1 : false;
	}

	bool is_null() const
	{
		return rank > 0 && array_data == nullptr;
	}

	bool is_array() const
	{
		return rank > 0 && array_data != nullptr;
	}

	bool is_cell() const
	{
		return rank == 0;
	}

	cell get_rank() const
	{
		return is_null() ? -1 : static_cast<cell>(rank);
	}

	char get_specifier() const
	{
		return tag->get_ops().format_spec(tag, is_array());
	}

	cell &operator[](cell index)
	{
		return begin()[index];
	}

	const cell &operator[](cell index) const
	{
		return begin()[index];
	}

	void swap(dyn_object &other) noexcept;

	cell *begin();
	cell *end();
	cell *data_begin();
	const cell *begin() const;
	const cell *end() const;
	const cell *data_begin() const;

	std::basic_string<cell> to_string() const;
	dyn_object operator+(const dyn_object &obj) const;
	dyn_object operator-(const dyn_object &obj) const;
	dyn_object operator*(const dyn_object &obj) const;
	dyn_object operator/(const dyn_object &obj) const;
	dyn_object operator%(const dyn_object &obj) const;
	dyn_object operator&(const dyn_object &obj) const;
	dyn_object operator|(const dyn_object &obj) const;
	dyn_object operator^(const dyn_object &obj) const;
	dyn_object operator>>(const dyn_object &obj) const;
	dyn_object operator<<(const dyn_object &obj) const;
	dyn_object operator-() const;
	dyn_object operator+() const;
	dyn_object operator~() const;
	dyn_object inc() const;
	dyn_object dec() const;
	bool operator==(const dyn_object &obj) const noexcept;
	bool operator!=(const dyn_object &obj) const noexcept;
	bool operator<(const dyn_object &obj) const noexcept;
	bool operator>(const dyn_object &obj) const noexcept;
	bool operator<=(const dyn_object &obj) const noexcept;
	bool operator>=(const dyn_object &obj) const noexcept;
	bool operator!() const noexcept;
	dyn_object &operator=(const dyn_object &obj);
	dyn_object &operator=(dyn_object &&obj) noexcept;

	~dyn_object();

private:
	dyn_object(cell value, tag_ptr tag, bool assign) noexcept;
	dyn_object(const dyn_object &obj, bool assign);
	bool init_op();
	bool init_op(cell *start, cell size) const;
	bool assign_op();
	bool assign_op(cell *start, cell size) const;
	bool collect_op();
	bool collect_op(cell *start, cell size) const;
	template <cell(tag_operations::*OpFunc)(tag_ptr, cell, cell) const>
	dyn_object operator_func(const dyn_object &obj) const;
	template <cell(tag_operations::*OpFunc)(tag_ptr, cell) const>
	dyn_object operator_func() const;
	template <class OpType>
	dyn_object operator_cell_func(const dyn_object &obj) const;
	template <class OpType>
	dyn_object operator_cell_func() const;
	template <bool(tag_operations::*OpFunc)(tag_ptr, cell, cell) const>
	bool operator_log_func(const dyn_object &obj) const;
};

namespace std
{
	template<>
	struct hash<dyn_object>
	{
		size_t operator()(const dyn_object &obj) const
		{
			return obj.get_hash();
		}
	};

	template <>
	inline void swap<dyn_object>(dyn_object &a, dyn_object &b) noexcept
	{
		a.swap(b);
	}
}

#endif