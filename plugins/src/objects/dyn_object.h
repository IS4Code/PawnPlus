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

	dyn_object(const dyn_object &obj) : dyn_object(obj, true)
	{

	}

	dyn_object(dyn_object &&obj) noexcept : rank(obj.rank), tag(obj.tag)
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

	bool tag_assignable(tag_ptr test_tag) const;
	bool struct_compatible(const dyn_object &obj) const;
	bool get_cell(const cell *indices, cell num_indices, cell &value) const;
	bool set_cell(const cell *indices, cell num_indices, cell value);
	cell get_array(const cell *indices, cell num_indices, cell *arr, cell maxsize) const;
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
	std::weak_ptr<void> handle() const;
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

	bool get_cell(cell index, cell &value) const
	{
		return get_cell(&index, 1, value);
	}

	bool set_cell(cell index, cell value)
	{
		return set_cell(&index, 1, value);
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
		return is_array() ? array_data == nullptr || *array_data <= 1 : false;
	}

	bool is_array() const
	{
		return rank > 0;
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

	std::basic_string<cell> to_string() const;
	dyn_object operator+(const dyn_object &obj) const;
	dyn_object operator-(const dyn_object &obj) const;
	dyn_object operator*(const dyn_object &obj) const;
	dyn_object operator/(const dyn_object &obj) const;
	dyn_object operator%(const dyn_object &obj) const;
	dyn_object operator-() const;
	dyn_object inc() const;
	dyn_object dec() const;
	bool operator==(const dyn_object &obj) const;
	bool operator!=(const dyn_object &obj) const;
	bool operator<(const dyn_object &obj) const;
	bool operator>(const dyn_object &obj) const;
	bool operator<=(const dyn_object &obj) const;
	bool operator>=(const dyn_object &obj) const;
	bool operator!() const;
	dyn_object &operator=(const dyn_object &obj);
	dyn_object &operator=(dyn_object &&obj) noexcept;

	~dyn_object();

private:
	dyn_object(cell value, tag_ptr tag, bool assign) noexcept;
	dyn_object(const dyn_object &obj, bool assign);
	cell *begin();
	cell *end();
	const cell *begin() const;
	const cell *end() const;
	bool init_op();
	bool assign_op();
	bool assign_op(cell *start, cell size) const;
	template <cell(tag_operations::*OpFunc)(tag_ptr, cell, cell) const>
	dyn_object operator_func(const dyn_object &obj) const;
	template <cell(tag_operations::*OpFunc)(tag_ptr, cell) const>
	dyn_object operator_func() const;
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
}

#endif