#ifndef TAGS_H_INCLUDED
#define TAGS_H_INCLUDED

#include "modules/strings.h"
#include "sdk/amx/amx.h"
#include <string>
#include <memory>

typedef const struct tag_info *tag_ptr;

enum class op_type
{
	add = 1,
	sub = 2,
	mul = 3,
	div = 4,
	mod = 5,
	neg = 6,
	inc = 7,
	dec = 8,

	eq = 10,
	neq = 11,
	lt = 12,
	gt = 13,
	lte = 14,
	gte = 15,
	not = 16,

	string = 20,
	del = 21,
	release = 22,
	collect = 23,
	copy = 24,
	clone = 25,
	assign = 26,
	init = 27,
	hash = 28,
	acquire = 29,
	handle = 30,
	format = 31
};

namespace std
{
	template <>
	struct hash<op_type>
	{
		size_t operator()(const op_type &t) const
		{
			return static_cast<size_t>(t);
		}
	};
}

class tag_operations;

class tag_control;

struct tag_info
{
	cell uid;
	std::string name;
	tag_ptr base;
	std::unique_ptr<tag_operations> ops;

	tag_info(cell uid, std::string &&name, tag_ptr base, std::unique_ptr<tag_operations> &&ops);

	bool strong() const;
	bool inherits_from(cell parent) const;
	bool inherits_from(tag_ptr parent) const;
	tag_ptr find_top_base() const;
	bool same_base(tag_ptr tag) const;
	cell get_id(AMX *amx) const;
	cell call_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const;
	const tag_operations &get_ops() const;
	tag_control *get_control() const;
	const char *format_name() const;
};

namespace tags
{
	constexpr const cell tag_unknown = 0;
	constexpr const cell tag_cell = 1;
	constexpr const cell tag_bool = 2;
	constexpr const cell tag_char = 3;
	constexpr const cell tag_float = 4;
	constexpr const cell tag_string = 5;
	constexpr const cell tag_variant = 6;
	constexpr const cell tag_list = 7;
	constexpr const cell tag_map = 8;
	constexpr const cell tag_iter = 9;
	constexpr const cell tag_ref = 10;
	constexpr const cell tag_task = 11;
	constexpr const cell tag_var = 12;
	constexpr const cell tag_linked_list = 13;
	constexpr const cell tag_guard = 14;
	constexpr const cell tag_callback_handler = 15;
	constexpr const cell tag_native_hook = 16;
	constexpr const cell tag_handle = 17;
	constexpr const cell tag_symbol = 18;
	constexpr const cell tag_signed = 19;
	constexpr const cell tag_unsigned = 20;
	constexpr const cell tag_pool = 21;
	constexpr const cell tag_expression = 22;
	constexpr const cell tag_address = 23;
	constexpr const cell tag_amx_guard = 24;

	tag_ptr find_tag(const char *name, size_t sublen=-1);
	tag_ptr find_tag(AMX *amx, cell tag_id);
	tag_ptr find_tag(cell tag_uid);
	tag_ptr find_existing_tag(const char *name, size_t sublen = -1);
	tag_ptr new_tag(const char *name, cell base_id);
}

#endif
