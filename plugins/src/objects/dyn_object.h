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
	dyn_object();
	dyn_object(AMX *amx, cell value, cell tag_id);
	dyn_object(AMX *amx, const cell *arr, cell size, cell tag_id);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell tag_id);
	dyn_object(AMX *amx, const cell *arr, cell size, cell size2, cell size3, cell tag_id);
	dyn_object(AMX *amx, const cell *str);
	dyn_object(cell value, tag_ptr tag);
	dyn_object(const cell *arr, cell size, tag_ptr tag);
	dyn_object(const dyn_object &obj);
	dyn_object(dyn_object &&obj);

	bool tag_assignable(AMX *amx, cell tag_id) const;
	bool tag_assignable(tag_ptr test_tag) const;
	bool tag_compatible(const dyn_object &obj) const;
	bool struct_compatible(const dyn_object &obj) const;

	bool get_cell(cell index, cell &value) const;
	bool get_cell(const cell *indices, cell num_indices, cell &value) const;
	bool set_cell(cell index, cell value);
	bool set_cell(const cell *indices, cell num_indices, cell value);
	cell get_array(cell *arr, cell maxsize) const;
	cell get_array(const cell *indices, cell num_indices, cell *arr, cell maxsize) const;
	cell *get_cell_addr(const cell *indices, cell num_indices);
	const cell *get_cell_addr(const cell *indices, cell num_indices) const;
	cell store(AMX *amx) const;
	void load(AMX *amx, cell amx_addr);

	tag_ptr get_tag() const;
	cell get_tag(AMX *amx) const;
	cell get_size() const;
	cell get_size(const cell *indices, cell num_indices) const;
	bool empty() const;
	bool is_array() const;
	cell array_start() const;
	cell data_size() const;
	cell array_size() const;

	char get_specifier() const;
	size_t get_hash() const;
	void free() const;
	dyn_object clone() const;
	dyn_object call_op(op_type type, cell *args, size_t numargs, bool wrap) const;

	cell &operator[](cell index);
	const cell &operator[](cell index) const;
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
	dyn_object &operator=(dyn_object &&obj);

	~dyn_object();

private:
	dyn_object(cell value, tag_ptr tag, bool assign);
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