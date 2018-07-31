#include "tags.h"
#include "modules/strings.h"
#include "modules/variants.h"
#include "modules/containers.h"
#include "modules/tasks.h"
#include <vector>
#include <cmath>
#include <cstring>

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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool equals(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		for(cell i = 0; i < size; i++)
		{
			if(!equals(tag, a[i], b[i]))
			{
				return false;
			}
		}
		return true;
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

	virtual cell copy(tag_ptr tag, cell arg) const override
	{
		return arg;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}

	virtual size_t hash(tag_ptr tag, cell arg) const override
	{
		return std::hash<cell>()(arg);
	}

protected:
	virtual void append_string(tag_ptr tag, cell arg, cell_string &str) const
	{
		str.append(strings::convert(std::to_string(arg)));
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
		return a / b;
	}

	virtual cell mod(tag_ptr tag, cell a, cell b) const override
	{
		return a % b;
	}

	virtual cell neg(tag_ptr tag, cell a) const override
	{
		return -a;
	}

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
	}

	virtual bool equals(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return !std::memcmp(a, b, size * sizeof(cell));
	}
};

struct bool_operations : public cell_operations
{
	bool_operations() : cell_operations(tags::tag_bool)
	{

	}

protected:
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
		return str;
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 's' : 'c';
	}

protected:
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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		return amx_ctof(a) == amx_ctof(b);
	}

	virtual bool equals(tag_ptr tag, const cell *a, const cell *b, cell size) const override
	{
		return null_operations::equals(tag, a, b, size);
	}

	virtual char format_spec(tag_ptr tag, bool arr) const override
	{
		return arr ? 'a' : 'f';
	}

protected:
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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
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

protected:
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

	template <dyn_object(dyn_object::*op)(const dyn_object&) const>
	cell op(tag_ptr tag, cell a, cell b) const
	{
		dyn_object *var1;
		if(!variants::pool.get_by_id(a, var1)) return 0;
		dyn_object *var2;
		if(!variants::pool.get_by_id(b, var2)) return 0;
		auto var = (var1->*op)(*var2);
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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		dyn_object *var1;
		if(!variants::pool.get_by_id(a, var1) && var1 != nullptr) return false;
		dyn_object *var2;
		if(!variants::pool.get_by_id(b, var2) && var2 != nullptr) return false;
		if(var1 == nullptr || var1->empty()) return var2 == nullptr || var2->empty();
		if(var2 == nullptr) return false;
		return *var1 == *var2;
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

protected:
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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
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

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		return a == b;
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
	iter_operations() : null_operations(tags::tag_iterator)
	{

	}

	virtual bool equals(tag_ptr tag, cell a, cell b) const override
	{
		dyn_iterator *iter1;
		if(!iter_pool.get_by_id(a, iter1)) return 0;
		dyn_iterator *iter2;
		if(!iter_pool.get_by_id(b, iter2)) return 0;
		return *iter1 == *iter2;
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
			return iter_pool.get_id(iter_pool.clone(iter));
		}
		return 0;
	}

	virtual cell clone(tag_ptr tag, cell arg) const override
	{
		return copy(tag, arg);
	}
};

struct ref_operations : public null_operations
{
	ref_operations() : null_operations(tags::tag_ref)
	{

	}
	
protected:
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
};

struct task_operations : public null_operations
{
	task_operations() : null_operations(tags::tag_task)
	{

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

struct guard_operations : public null_operations
{
	guard_operations() : null_operations(tags::tag_guard)
	{

	}
};

std::vector<tag_operations*> op_list = {
	new null_operations(tags::tag_unknown),
	new cell_operations(tags::tag_cell),
	new bool_operations(),
	new char_operations(),
	new float_operations(),
	new string_operations(),
	new variant_operations(),
	new list_operations(),
	new map_operations(),
	new iter_operations(),
	new ref_operations(),
	new task_operations(),
	new guard_operations(),
};

const tag_operations &tag_info::get_ops() const
{
	if(uid >= 0 && (size_t)uid < op_list.size())
	{
		auto op = op_list[uid];
		if(op != nullptr)
		{
			return *op;
		}
	}
	if(base)
	{
		return base->get_ops();
	}
	return *op_list[tags::tag_unknown];
}
