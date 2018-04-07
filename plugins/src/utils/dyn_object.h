#ifndef DYN_OBJECT_H_INCLUDED
#define DYN_OBJECT_H_INCLUDED

#include "sdk/amx/amx.h"
#include <memory>
#include <string>
#include <cstring>

class dyn_object
{
	bool is_array;
	union{
		cell cell_value;
		cell array_size;
	};
	std::unique_ptr<cell[]> array_value;
	std::string tag_name;
	std::string find_tag(AMX *amx, cell tag_id) const;

public:
	dyn_object();
	dyn_object(AMX *amx, cell value, cell tag_id);
	dyn_object(AMX *amx, cell *arr, cell size, cell tag_id);
	dyn_object(AMX *amx, cell *str);
	dyn_object(cell value, const char *tag);
	dyn_object(cell *arr, cell size, const char *tag);
	dyn_object(const dyn_object &obj);
	dyn_object(dyn_object &&obj);
	bool check_tag(AMX *amx, cell tag_id) const;
	bool get_cell(cell index, cell &value) const;
	cell get_array(cell *arr, cell maxsize) const;
	bool set_cell(cell index, cell value);
	cell get_tag(AMX *amx) const;
	cell store(AMX *amx) const;
	void load(AMX *amx, cell amx_addr);
	cell get_size() const;
	char get_specifier() const;
	size_t tag_hash() const;
	size_t get_hash() const;
	bool empty() const;
	bool tag_check(const dyn_object &obj) const;
	cell &operator[](cell index);
	const cell &operator[](cell index) const;
	friend bool operator==(const dyn_object &a, const dyn_object &b);
	std::basic_string<cell> to_string() const;
	dyn_object operator+(const dyn_object &obj) const;
	dyn_object operator-(const dyn_object &obj) const;
	dyn_object operator*(const dyn_object &obj) const;
	dyn_object operator/(const dyn_object &obj) const;
	dyn_object operator%(const dyn_object &obj) const;
	dyn_object &operator=(const dyn_object &obj);
	dyn_object &operator=(dyn_object &&obj);

private:
	template <template <class T> class OpType, class TagType>
	dyn_object operator_func_tagged(const dyn_object &obj) const;
	template <template <class T> class OpType>
	dyn_object operator_func(const dyn_object &obj) const;
	template <template <class T> class OpType, class TagType>
	dyn_object operator_func_tagged() const;
	template <template <class T> class OpType>
	dyn_object operator_func() const;
	template <class TagType>
	std::basic_string<cell> to_string_tagged() const;
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