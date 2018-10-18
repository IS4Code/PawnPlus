#include "tags.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/containers.h"
#include "modules/tasks.h"
#include "objects/stored_param.h"
#include "fixes/linux.h"
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
			str.append(strings::convert(tag->name));
			str.append({':'});
		}
		append_string(tag, arg, str);
		return str;
	}

	virtual cell_string to_string(tag_ptr tag, const cell *arg, cell size) const override
	{
		cell_string str;
		if(tag->uid != tag_uid)
		{
			str.append(strings::convert(tag->name));
			str.append({':'});
		}
		str.append({'{'});
		bool first = true;
		for(cell i = 0; i < size; i++)
		{
			if(first)
			{
				first = false;
			} else {
				str.append({',', ' '});
			}
			append_string(tag, arg[i], str);
		}
		str.append({'}'});
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

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
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
		str.append(strings::convert(std::to_string(arg)));
	}

	virtual cell call_dyn_op(tag_ptr tag, op_type type, cell *args, size_t numargs) const override
	{
		return 0;
	}
};

struct cell_operations : public null_operations
{
	cell_operations(cell tag_uid) : null_operations(tag_uid)
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
		if(b == 0) return 0;
		return a / b;
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		if(b == 0) return 0;
		return a % b;
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return -a;
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
};

struct bool_operations : public cell_operations
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

struct char_operations : public cell_operations
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
		str.append({arg});
	}
};

struct float_operations : public cell_operations
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

struct string_operations : public null_operations
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
			return strings::pool.get_id(strings::pool.add(true));
		}
		if(str1 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add(cell_string(*str2), true));
		}
		if(str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add(cell_string(*str1), true));
		}

		auto str = *str1 + *str2;
		return strings::pool.get_id(strings::pool.add(std::move(str), true));
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
		cell_string *str;
		if(strings::pool.get_by_id(arg, str))
		{
			return strings::pool.remove(str);
		}
		return false;
	}

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		cell_string *str;
		if(strings::pool.get_by_id(arg, str))
		{
			return strings::pool.get_id(strings::pool.clone(str));
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
};

struct variant_operations : public null_operations
{
	variant_operations() : null_operations(tags::tag_variant)
	{

	}

	template <dyn_object(dyn_object::*op_type)(const dyn_object&) const>
	cell op(tag_ptr tag, cell a, cell b) const
	{
		dyn_object *var1;
		if(!variants::pool.get_by_id(a, var1)) return 0;
		dyn_object *var2;
		if(!variants::pool.get_by_id(b, var2)) return 0;
		auto var = (var1->*op_type)(*var2);
		if(var.empty()) return 0;
		return variants::create(std::move(var));
	}

	virtual cell add(tag_ptr tag, cell a, cell b) const override
	{
		return op<&dyn_object::operator+>(tag, a, b);
	}

	virtual cell sub(tag_ptr tag, cell a, cell b) const override
	{
		return op<&dyn_object::operator- >(tag, a, b);
	}

	virtual cell mul(tag_ptr tag, cell a, cell b) const override
	{
		return op<&dyn_object::operator*>(tag, a, b);
	}

	virtual cell div(tag_ptr tag, cell a, cell b) const override
	{
		return op<&dyn_object::operator/>(tag, a, b);
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return op<&dyn_object::operator% >(tag, a, b);
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(a, var)) return 0;
		auto result = -(*var);
		if(result.empty()) return 0;
		return variants::create(std::move(result));
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
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			return variants::pool.remove(var);
		}
		return false;
	}

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			var->free();
			return variants::pool.remove(var);
		}
		return false;
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			return variants::pool.get_id(variants::pool.clone(var));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			return variants::pool.get_id(variants::pool.clone(var, [](const dyn_object &obj) {return obj.clone(); }));
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
		str.append({'('});
		dyn_object *var;
		if(variants::pool.get_by_id(arg, var))
		{
			str.append(var->to_string());
		}
		str.append({')'});
	}
};

struct list_operations : public null_operations
{
	list_operations() : null_operations(tags::tag_list)
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

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		list_t *l;
		if(list_pool.get_by_id(arg, l))
		{
			for(auto &obj : *l)
			{
				obj.free();
			}
			return list_pool.remove(l);
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
			list_t *l2 = list_pool.add().get();
			for(auto &obj : *l)
			{
				l2->push_back(obj.clone());
			}
			return list_pool.get_id(l2);
		}
		return 0;
	}
};

struct map_operations : public null_operations
{
	map_operations() : null_operations(tags::tag_map)
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

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		map_t *m;
		if(map_pool.get_by_id(arg, m))
		{
			for(auto &pair : *m)
			{
				pair.first.free();
				pair.second.free();
			}
			return map_pool.remove(m);
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
			map_t *m2 = map_pool.add().get();
			for(auto &pair : *m)
			{
				m2->insert(std::make_pair(pair.first.clone(), pair.second.clone()));
			}
			return map_pool.get_id(m2);
		}
		return 0;
	}
};

struct iter_operations : public null_operations
{
	iter_operations() : null_operations(tags::tag_iter)
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
		dyn_iterator *iter;
		if(iter_pool.get_by_id(arg, iter))
		{
			return iter_pool.remove(iter);
		}
		return false;
	}

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		dyn_iterator *iter;
		if(iter_pool.get_by_id(arg, iter))
		{
			return iter_pool.get_id(iter_pool.clone(iter, [](const dyn_iterator &iter) {return iter.clone(); }));
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
};

struct ref_operations : public null_operations
{
	ref_operations() : null_operations(tags::tag_ref)
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

struct task_operations : public null_operations
{
	task_operations() : null_operations(tags::tag_task)
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

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		return del(tag, arg);
	}

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		tasks::task *task;
		if(tasks::get_by_id(arg, task))
		{
			return tasks::remove(task);
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}
};

static const null_operations unknown_ops(tags::tag_unknown);

std::vector<std::unique_ptr<tag_info>> tag_list([]()
{
	std::vector<std::unique_ptr<tag_info>> v;
	auto unknown = std::make_unique<tag_info>(0, "{...}", nullptr, std::make_unique<null_operations>(tags::tag_unknown));
	auto unknown_tag = unknown.get();
	auto string_const = std::make_unique<tag_info>(12, "String@Const", unknown_tag, std::make_unique<string_operations>());
	auto variant_const = std::make_unique<tag_info>(13, "Variant@Const", unknown_tag, std::make_unique<variant_operations>());
	v.push_back(std::move(unknown));
	v.push_back(std::make_unique<tag_info>(1, "", unknown_tag, std::make_unique<cell_operations>(tags::tag_cell)));
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
	v.push_back(std::move(string_const));
	v.push_back(std::move(variant_const));
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

class dynamic_operations : public null_operations, public tag_control
{
	class op_handler
	{
		op_type _type;
		amx::handle _amx;
		std::vector<stored_param> _args;
		std::string _handler;

	public:
		op_handler() = default;

		op_handler(op_type type, AMX *amx, const char *handler, const char *add_format, const cell *args, int numargs)
			: _type(type), _amx(amx::load(amx)), _handler(handler)
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

		cell invoke(tag_ptr tag, cell *args, size_t numargs) const
		{
			if(auto lock = _amx.lock())
			{
				if(lock->valid())
				{
					auto amx = lock->get();
					int pub;
					if(amx_FindPublic(amx, _handler.c_str(), &pub) == AMX_ERR_NONE)
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

		cell invoke(tag_ptr tag, cell arg1, cell arg2) const
		{
			cell args[2] = {arg1, arg2};
			return invoke(tag, args, 2);
		}

		cell invoke(tag_ptr tag, cell arg) const
		{
			return invoke(tag, &arg, 1);
		}
	};

	bool _locked = false;
	std::unordered_map<op_type, op_handler> dyn_ops;

public:
	dynamic_operations(cell tag_uid) : null_operations(tag_uid)
	{

	}

	cell op_bin(tag_ptr tag, op_type op, cell a, cell b) const
	{
		auto it = dyn_ops.find(op);
		if(it != dyn_ops.end())
		{
			return it->second.invoke(tag, a, b);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		cell args[2] = {a, b};
		return base->call_op(tag, op, args, 2);
	}

	cell op_un(tag_ptr tag, op_type op, cell a) const
	{
		auto it = dyn_ops.find(op);
		if(it != dyn_ops.end())
		{
			return it->second.invoke(tag, a);
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

	virtual bool eq(tag_ptr tag, cell a, cell b) const override
	{
		return op_bin(tag, op_type::eq, a, b);
	}

	virtual bool del(tag_ptr tag, cell arg) const override
	{
		return op_un(tag, op_type::del, arg);
	}

	virtual bool free(tag_ptr tag, cell arg) const override
	{
		auto it = dyn_ops.find(op_type::free);
		if(it != dyn_ops.end())
		{
			return it->second.invoke(tag, arg);
		}
		return del(tag, arg);
	}

	virtual bool collect(tag_ptr tag, const cell *arg, cell size) const override
	{
		auto it = dyn_ops.find(op_type::collect);
		if(it != dyn_ops.end())
		{
			bool ok = false;
			for(cell i = 0; i < size; i++)
			{
				if(it->second.invoke(tag, arg[i])) ok = true;
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
		if(it != dyn_ops.end())
		{
			return it->second.invoke(tag, arg);
		}
		return copy(tag, arg);
	}

	virtual bool assign(tag_ptr tag, cell *arg, cell size) const override
	{
		auto it = dyn_ops.find(op_type::assign);
		if(it != dyn_ops.end())
		{
			for(cell i = 0; i < size; i++)
			{
				arg[i] = it->second.invoke(tag, arg[i]);
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
		if(it != dyn_ops.end())
		{
			for(cell i = 0; i < size; i++)
			{
				arg[i] = it->second.invoke(tag, arg[i]);
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
		try {
			dyn_ops[type] = op_handler(type, amx, handler, add_format, args, numargs);
		}catch(std::nullptr_t)
		{
			return false;
		}
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
		if(it != dyn_ops.end())
		{
			cell result = it->second.invoke(tag, arg);
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
		if(it != dyn_ops.end())
		{
			return it->second.invoke(tag, args, numargs);
		}
		tag_ptr base = tags::find_tag(tag_uid)->base;
		if(base == nullptr) base = tags::find_tag(tags::tag_unknown);
		return base->call_op(tag, type, args, numargs);
	}
};

std::unique_ptr<tag_operations> tags::new_dynamic_ops(cell uid)
{
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
		case op_type::string:
			return numargs >= 1 ? strings::pool.get_id(strings::pool.add(to_string(tag, args[0]), true)) : 0;
		case op_type::eq:
			return numargs >= 2 ? eq(tag, args[0], args[1]) : 0;
		case op_type::del:
			return numargs >= 1 ? del(tag, args[0]) : 0;
		case op_type::free:
			return numargs >= 1 ? free(tag, args[0]) : 0;
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
