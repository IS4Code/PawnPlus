#ifndef TAG_OPS_H_INCLUDED
#define TAG_OPS_H_INCLUDED

#include "tags.h"

#include <memory>

namespace strings
{
	template <class Iter>
	struct num_parser;

	template <class Iter>
	struct format_info
	{
		cell type;
		Iter &fmt_begin;
		Iter &fmt_end;
		const num_parser<Iter> &parse_num;
		cell_string &target;
		const encoding &encoding;
	};
}

class tag_operations
{
protected:
	const cell tag_uid;

	tag_operations(cell tag_uid) : tag_uid(tag_uid)
	{

	}

public:
	cell get_tag_uid() const { return tag_uid; }
	cell call_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const;
	virtual cell call_dyn_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const = 0;

	virtual cell add(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell sub(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell mul(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell div(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell mod(tag_ptr tag, cell a, cell b) const = 0;
	virtual cell neg(tag_ptr tag, cell a) const = 0;
	virtual cell inc(tag_ptr tag, cell a) const = 0;
	virtual cell dec(tag_ptr tag, cell a) const = 0;
	virtual bool eq(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool eq(tag_ptr tag, const cell *a, const cell *b, cell size) const = 0;
	virtual bool neq(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool neq(tag_ptr tag, const cell *a, const cell *b, cell size) const = 0;
	virtual bool lt(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool gt(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool lte(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool gte(tag_ptr tag, cell a, cell b) const = 0;
	virtual bool not(tag_ptr tag, cell a) const = 0;

	virtual std::basic_string<cell> to_string(tag_ptr tag, cell arg, const strings::encoding &encoding) const = 0;
	virtual std::basic_string<cell> to_string(tag_ptr tag, const cell *arg, cell size, const strings::encoding &encoding) const = 0;
	virtual bool append_string(tag_ptr tag, cell arg, std::basic_string<cell> &str, const strings::encoding &encoding) const = 0;
	virtual char format_spec(tag_ptr tag, bool arr) const = 0;
	virtual bool del(tag_ptr tag, cell arg) const = 0;
	virtual bool release(tag_ptr tag, cell arg) const = 0;
	virtual bool acquire(tag_ptr tag, cell arg) const = 0;
	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const = 0;
	virtual bool collect(tag_ptr tag, const cell *arg, cell size) const = 0;
	virtual cell copy(tag_ptr tag, cell arg) const = 0;
	virtual cell clone(tag_ptr tag, cell arg) const = 0;
	virtual bool assign(tag_ptr tag, cell *arg, cell size) const = 0;
	virtual bool init(tag_ptr tag, cell *arg, cell size) const = 0;
	virtual size_t hash(tag_ptr tag, cell arg) const = 0;

protected:
	using it1_t = const cell*;
	using it2_t = std::basic_string<cell>::const_iterator;
	using it3_t = strings::aligned_const_char_iterator;
	using it4_t = strings::unaligned_const_char_iterator;

public:
	virtual bool format_base(tag_ptr tag, const cell *arg, const strings::format_info<it1_t> &info) const = 0;
	virtual bool format_base(tag_ptr tag, const cell *arg, const strings::format_info<it2_t> &info) const = 0;
	virtual bool format_base(tag_ptr tag, const cell *arg, const strings::format_info<it3_t> &info) const = 0;
	virtual bool format_base(tag_ptr tag, const cell *arg, const strings::format_info<it4_t> &info) const = 0;

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const = 0;
	virtual tag_ptr get_element() const = 0;
	virtual ~tag_operations() = default;

	static const tag_operations *from_specifier(cell specifier, bool &is_string);
	bool register_specifier(cell specifier, bool is_string = false, bool overwrite = false) const;
};

class tag_control
{
protected:
	tag_control() = default;
public:
	virtual bool set_op(op_type type, AMX *amx, const char *handler, const char *add_format, const cell *args, int numargs) = 0;
	virtual bool set_op(op_type type, cell(*handler)(void *cookie, const void *tag, cell *args, cell numargs), void *cookie) = 0;
	virtual bool set_op(op_type type, std::shared_ptr<const class expression> handler) = 0;
	virtual bool lock() = 0;
	virtual ~tag_control() = default;
};

#endif
