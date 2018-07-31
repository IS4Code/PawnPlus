#ifndef TAGS_H_INCLUDED
#define TAGS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>

typedef const struct tag_info *tag_ptr;

class tag_operations
{
protected:
	tag_operations() = default;
public:
	virtual cell add(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell sub(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell mul(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell div(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell mod(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell neg(tag_ptr tag, cell a) const = 0;
	virtual std::basic_string<cell> to_string(tag_ptr tag, cell arg) const = 0;
	virtual std::basic_string<cell> to_string(tag_ptr tag, const cell *arg, cell size) const = 0;
	virtual bool equals(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool equals(tag_ptr tag, const cell *a, const cell *b, cell size) const = 0;
	virtual char format_spec(tag_ptr tag, bool arr) const = 0;
	virtual bool del(tag_ptr tag, cell arg) const = 0;
	virtual bool free(tag_ptr tag, cell arg) const = 0;
	virtual cell copy(tag_ptr tag, cell arg) const = 0;
	virtual cell clone(tag_ptr tag, cell arg) const = 0;
	virtual size_t hash(tag_ptr tag, cell arg) const = 0;
	virtual ~tag_operations() = default;
};

struct tag_info
{
	cell uid;
	std::string name;
	tag_ptr base;

	tag_info(cell uid, std::string &&name, tag_ptr base) : uid(uid), name(std::move(name)), base(base)
	{

	}

	bool strong() const;
	bool inherits_from(cell parent) const;
	bool inherits_from(tag_ptr parent) const;
	tag_ptr find_top_base() const;
	bool same_base(tag_ptr tag) const;
	cell get_id(AMX *amx) const;
	const class tag_operations &get_ops() const;
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
	constexpr const cell tag_iterator = 9;
	constexpr const cell tag_ref = 10;
	constexpr const cell tag_task = 11;
	constexpr const cell tag_guard = 12;

	tag_ptr find_tag(const char *name, size_t sublen=-1);
	tag_ptr find_tag(AMX *amx, cell tag_id);
	tag_ptr find_tag(cell tag_uid);
}

#endif
