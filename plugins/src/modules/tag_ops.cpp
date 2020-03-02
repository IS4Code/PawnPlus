#include "tags.h"
#include "main.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/containers.h"
#include "modules/tasks.h"
#include "modules/amxutils.h"
#include "modules/events.h"
#include "modules/amxhook.h"
#include "modules/expressions.h"
#include "objects/stored_param.h"
#include "fixes/linux.h"
#include "utils/optional.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

using cell_string = strings::cell_string;

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class Self>
struct null_operations : public tag_operations
{
	cell tag_uid;

	null_operations(cell tag_uid) : tag_uid(tag_uid)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		return 0;
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		return 0;
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		return 0;
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		return 0;
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return 0;
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return 0;
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		return 0;
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		return 0;
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool eq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		for(cell i = 0; i < size; i++)
		{
			if(!eq(tag, a[i], b[i]))
			{
				return false;
			}
		}
		return true;
	}

	virtual bool neq(tag_ptr tag, cell a, cell b) const override
	{
		return !eq(tag, a, b);
	}

	virtual bool neq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		for(cell i = 0; i < size; i++)
		{
			if(neq(tag, a[i], b[i]))
			{
				return true;
			}
		}
		return false;
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return false;
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return false;
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return false;
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return false;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		return a == 0;
	}

	virtual cell_string to_string(tag_ptr tag, cell arg) const override
	{
		cell_string str;
		if(tag->uid != tag_uid)
		{
			str.append(strings::convert(tag->format_name()));
			str.push_back(':');
		}
		append_string(tag, arg, str);
		return str;
	}

	virtual cell_string to_string(tag_ptr tag, const cell *arg, cell size) const override
	{
		cell_string str;
		if(tag->uid != tag_uid)
		{
			str.append(strings::convert(tag->format_name()));
			str.push_back(':');
		}
		str.push_back('{');
		bool first = true;
		for(cell i = 0; i < size; i++)
		{
			if(first)
			{
				first = false;
			}else{
				str.push_back(',');
				str.push_back(' ');
			}
			append_string(tag, arg[i], str);
		}
		str.push_back('}');
		return str;
	}
	
	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'i';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return false;
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		return {};
	}

	virtual bool collect(tag_ptr tag, const cell *arg, cell size) const override
	{
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}

	virtual bool assign(tag_ptr tag, cell *arg, cell size) const override
	{
		return false;
	}

	virtual bool init(tag_ptr tag, cell *arg, cell size) const override
	{
		return false;
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		return std::hash<cell>()(arg);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.append(strings::convert(tags::find_tag(tag_uid)->format_name()));
		str.push_back(':');
		str.append(strings::convert(std::to_string(arg)));
	}

	virtual cell call_dyn_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const override
	{
		return 0;
	}

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const override;

	virtual tag_ptr get_element() const override
	{
		return nullptr;
	}

	const char *get_subname(tag_ptr tag, const char *name) const
	{
		size_t len = tag->name.length();
		if(std::memcmp(name, tag->name.c_str(), len) == 0 && name[len] == '@')
		{
			return name + len + 1;
		}
		return nullptr;
	}

	virtual void format_base(tag_ptr tag, const cell *arg, const char *fmt_begin, const char *fmt_end, std::basic_string<cell> &str) const override
	{
		return static_cast<const Self*>(this)->format(tag, arg, fmt_begin, fmt_end, str);
	}

	virtual void format_base(tag_ptr tag, const cell *arg, std::basic_string<cell>::const_iterator fmt_begin, std::basic_string<cell>::const_iterator fmt_end, std::basic_string<cell> &str) const override
	{
		return static_cast<const Self*>(this)->format(tag, arg, fmt_begin, fmt_end, str);
	}

	virtual void format_base(tag_ptr tag, const cell *arg, strings::aligned_const_char_iterator fmt_begin, strings::aligned_const_char_iterator fmt_end, std::basic_string<cell> &str) const override
	{
		return static_cast<const Self*>(this)->format(tag, arg, fmt_begin, fmt_end, str);
	}

	virtual void format_base(tag_ptr tag, const cell *arg, strings::unaligned_const_char_iterator fmt_begin, strings::unaligned_const_char_iterator fmt_end, std::basic_string<cell> &str) const override
	{
		return static_cast<const Self*>(this)->format(tag, arg, fmt_begin, fmt_end, str);
	}

	template <class Iter>
	void format(tag_ptr tag, const cell *arg, Iter fmt_begin, Iter fmt_end, std::basic_string<cell> &str) const
	{
		
	}
};

template <class Self>
struct cell_operations : public null_operations<Self>
{
	cell_operations(cell tag_uid) : null_operations<Self>(tag_uid)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		return a + b;
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		return a - b;
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		return a * b;
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return a / b;
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return a % b;
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return -a;
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		return a+1;
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		return a-1;
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool eq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return !std::memcmp(a, b, size * sizeof(cell));
	}

	virtual bool neq(tag_ptr tag, cell a, cell b) const override
	{
		return a != b;
	}

	virtual bool neq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return !!std::memcmp(a, b, size * sizeof(cell));
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return a < b;
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return a > b;
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return a <= b;
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return a >= b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		return !a;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		return arg;
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.append(strings::convert(std::to_string(arg)));
	}
};

struct signed_operations : public cell_operations<signed_operations>
{
	signed_operations() : cell_operations(tags::tag_signed)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		cell res = b + a;
		if((b ^ a) >= 0 && (res ^ b) < 0)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		ucell res = a - b;
		if(res > static_cast<ucell>(a))
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		auto res = (int64_t)a * (int64_t)b;
		if((cell)res != res)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return static_cast<cell>(res);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return a / b;
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return a % b;
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return -a;
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		if(a == std::numeric_limits<cell>::max())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a + 1;
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		if(a == std::numeric_limits<cell>::min())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return a - 1;
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return a < b;
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return a > b;
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return a <= b;
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return a >= b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		return !a;
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.append(strings::convert(tags::find_tag(tag_uid)->format_name()));
		str.push_back(':');
		str.append(strings::convert(std::to_string(arg)));
	}
};

struct unsigned_operations : public cell_operations<unsigned_operations>
{
	unsigned_operations() : cell_operations(tags::tag_unsigned)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		ucell res = b + a;
		if(res < static_cast<ucell>(b))
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		ucell res = a - b;
		if(res > static_cast<ucell>(a))
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return res;
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		auto res = (uint64_t)(ucell)a * (uint64_t)(ucell)b;
		if(res > 0xFFFFFFFF)
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return static_cast<ucell>(res);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return static_cast<ucell>(a) / static_cast<ucell>(b);
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) throw errors::amx_error(AMX_ERR_DIVIDE);
		return static_cast<ucell>(a) % static_cast<ucell>(b);
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return -a;
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		if(static_cast<ucell>(a) == std::numeric_limits<ucell>::max())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return static_cast<ucell>(a) + 1;
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		if(static_cast<ucell>(a) == std::numeric_limits<ucell>::min())
		{
			throw errors::amx_error(AMX_ERR_DOMAIN);
		}
		return static_cast<ucell>(a) - 1;
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return static_cast<ucell>(a) == static_cast<ucell>(b);
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return static_cast<ucell>(a) < static_cast<ucell>(b);
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return static_cast<ucell>(a) > static_cast<ucell>(b);
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return static_cast<ucell>(a) <= static_cast<ucell>(b);
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return static_cast<ucell>(a) >= static_cast<ucell>(b);
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		return !static_cast<ucell>(a);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.append(strings::convert(tags::find_tag(tag_uid)->format_name()));
		str.push_back(':');
		str.append(strings::convert(std::to_string(static_cast<ucell>(arg))));
	}
};

struct bool_operations : public cell_operations<bool_operations>
{
	bool_operations() : cell_operations(tags::tag_bool)
	{

	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		static auto str_true = strings::convert("true");
		static auto str_false = strings::convert("false");
		static auto str_bool = strings::convert("bool:");

		if(arg == 1)
		{
			str.append(str_true);
		}else if(arg == 0)
		{
			str.append(str_false);
		}else{
			if(tag->uid == tag_uid)
			{
				str.append(str_bool);
			}
			str.append(strings::convert(std::to_string(arg)));
		}
	}
};

struct char_operations : public cell_operations<char_operations>
{
	char_operations() : cell_operations(tags::tag_char)
	{

	}

	virtual cell_string to_string(tag_ptr tag, cell arg) const override
	{
		cell_string str;
		append_string(tag, arg, str);
		return str;
	}

	virtual cell_string to_string(tag_ptr tag, const cell *arg, cell size) const override
	{
		cell_string str(arg, size);
		size_t null = str.find(0);
		if(null != cell_string::npos)
		{
			str.resize(null, 0);
		}
		return str;
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 's' : 'c';
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.push_back(arg);
	}
};

struct float_operations : public cell_operations<float_operations>
{
	float_operations() : cell_operations(tags::tag_float)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		float result = amx_ctof(a) + amx_ctof(b);
		return amx_ftoc(result);
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		float result = amx_ctof(a) - amx_ctof(b);
		return amx_ftoc(result);
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		float result = amx_ctof(a) * amx_ctof(b);
		return amx_ftoc(result);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		float result = amx_ctof(a) / amx_ctof(b);
		return amx_ftoc(result);
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		float result = std::fmod(amx_ctof(a), amx_ctof(b));
		return amx_ftoc(result);
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		float result = -amx_ctof(a);
		return amx_ftoc(result);
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		float result = amx_ctof(a) + 1.0f;
		return amx_ftoc(result);
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		float result = amx_ctof(a) - 1.0f;
		return amx_ftoc(result);
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) == amx_ctof(b);
	}

	virtual bool eq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return null_operations::eq(tag, a, b, size);
	}

	virtual bool neq(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) != amx_ctof(b);
	}

	virtual bool neq(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return null_operations::neq(tag, a, b, size);
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) < amx_ctof(b);
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) > amx_ctof(b);
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) <= amx_ctof(b);
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) >= amx_ctof(b);
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		return !amx_ctof(a);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'f';
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.append(strings::convert(std::to_string(amx_ctof(arg))));
	}
};

struct string_operations : public null_operations<string_operations>
{
	string_operations() : null_operations(tags::tag_string)
	{

	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		cell_string *str1, *str2;
		if((!strings::pool.get_by_id(a, str1) && str1 != nullptr) || (!strings::pool.get_by_id(b, str2) && str2 != nullptr))
		{
			return 0;
		}
		if(str1 == nullptr && str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		if(str1 == nullptr)
		{
			return strings::pool.get_id(strings::pool.emplace(*str2));
		}
		if(str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.emplace(*str1));
		}

		auto str = *str1 + *str2;
		return strings::pool.get_id(strings::pool.add(std::move(str)));
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return add(tag, a, b);
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(a, str1) && str1 != nullptr) return false;
		cell_string *str2;
		if(!strings::pool.get_by_id(b, str2) && str2 != nullptr) return false;

		if(str1 == nullptr && str2 == nullptr) return true;
		if(str1 == nullptr)
		{
			return str2->size() == 0;
		}
		if(str2 == nullptr)
		{
			return str1->size() == 0;
		}
		return *str1 == *str2;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		cell_string *str;
		return !strings::pool.get_by_id(a, str);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'S';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return strings::pool.remove_by_id(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(arg, str)) return false;
		if(!strings::pool.release_ref(*str)) return false;
		return true;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(arg, str)) return false;
		if(!strings::pool.acquire_ref(*str)) return false;
		return true;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		cell_string *str;
		if(strings::pool.get_by_id(arg, str))
		{
			return strings::pool.get_id(strings::pool.emplace(*str));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		cell_string *str;
		if(strings::pool.get_by_id(arg, str))
		{
			size_t seed = 0;
			for(size_t i = 0; i < str->size(); i++)
			{
				hash_combine(seed, (*str)[i]);
			}
			return seed;
		}
		return null_operations::hash(tag, arg);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		cell_string *ptr;
		if(strings::pool.get_by_id(arg, ptr))
		{
			str.append(*ptr);
		}
	}


	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<cell_string> ptr;
		if(strings::pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};

struct variant_operations : public null_operations<variant_operations>
{
	variant_operations() : null_operations<variant_operations>(tags::tag_variant)
	{

	}

	template <dyn_object(dyn_object::*op_type)(const dyn_object&) const>
	cell bin_op(tag_ptr tag, cell a, cell b) const
	{
		dyn_object *var1;
		if(!variants::pool.get_by_id(a, var1)) return 0;
		dyn_object *var2;
		if(!variants::pool.get_by_id(b, var2)) return 0;
		auto var = (var1->*op_type)(*var2);
		return variants::create(std::move(var));
	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		return bin_op<&dyn_object::operator+>(tag, a, b);
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		return bin_op<&dyn_object::operator- >(tag, a, b);
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		return bin_op<&dyn_object::operator*>(tag, a, b);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		return bin_op<&dyn_object::operator/>(tag, a, b);
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return bin_op<&dyn_object::operator% >(tag, a, b);
	}

	template <dyn_object(dyn_object::*op_type)() const>
	cell un_op(tag_ptr tag, cell a) const
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(a, var)) return 0;
		auto result = (var->*op_type)();
		return variants::create(std::move(result));
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return un_op<&dyn_object::operator- >(tag, a);
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		return un_op<&dyn_object::inc>(tag, a);
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		return un_op<&dyn_object::dec>(tag, a);
	}

	template <bool(dyn_object::*op)(const dyn_object&) const>
	bool AMX_NATIVE_CALL log_op(tag_ptr tag, cell a, cell b) const
	{
		dyn_object *var1, *var2;
		if((!variants::pool.get_by_id(a, var1) && var1 != nullptr) || (!variants::pool.get_by_id(b, var2) && var2 != nullptr)) return false;
		bool init1 = false, init2 = false;
		if(var1 == nullptr)
		{
			var1 = new dyn_object();
			init1 = true;
		}
		if(var2 == nullptr)
		{
			var2 = new dyn_object();
			init2 = false;
		}
		bool result = (var1->*op)(*var2);
		if(init1)
		{
			delete var1;
		}
		if(init2)
		{
			delete var2;
		}
		return result;
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<&dyn_object::operator==>(tag, a, b);
	}

	virtual bool neq(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<&dyn_object::operator!=>(tag, a, b);
	}

	virtual bool lt(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<&dyn_object::operator<>(tag, a, b);
	}

	virtual bool gt(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<(&dyn_object::operator>)>(tag, a, b);
	}

	virtual bool lte(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<&dyn_object::operator<=>(tag, a, b);
	}

	virtual bool gte(tag_ptr tag, cell a, cell b) const override
	{
		return log_op<&dyn_object::operator>=>(tag, a, b);
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(a, var)) return true;
		return !(*var);
	}
	
	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'V';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return variants::pool.remove_by_id(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(arg, var)) return false;
		if(!variants::pool.release_ref(*var)) return false;
		return true;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		decltype(variants::pool)::ref_container *var;
		if(!variants::pool.get_by_id(arg, var)) return false;
		if(!variants::pool.acquire_ref(*var)) return false;
		return true;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			return variants::pool.get_id(variants::pool.emplace(*var));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			dyn_object tmp;
			std::swap(*var, tmp);
			cell clone = variants::pool.get_id(variants::pool.add(tmp.clone()));
			std::swap(*var, tmp);
			return clone;
		}
		return 0;
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			return var->get_hash();
		}
		return null_operations::hash(tag, arg);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.push_back('(');
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			str.append(var->to_string());
		}
		str.push_back(')');
	}

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const override
	{
		return std::make_unique<variant_operations>();
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<dyn_object> ptr;
		if(variants::pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};


struct handle_operations : public null_operations<handle_operations>
{
	handle_operations() : null_operations<handle_operations>(tags::tag_handle)
	{

	}
	
	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		handle_t *handle1;
		if(!handle_pool.get_by_id(a, handle1)) return false;
		handle_t *handle2;
		if(!handle_pool.get_by_id(b, handle2)) return false;
		return *handle1 == *handle2;
	}
	
	virtual bool not(tag_ptr tag, cell a) const override
	{
		handle_t *handle;
		if(!handle_pool.get_by_id(a, handle)) return true;
		return !handle->alive();
	}
	
	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return handle_pool.remove_by_id(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(arg, handle)) return false;
		if(!handle_pool.release_ref(*handle)) return false;
		return true;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		decltype(handle_pool)::ref_container *handle;
		if(!handle_pool.get_by_id(arg, handle)) return false;
		if(!handle_pool.acquire_ref(*handle)) return false;
		return true;
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		handle_t *handle;
		if(handle_pool.get_by_id(arg, handle))
		{
			return handle->get().get_hash();
		}
		return null_operations::hash(tag, arg);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.push_back('<');
		handle_t *handle;
		if(handle_pool.get_by_id(arg, handle))
		{
			str.append(handle->get().to_string());
		}
		str.push_back('>');
	}

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const override
	{
		return std::make_unique<handle_operations>();
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<handle_t> ptr;
		if(handle_pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};

template <class Self, cell Tag>
struct generic_operations : public null_operations<Self>
{
	tag_ptr element;

	generic_operations() : null_operations<Self>(Tag), element(nullptr)
	{

	}

	generic_operations(tag_ptr element) : null_operations<Self>(Tag), element(element)
	{

	}

	virtual tag_ptr get_element() const override
	{
		return element;
	}

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const override
	{
		auto subname = this->get_subname(tags::find_tag(this->tag_uid), name);
		if(subname)
		{
			return std::make_unique<Self>(tags::find_tag(subname));
		}else{
			return std::make_unique<Self>();
		}
	}
};

struct list_operations : public generic_operations<list_operations, tags::tag_list>
{
	list_operations() : generic_operations()
	{

	}

	list_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		list_t *l;
		return !list_pool.get_by_id(a, l);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'l';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		list_t *l;
		if(list_pool.get_by_id(arg, l))
		{
			return list_pool.remove(l);
		}
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<list_t> l;
		if(list_pool.get_by_id(arg, l))
		{
			return l;
		}
		return {};
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		list_t *l;
		if(list_pool.get_by_id(arg, l))
		{
			list_t old;
			std::swap(*l, old);
			list_pool.remove(l);
			for(auto &obj : old)
			{
				obj.release();
			}
			return true;
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		list_t *l;
		if(list_pool.get_by_id(arg, l))
		{
			list_t *l2 = list_pool.add().get();
			*l2 = *l;
			return list_pool.get_id(l2);
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		list_t *l;
		if(list_pool.get_by_id(arg, l))
		{
			list_t tmp;
			std::swap(*l, tmp);
			list_t *l2 = list_pool.add().get();
			for(auto &obj : tmp)
			{
				l2->push_back(obj.clone());
			}
			std::swap(*l, tmp);
			return list_pool.get_id(l2);
		}
		return 0;
	}
};

struct linked_list_operations : public generic_operations<linked_list_operations, tags::tag_linked_list>
{
	linked_list_operations() : generic_operations()
	{

	}

	linked_list_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		linked_list_t *l;
		return !linked_list_pool.get_by_id(a, l);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'l';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		linked_list_t *l;
		if(linked_list_pool.get_by_id(arg, l))
		{
			return linked_list_pool.remove(l);
		}
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<linked_list_t> l;
		if(linked_list_pool.get_by_id(arg, l))
		{
			return l;
		}
		return {};
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		linked_list_t *l;
		if(linked_list_pool.get_by_id(arg, l))
		{
			linked_list_t old;
			std::swap(*l, old);
			linked_list_pool.remove(l);
			for(auto &obj : old)
			{
				obj->release();
			}
			return true;
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		linked_list_t *l;
		if(linked_list_pool.get_by_id(arg, l))
		{
			linked_list_t *l2 = linked_list_pool.add().get();
			*l2 = *l;
			return linked_list_pool.get_id(l2);
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		linked_list_t *l;
		if(linked_list_pool.get_by_id(arg, l))
		{
			linked_list_t tmp;
			std::swap(*l, tmp);
			linked_list_t *l2 = linked_list_pool.add().get();
			for(auto &obj : tmp)
			{
				l2->push_back(obj->clone());
			}
			std::swap(*l, tmp);
			return linked_list_pool.get_id(l2);
		}
		return 0;
	}
};

struct map_operations : public generic_operations<map_operations, tags::tag_map>
{
	map_operations() : generic_operations()
	{

	}

	map_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		map_t *m;
		return !map_pool.get_by_id(a, m);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'm';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		map_t *m;
		if(map_pool.get_by_id(arg, m))
		{
			return map_pool.remove(m);
		}
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<map_t> m;
		if(map_pool.get_by_id(arg, m))
		{
			return m;
		}
		return {};
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		map_t *m;
		if(map_pool.get_by_id(arg, m))
		{
			map_t old(m->ordered());
			std::swap(*m, old);
			map_pool.remove(m);
			for(auto &pair : old)
			{
				pair.first.release();
				pair.second.release();
			}
			return true;
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		map_t *m;
		if(map_pool.get_by_id(arg, m))
		{
			map_t *m2 = map_pool.add().get();
			*m2 = *m;
			return map_pool.get_id(m2);
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		map_t *m;
		if(map_pool.get_by_id(arg, m))
		{
			map_t tmp;
			std::swap(*m, tmp);
			map_t *m2 = map_pool.add().get();
			m2->set_ordered(m->ordered());
			for(auto &pair : tmp)
			{
				m2->insert(pair.first.clone(), pair.second.clone());
			}
			std::swap(*m, tmp);
			return map_pool.get_id(m2);
		}
		return 0;
	}
};

struct iter_operations : public generic_operations<iter_operations, tags::tag_iter>
{
	iter_operations() : generic_operations()
	{

	}

	iter_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		dyn_iterator *iter1;
		if(!iter_pool.get_by_id(a, iter1)) return 0;
		dyn_iterator *iter2;
		if(!iter_pool.get_by_id(b, iter2)) return 0;
		return *iter1 == *iter2;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		dyn_iterator *iter;
		return !iter_pool.get_by_id(a, iter);
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return iter_pool.remove_by_id(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(arg, iter)) return false;
		if(!iter_pool.release_ref(*iter)) return false;
		return true;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(arg, iter)) return false;
		if(!iter_pool.acquire_ref(*iter)) return false;
		return true;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(arg, iter))
		{
			return iter_pool.get_id(iter_pool.add(std::dynamic_pointer_cast<object_pool<dyn_iterator>::ref_container>(iter->clone_shared())));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(arg, iter))
		{
			return iter->get_hash();
		}
		return null_operations::hash(tag, arg);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		str.push_back('[');
		dyn_iterator *iter;
		if(iter_pool.get_by_id(arg, iter))
		{
			const std::pair<const dyn_object, dyn_object> *pair;
			if(iter->extract(pair))
			{
				str.append(pair->first.to_string());
				str.push_back('=');
				str.push_back('>');
				str.append(pair->second.to_string());
			}else{
				std::shared_ptr<const std::pair<const dyn_object, dyn_object>> spair;
				if(iter->extract(spair))
				{
					str.append(spair->first.to_string());
					str.push_back('=');
					str.push_back('>');
					str.append(spair->second.to_string());
				}else{
					dyn_object *obj;
					if(iter->extract(obj))
					{
						str.append(obj->to_string());
					}
				}
			}
		}
		str.push_back(']');
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<dyn_iterator> ptr;
		if(iter_pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};

struct ref_operations : public generic_operations<ref_operations, tags::tag_ref>
{
	ref_operations() : generic_operations()
	{

	}

	ref_operations(tag_ptr element) : generic_operations(element)
	{

	}
	
	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		auto base = tags::find_tag(tag_uid);
		if(tag != base && tag->inherits_from(base))
		{
			auto subtag = tags::find_tag(tag->name.substr(base->name.size()+1).c_str());
			str.append(subtag->get_ops().to_string(subtag, arg));
		}else{
			null_operations::append_string(tag, arg, str);
		}
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		return arg;
	}
};

struct task_operations : public generic_operations<task_operations, tags::tag_task>
{
	task_operations() : generic_operations()
	{

	}

	task_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		tasks::task *task;
		return !tasks::get_by_id(a, task);
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		tasks::task *task;
		if(tasks::get_by_id(arg, task))
		{
			return tasks::remove(task);
		}
		return false;
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<tasks::task> t;
		if(tasks::get_by_id(arg, t))
		{
			return t;
		}
		return {};
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		tasks::task *task;
		if(tasks::get_by_id(arg, task))
		{
			return tasks::get_id(&(*tasks::add() = task->clone()));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}
};

struct var_operations : public null_operations<var_operations>
{
	var_operations() : null_operations<var_operations>(tags::tag_var)
	{

	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		amx_var_info *info;
		return !amx_var_pool.get_by_id(a, info);
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(arg, info))
		{
			return amx_var_pool.remove(info);
		}
		return false;
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		amx_var_info *info;
		if(amx_var_pool.get_by_id(arg, info))
		{
			return amx_var_pool.get_id(amx_var_pool.emplace(*info));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<amx_var_info> ptr;
		if(amx_var_pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};

struct guard_operations : public null_operations<guard_operations>
{
	guard_operations() : null_operations<guard_operations>(tags::tag_guard)
	{

	}
};

struct amx_guard_operations : public null_operations<amx_guard_operations>
{
	amx_guard_operations() : null_operations<amx_guard_operations>(tags::tag_amx_guard)
	{

	}
};

struct callback_handler_operations : public null_operations<callback_handler_operations>
{
	callback_handler_operations() : null_operations<callback_handler_operations>(tags::tag_callback_handler)
	{

	}
};


struct native_hook_operations : public null_operations<native_hook_operations>
{
	native_hook_operations() : null_operations<native_hook_operations>(tags::tag_native_hook)
	{

	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return amxhook::remove_hook(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}
};


struct symbol_operations : public null_operations<symbol_operations>
{
	symbol_operations() : null_operations<symbol_operations>(tags::tag_symbol)
	{

	}
};

struct pool_operations : public generic_operations<pool_operations, tags::tag_pool>
{
	pool_operations() : generic_operations<pool_operations, tags::tag_pool>()
	{

	}

	pool_operations(tag_ptr element) : generic_operations(element)
	{

	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool not(tag_ptr tag, cell a) const override
	{
		pool_t *l;
		return !pool_pool.get_by_id(a, l);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'l';
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		pool_t *p;
		if(pool_pool.get_by_id(arg, p))
		{
			return pool_pool.remove(p);
		}
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		std::shared_ptr<pool_t> p;
		if(pool_pool.get_by_id(arg, p))
		{
			return p;
		}
		return {};
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		pool_t *p;
		if(pool_pool.get_by_id(arg, p))
		{
			pool_t old(p->ordered());
			std::swap(*p, old);
			pool_pool.remove(p);
			for(auto &obj : old)
			{
				obj.release();
			}
			return true;
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		pool_t *p;
		if(pool_pool.get_by_id(arg, p))
		{
			pool_t *p2 = pool_pool.add().get();
			*p2 = *p;
			return pool_pool.get_id(p2);
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		pool_t *p;
		if(pool_pool.get_by_id(arg, p))
		{
			pool_t tmp;
			std::swap(*p, tmp);
			pool_t *p2 = pool_pool.add().get();
			p2->set_ordered(p->ordered());
			for(auto it = p->begin(); it != p->end(); ++it)
			{
				p2->insert_or_set(p->index_of(it), it->clone());
			}
			std::swap(*p, tmp);
			return pool_pool.get_id(p2);
		}
		return 0;
	}
};

struct expression_operations : public null_operations<expression_operations>
{
	expression_operations() : null_operations<expression_operations>(tags::tag_expression)
	{

	}
	
	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}
	
	virtual bool not(tag_ptr tag, cell a) const override
	{
		expression *ptr;
		return !expression_pool.get_by_id(a, ptr);
	}
	
	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return expression_pool.remove_by_id(arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(arg, ptr)) return false;
		if(!expression_pool.release_ref(*ptr)) return false;
		return true;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		decltype(expression_pool)::ref_container *ptr;
		if(!expression_pool.get_by_id(arg, ptr)) return false;
		if(!expression_pool.acquire_ref(*ptr)) return false;
		return true;
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		expression *ptr;
		if(expression_pool.get_by_id(arg, ptr))
		{
			ptr->to_string(str);
		}
	}

	virtual std::unique_ptr<tag_operations> derive(tag_ptr tag, cell uid, const char *name) const override
	{
		return std::make_unique<expression_operations>();
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		expression_ptr ptr;
		if(expression_pool.get_by_id(arg, ptr))
		{
			return ptr;
		}
		return {};
	}
};

static const null_operations<signed_operations> unknown_ops(tags::tag_unknown);

std::vector<std::unique_ptr<tag_info>> tag_list([]()
{
	std::vector<std::unique_ptr<tag_info>> v;
	auto unknown = std::make_unique<tag_info>(0, "{...}", nullptr, std::make_unique<null_operations<signed_operations>>(tags::tag_unknown));
	auto unknown_tag = unknown.get();
	auto string_const = std::make_unique<tag_info>(25, "String@Const", unknown_tag, std::make_unique<string_operations>());
	auto variant_const = std::make_unique<tag_info>(26, "Variant@Const", unknown_tag, std::make_unique<variant_operations>());
	v.push_back(std::move(unknown));
	v.push_back(std::make_unique<tag_info>(1, "", unknown_tag, std::make_unique<cell_operations<signed_operations>>(tags::tag_cell)));
	v.push_back(std::make_unique<tag_info>(2, "bool", unknown_tag, std::make_unique<bool_operations>()));
	v.push_back(std::make_unique<tag_info>(3, "char", unknown_tag, std::make_unique<char_operations>()));
	v.push_back(std::make_unique<tag_info>(4, "Float", unknown_tag, std::make_unique<float_operations>()));
	v.push_back(std::make_unique<tag_info>(5, "String", string_const.get(), nullptr));
	v.push_back(std::make_unique<tag_info>(6, "Variant", variant_const.get(), nullptr));
	v.push_back(std::make_unique<tag_info>(7, "List", unknown_tag, std::make_unique<list_operations>()));
	v.push_back(std::make_unique<tag_info>(8, "Map", unknown_tag, std::make_unique<map_operations>()));
	v.push_back(std::make_unique<tag_info>(9, "Iter", unknown_tag, std::make_unique<iter_operations>()));
	v.push_back(std::make_unique<tag_info>(10, "Ref", unknown_tag, std::make_unique<ref_operations>()));
	v.push_back(std::make_unique<tag_info>(11, "Task", unknown_tag, std::make_unique<task_operations>()));
	v.push_back(std::make_unique<tag_info>(12, "Var", unknown_tag, std::make_unique<var_operations>()));
	v.push_back(std::make_unique<tag_info>(13, "LinkedList", unknown_tag, std::make_unique<linked_list_operations>()));
	v.push_back(std::make_unique<tag_info>(14, "Guard", unknown_tag, std::make_unique<guard_operations>()));
	v.push_back(std::make_unique<tag_info>(15, "CallbackHandler", unknown_tag, std::make_unique<callback_handler_operations>()));
	v.push_back(std::make_unique<tag_info>(16, "NativeHook", unknown_tag, std::make_unique<native_hook_operations>()));
	v.push_back(std::make_unique<tag_info>(17, "Handle", unknown_tag, std::make_unique<handle_operations>()));
	v.push_back(std::make_unique<tag_info>(18, "Symbol", unknown_tag, std::make_unique<symbol_operations>()));
	v.push_back(std::make_unique<tag_info>(19, "signed", unknown_tag, std::make_unique<signed_operations>()));
	v.push_back(std::make_unique<tag_info>(20, "unsigned", unknown_tag, std::make_unique<unsigned_operations>()));
	v.push_back(std::make_unique<tag_info>(21, "Pool", unknown_tag, std::make_unique<pool_operations>()));
	v.push_back(std::make_unique<tag_info>(22, "Expression", unknown_tag, std::make_unique<expression_operations>()));
	v.push_back(std::make_unique<tag_info>(23, "address", unknown_tag, std::make_unique<cell_operations<unsigned_operations>>(tags::tag_address)));
	v.push_back(std::make_unique<tag_info>(24, "AmxGuard", unknown_tag, std::make_unique<amx_guard_operations>()));
	v.push_back(std::move(string_const));
	v.push_back(std::move(variant_const));
	v.push_back(std::make_unique<tag_info>(27, "char@", v[3].get(), std::make_unique<char_operations>()));
	return v;
}());

const tag_operations &tag_info::get_ops() const
{
	if(ops)
	{
		return *ops;
	}
	if(base)
	{
		return base->get_ops();
	}
	return unknown_ops;
}

class dynamic_operations : public null_operations<dynamic_operations>, public tag_control
{
	class op_handler
	{
	protected:
		op_type _type;

	public:
		op_handler(op_type type) : _type(type)
		{

		}

		virtual cell invoke(tag_ptr tag, cell *args, size_t numargs) = 0;

		cell invoke(tag_ptr tag, cell arg1, cell arg2)
		{
			cell args[2] = {arg1, arg2};
			return invoke(tag, args, 2);
		}

		cell invoke(tag_ptr tag, cell arg)
		{
			return invoke(tag, &arg, 1);
		}

		virtual ~op_handler() = default;
	};

	class amx_op_handler : public op_handler
	{
		amx::handle _amx;
		std::vector<stored_param> _args;
		std::string _handler;
		aux::optional<int> _index;

		bool handler_index(AMX *amx, int &index)
		{
			if(this->_index.has_value())
			{
				index = this->_index.value();

				char *funcname = amx_NameBuffer(amx);

				if(amx_GetPublic(amx, index, funcname) == AMX_ERR_NONE && !std::strcmp(_handler.c_str(), funcname))
				{
					return true;
				}else if(amx_FindPublicSafe(amx, _handler.c_str(), &index) == AMX_ERR_NONE)
				{
					this->_index = index;
					return true;
				}
			}else if(amx_FindPublicSafe(amx, _handler.c_str(), &index) == AMX_ERR_NONE)
			{
				this->_index = index;
				return true;
			}
			logwarn(amx, "[PawnPlus] Tag operation handler %s was not found.", _handler.c_str());
			return false;
		}

	public:
		amx_op_handler() = default;

		amx_op_handler(op_type type, AMX *amx, const char *handler, const char *add_format, const cell *args, int numargs)
			: op_handler(type), _amx(amx::load(amx)), _handler(handler)
		{
			if(add_format)
			{
				size_t argi = -1;
				size_t len = std::strlen(add_format);
				for(size_t i = 0; i < len; i++)
				{
					_args.push_back(stored_param::create(amx, add_format[i], args, argi, numargs));
				}
			}
		}

		virtual cell invoke(tag_ptr tag, cell *args, size_t numargs) override
		{
			if(auto lock = _amx.lock())
			{
				if(lock->valid())
				{
					auto amx = lock->get();
					int pub;
					if(handler_index(amx, pub))
					{
						for(size_t i = 1; i <= numargs; i++)
						{
							amx_Push(amx, args[numargs - i]);
						}
						for(auto it = _args.rbegin(); it != _args.rend(); it++)
						{
							it->push(amx, static_cast<int>(_type));
						}
						cell ret;
						if(amx_Exec(amx, &ret, pub) == AMX_ERR_NONE)
						{
							return ret;
						}
					}
				}
			}
			return 0;
		}
	};

	class func_op_handler : public op_handler
	{
		cell(*func)(void *cookie, const void *tag, cell *args, cell numargs);
		void *cookie;

	public:
		func_op_handler() = default;

		func_op_handler(op_type type, cell(*func)(void *cookie, const void *tag, cell *args, cell numargs), void *cookie)
			: op_handler(type), func(func), cookie(cookie)
		{

		}

		virtual cell invoke(tag_ptr tag, cell *args, size_t numargs) override
		{
			if(func != nullptr)
			{
				return func(cookie, tag, args, numargs);
			}
			return 0;
		}
	};

	bool _locked = false;
	std::unordered_map<op_type, std::unique_ptr<op_handler>> dyn_ops;

public:
	dynamic_operations(cell tag_uid) : null_operations(tag_uid)
	{

	}

	cell op_bin(tag_ptr tag, op_type op, cell a, cell b) const
	{
		auto it = dyn_ops.find(op);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, a, b);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		cell args[2] = {a, b};
		return base->call_op(tag, op, args, 2);
	}

	cell op_un(tag_ptr tag, op_type op, cell a) const
	{
		auto it = dyn_ops.find(op);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, a);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->call_op(tag, op, &a, 1);
	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::add, a, b);
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::sub, a, b);
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::mul, a, b);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::div, a, b);
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::mod, a, b);
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return op_un(tag, op_type::neg, a);
	}

	virtual cell inc(tag_ptr tag, cell a) const override
	{
		return op_un(tag, op_type::inc, a);
	}

	virtual cell dec(tag_ptr tag, cell a) const override
	{
		return op_un(tag, op_type::dec, a);
	}

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::eq, a, b);
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return op_un(tag, op_type::del, arg);
	}

	virtual bool release(tag_ptr tag, cell arg) const override
	{
		auto it = dyn_ops.find(op_type::release);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, arg);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().release(tag, arg);
		}
		return false;
	}

	virtual bool acquire(tag_ptr tag, cell arg) const override
	{
		auto it = dyn_ops.find(op_type::acquire);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, arg);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().acquire(tag, arg);
		}
		return false;
	}

	virtual std::weak_ptr<const void> handle(tag_ptr tag, cell arg) const override
	{
		auto it = dyn_ops.find(op_type::handle);
		if(it != dyn_ops.end() && it->second)
		{
			cell id = it->second->invoke(tag, arg);
			handle_t *handle;
			if(handle_pool.get_by_id(id, handle))
			{
				return handle->get_bond();
			}
			return {};
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().handle(tag, arg);
		}
		return {};
	}

	virtual bool collect(tag_ptr tag, const cell *arg, cell size) const override
	{
		auto it = dyn_ops.find(op_type::collect);
		if(it != dyn_ops.end() && it->second)
		{
			bool ok = false;
			for(cell i = 0; i < size; i++)
			{
				if(it->second->invoke(tag, arg[i])) ok = true;
			}
			return ok;
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().collect(tag, arg, size);
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		return op_un(tag, op_type::copy, arg);
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		auto it = dyn_ops.find(op_type::clone);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, arg);
		}
		return copy(tag, arg);
	}

	virtual bool assign(tag_ptr tag, cell *arg, cell size) const override
	{
		auto it = dyn_ops.find(op_type::assign);
		if(it != dyn_ops.end() && it->second)
		{
			for(cell i = 0; i < size; i++)
			{
				arg[i] = it->second->invoke(tag, arg[i]);
			}
			return true;
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().assign(tag, arg, size);
		}
		return false;
	}

	virtual bool init(tag_ptr tag, cell *arg, cell size) const override
	{
		auto it = dyn_ops.find(op_type::init);
		if(it != dyn_ops.end() && it->second)
		{
			for(cell i = 0; i < size; i++)
			{
				arg[i] = it->second->invoke(tag, arg[i]);
			}
			return true;
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base != nullptr)
		{
			return base->get_ops().init(tag, arg, size);
		}
		return false;
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		return op_un(tag, op_type::hash, arg);
	}

	virtual bool set_op(op_type type, AMX *amx, const char *handler, const char *add_format, const cell *args, int numargs) override
	{
		if(_locked) return false;
		dyn_ops[type] = std::make_unique<amx_op_handler>(type, amx, handler, add_format, args, numargs);
		return true;
	}

	virtual bool set_op(op_type type, cell(*handler)(void *cookie, const void *tag, cell *args, cell numargs), void *cookie) override
	{
		if(_locked) return false;
		dyn_ops[type] = std::make_unique<func_op_handler>(type, handler, cookie);
		return true;
	}

	virtual bool lock() override
	{
		if(_locked) return false;
		_locked = true;
		return true;
	}

	virtual cell_string to_string(tag_ptr tag, cell arg) const override
	{
		if(dyn_ops.find(op_type::string) != dyn_ops.end())
		{
			return null_operations::to_string(tags::find_tag(tag_uid), arg);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->get_ops().to_string(tag, arg);
	}

	virtual cell_string to_string(tag_ptr tag, const cell *arg, cell size) const override
	{
		if(dyn_ops.find(op_type::string) != dyn_ops.end())
		{
			return null_operations::to_string(tags::find_tag(tag_uid), arg, size);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->get_ops().to_string(tag, arg, size);
	}

	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const override
	{
		auto it = dyn_ops.find(op_type::string);
		if(it != dyn_ops.end() && it->second)
		{
			cell result = it->second->invoke(tag, arg);
			cell_string *ptr;
			if(strings::pool.get_by_id(result, ptr))
			{
				str.append(*ptr);
			}
			return;
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		base->get_ops().append_string(tag, arg, str);
	}

	virtual cell call_dyn_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const override
	{
		auto it = dyn_ops.find(type);
		if(it != dyn_ops.end() && it->second)
		{
			return it->second->invoke(tag, args, numargs);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->call_op(tag, type, args, numargs);
	}

	template <class Iter>
	void format(tag_ptr tag, const cell *arg, Iter fmt_begin, Iter fmt_end, std::basic_string<cell> &str) const
	{
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->get_ops().format_base(tag, arg, fmt_begin, fmt_end, str);
	}
};

template <class Self>
std::unique_ptr<tag_operations> null_operations<Self>::derive(tag_ptr tag, cell uid, const char *name) const
{
	if(tag->base)
	{
		return tag->base->get_ops().derive(tag->base, uid, name);
	}
	return std::make_unique<dynamic_operations>(uid);
}

tag_control *tag_info::get_control() const
{
	return dynamic_cast<dynamic_operations*>(ops.get());
}

cell tag_operations::call_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const
{
	switch(type)
	{
		case op_type::add:
			return numargs >= 2 ? add(tag, args[0], args[1]) : 0;
		case op_type::sub:
			return numargs >= 2 ? sub(tag, args[0], args[1]) : 0;
		case op_type::mul:
			return numargs >= 2 ? mul(tag, args[0], args[1]) : 0;
		case op_type::div:
			return numargs >= 2 ? div(tag, args[0], args[1]) : 0;
		case op_type::mod:
			return numargs >= 2 ? mod(tag, args[0], args[1]) : 0;
		case op_type::neg:
			return numargs >= 1 ? neg(tag, args[0]) : 0;
		case op_type::inc:
			return numargs >= 1 ? inc(tag, args[0]) : 0;
		case op_type::dec:
			return numargs >= 1 ? dec(tag, args[0]) : 0;

		case op_type::eq:
			return numargs >= 2 ? eq(tag, args[0], args[1]) : 0;
		case op_type::neq:
			return numargs >= 2 ? neq(tag, args[0], args[1]) : 0;
		case op_type::lt:
			return numargs >= 2 ? lt(tag, args[0], args[1]) : 0;
		case op_type::gt:
			return numargs >= 2 ? gt(tag, args[0], args[1]) : 0;
		case op_type::lte:
			return numargs >= 2 ? lte(tag, args[0], args[1]) : 0;
		case op_type::gte:
			return numargs >= 2 ? gte(tag, args[0], args[1]) : 0;
		case op_type::not:
			return numargs >= 1 ? not(tag, args[0]) : 0;

		case op_type::string:
			return numargs >= 1 ? strings::pool.get_id(strings::pool.add(to_string(tag, args[0]))) : 0;
		case op_type::del:
			return numargs >= 1 ? del(tag, args[0]) : 0;
		case op_type::release:
			return numargs >= 1 ? release(tag, args[0]) : 0;
		case op_type::acquire:
			return numargs >= 1 ? acquire(tag, args[0]) : 0;
		case op_type::collect:
			return 0;
		case op_type::copy:
			return numargs >= 1 ? copy(tag, args[0]) : 0;
		case op_type::clone:
			return numargs >= 1 ? clone(tag, args[0]) : 0;
		case op_type::assign:
			return numargs >= 1 ? assign(tag, &args[0], 1) : 0;
		case op_type::init:
			return numargs >= 1 ? init(tag, &args[0], 1) : 0;
		case op_type::hash:
			return numargs >= 1 ? hash(tag, args[0]) : 0;
		default:
			return call_dyn_op(tag, type, args, numargs);
	}
}

cell tag_info::call_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const
{
	return get_ops().call_op(tag, type, args, numargs);
}
