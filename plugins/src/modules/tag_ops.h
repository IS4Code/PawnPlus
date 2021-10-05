#ifndef TAG_OPS_H_INCLUDED
#define TAG_OPS_H_INCLUDED

#include "tags.h"

class tag_operations
{
protected:
	tag_operations() = default;
public:
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

	virtual std::basic_string<cell> to_string(tag_ptr tag, cell arg) const = 0;
	virtual std::basic_string<cell> to_string(tag_ptr tag, const cell *arg, cell size) const = 0;
	virtual void append_string(tag_ptr tag, cell arg, std::basic_string<cell> &str) const = 0;
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

	virtual void format_base(tag_ptr tag, const cell *arg, cell type, const cell *fmt_begin, const cell *fmt_end, std::basic_string<cell> &str) const = 0;
	virtual void format_base(tag_ptr tag, const cell *arg, cell type, std::basic_string<cell>::const_iterator fmt_begin, std::basic_string<cell>::const_iterator fmt_end, std::basic_string<cell> &str) const = 0;
	virtual void format_base(tag_ptr tag, const cell *arg, cell type, strings::aligned_const_char_iterator fmt_begin, strings::aligned_const_char_iterator fmt_end, std::basic_string<cell> &str) const = 0;
	virtual void format_base(tag_ptr tag, const cell *arg, cell type, strings::unaligned_const_char_iterator fmt_begin, strings::unaligned_const_char_iterator fmt_end, std::basic_string<cell> &str) const = 0;

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const = 0;
	virtual tag_ptr get_element() const = 0;
	virtual ~tag_operations() = default;
};

#endif
