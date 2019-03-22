#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/variants.h"
#include "modules/strings.h"
#include "objects/dyn_object.h"
#include "fixes/linux.h"

class range_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
	cell index;
	cell count;
	cell skip;
	dyn_object begin;
	dyn_object current;

public:
	range_iterator(dyn_object &&start, cell count, cell skip) : count(count), skip(skip), index(-1), begin(std::move(start))
	{

	}

	virtual bool expired() const override
	{
		return false;
	}

	virtual bool valid() const override
	{
		return index != -1;
	}

	virtual bool move_next() override
	{
		if(valid())
		{
			index++;
			if(index != count)
			{
				if(skip > 0)
				{
					for(cell i = 0; i < skip; i++)
					{
						current = current.inc();
					}
				}else if(skip < 0)
				{
					for(cell i = skip; i < 0; i++)
					{
						current = current.dec();
					}
				}
				return true;
			}else{
				reset();
			}
		}
		return false;
	}

	virtual bool move_previous() override
	{
		if(valid())
		{
			if(index != 0)
			{
				if(skip > 0)
				{
					for(cell i = 0; i < skip; i++)
					{
						current = current.dec();
					}
				}else if(skip < 0)
				{
					for(cell i = skip; i < 0; i++)
					{
						current = current.inc();
					}
				}
				index--;
			}else{
				reset();
			}
		}
		return false;
	}

	virtual bool set_to_first() override
	{
		if(count == 0)
		{
			return false;
		}
		current = begin;
		index = 0;
		return true;
	}

	virtual bool set_to_last() override
	{
		return false;
	}

	virtual bool reset() override
	{
		index = -1;
		return true;
	}

	virtual bool erase() override
	{
		return false;
	}

	virtual std::unique_ptr<dyn_iterator> clone() const override
	{
		return std::make_unique<range_iterator>(*this);
	}

	virtual std::shared_ptr<dyn_iterator> clone_shared() const override
	{
		return std::make_shared<range_iterator>(*this);
	}

	virtual size_t get_hash() const override
	{
		if(valid())
		{
			return current.get_hash();
		}else{
			return begin.get_hash();
		}
	}

	virtual bool operator==(const dyn_iterator &obj) const override
	{
		auto other = dynamic_cast<const range_iterator*>(&obj);
		if(other != nullptr)
		{
			if(valid())
			{
				return other->valid() && current == other->current;
			}else{
				return !other->valid();
			}
		}
		return false;
	}

protected:
	virtual bool extract_dyn(const std::type_info &type, void *value) const override
	{
		if(valid())
		{
			if(type == typeid(const dyn_object*))
			{
				*reinterpret_cast<const dyn_object**>(value) = &current;
				return true;
			}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
			{
				*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(std::pair<const dyn_object, dyn_object>(dyn_object(index, tags::find_tag(tags::tag_cell)), current));
				return true;
			}
		}
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, void *value) override
	{
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, const void *value) override
	{
		return false;
	}

public:
	virtual dyn_iterator *get() override
	{
		return this;
	}

	virtual const dyn_iterator *get() const override
	{
		return this;
	}
};

class repeat_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
	cell index;
	cell count;
	dyn_object value;

public:
	repeat_iterator(dyn_object &&value, cell count) : count(count), index(-1), value(std::move(value))
	{

	}

	virtual bool expired() const override
	{
		return false;
	}

	virtual bool valid() const override
	{
		return index != -1;
	}

	virtual bool move_next() override
	{
		if(valid())
		{
			index++;
			if(index != count)
			{
				return true;
			}else{
				reset();
			}
		}
		return false;
	}

	virtual bool move_previous() override
	{
		if(valid())
		{
			if(index != 0)
			{
				index--;
			}else{
				reset();
			}
		}
		return false;
	}

	virtual bool set_to_first() override
	{
		if(count == 0)
		{
			return false;
		}
		index = 0;
		return true;
	}

	virtual bool set_to_last() override
	{
		if(count == 0)
		{
			return false;
		}
		index = count - 1;
		return true;
	}

	virtual bool reset() override
	{
		index = -1;
		return true;
	}

	virtual bool erase() override
	{
		return false;
	}

	virtual std::unique_ptr<dyn_iterator> clone() const override
	{
		return std::make_unique<repeat_iterator>(*this);
	}

	virtual std::shared_ptr<dyn_iterator> clone_shared() const override
	{
		return std::make_shared<repeat_iterator>(*this);
	}

	virtual size_t get_hash() const override
	{
		return value.get_hash();
	}

	virtual bool operator==(const dyn_iterator &obj) const override
	{
		auto other = dynamic_cast<const repeat_iterator*>(&obj);
		if(other != nullptr)
		{
			if(valid())
			{
				return other->valid() && index == other->index && value == other->value;
			}else{
				return !other->valid();
			}
		}
		return false;
	}

protected:
	virtual bool extract_dyn(const std::type_info &type, void *value) const override
	{
		if(valid())
		{
			if(type == typeid(const dyn_object*))
			{
				*reinterpret_cast<const dyn_object**>(value) = &this->value;
				return true;
			}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
			{
				*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(std::pair<const dyn_object, dyn_object>(dyn_object(index, tags::find_tag(tags::tag_cell)), this->value));
				return true;
			}
		}
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, void *value) override
	{
		return false;
	}

	virtual bool insert_dyn(const std::type_info &type, const void *value) override
	{
		return false;
	}

public:
	virtual dyn_iterator *get() override
	{
		return this;
	}

	virtual const dyn_iterator *get() const override
	{
		return this;
	}
};

template <class Func>
auto value_write(cell arg, Func f) -> typename std::result_of<Func(dyn_object&)>::type
{
	dyn_iterator *iter;
	if(iter_pool.get_by_id(arg, iter))
	{
		dyn_object *obj;
		if(iter->extract(obj))
		{
			return f(*obj);
		}
		std::shared_ptr<dyn_object> sobj;
		if(iter->extract(sobj))
		{
			return f(*sobj);
		}
		std::pair<const dyn_object, dyn_object> *pair;
		if(iter->extract(pair))
		{
			return f(pair->second);
		}
		std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
		if(iter->extract(spair))
		{
			return f(spair->second);
		}
		amx_LogicError(errors::operation_not_supported, "iterator", arg);
	}
	amx_LogicError(errors::pointer_invalid, "iterator", arg);
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto value_read(cell arg, dyn_iterator *iter, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
{
	const dyn_object *obj;
	if(iter->extract(obj))
	{
		return f(*obj);
	}
	std::shared_ptr<const dyn_object> sobj;
	if(iter->extract(sobj))
	{
		return f(*sobj);
	}
	const std::pair<const dyn_object, dyn_object> *pair;
	if(iter->extract(pair))
	{
		return f(pair->second);
	}
	std::shared_ptr<const std::pair<const dyn_object, dyn_object>> spair;
	if(iter->extract(spair))
	{
		return f(spair->second);
	}
	amx_LogicError(errors::operation_not_supported, "iterator", arg);
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto value_read(cell arg, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
{
	dyn_iterator *iter;
	if(iter_pool.get_by_id(arg, iter))
	{
		return value_read(arg, iter, f);
	}
	amx_LogicError(errors::pointer_invalid, "iterator", arg);
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto key_read(cell arg, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
{
	dyn_iterator *iter;
	if(iter_pool.get_by_id(arg, iter))
	{
		const std::pair<const dyn_object, dyn_object> *pair;
		if(iter->extract(pair))
		{
			return f(pair->first);
		}
		std::shared_ptr<const std::pair<const dyn_object, dyn_object>> spair;
		if(iter->extract(spair))
		{
			return f(spair->first);
		}
		amx_LogicError(errors::operation_not_supported, "iterator", arg);
	}
	amx_LogicError(errors::pointer_invalid, "iterator", arg);
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto key_value_access(cell arg, dyn_iterator *&iter, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
{
	if(iter_pool.get_by_id(arg, iter))
	{
		dyn_object *obj;
		if(iter->extract(obj))
		{
			return f(*obj);
		}
		std::shared_ptr<dyn_object> sobj;
		if(iter->extract(sobj))
		{
			return f(*sobj);
		}
		std::pair<const dyn_object, dyn_object> *pair;
		if(iter->extract(pair))
		{
			f(pair->first);
			return f(pair->second);
		}
		std::shared_ptr<std::pair<const dyn_object, dyn_object>> spair;
		if(iter->extract(spair))
		{
			f(spair->first);
			return f(spair->second);
		}
		amx_LogicError(errors::operation_not_supported, "iterator", arg);
	}
	amx_LogicError(errors::pointer_invalid, "iterator", arg);
	dyn_object tmp;
	return f(tmp);
}

template <size_t... Indices>
class value_at
{
	using value_ftype = typename dyn_factory<Indices...>::type;
	using result_ftype = typename dyn_result<Indices...>::type;

public:
	// native bool:iter_set(IterTag:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_set(AMX *amx, cell *params)
	{
		return value_write(params[1], [&](dyn_object &obj)
		{
			obj = Factory(amx, params[Indices]...);
			return 1;
		});
	}

	// native iter_get(IterTag:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get(AMX *amx, cell *params)
	{
		return value_read(params[1], [&](const dyn_object &obj)
		{
			return Factory(amx, obj, params[Indices]...);
		});
	}

	// native bool:iter_insert(IterTag:iter, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_insert(AMX *amx, cell *params)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		if(!iter->insert(Factory(amx, params[Indices]...))) amx_LogicError(errors::operation_not_supported, "iterator", params[1]);

		return 1;
	}

	// native iter_get_key(IterTag:iter, ...);
	template <result_ftype Factory>
	static cell AMX_NATIVE_CALL iter_get_key(AMX *amx, cell *params)
	{
		return key_read(params[1], [&](const dyn_object &obj)
		{
			return Factory(amx, obj, params[Indices]...);
		});
	}

	// native Iter:iter_range(AnyTag:start, count, skip=1, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_range(AMX *amx, cell *params)
	{
		auto &iter = iter_pool.add(std::make_unique<range_iterator>(Factory(amx, params[Indices]...), params[2], optparam(3, 1)));
		iter->set_to_first();
		return iter_pool.get_id(iter);
	}

	// native Iter:iter_repeat(AnyTag:value, count, ...);
	template <value_ftype Factory>
	static cell AMX_NATIVE_CALL iter_repeat(AMX *amx, cell *params)
	{
		auto &iter = iter_pool.add(std::make_unique<repeat_iterator>(Factory(amx, params[Indices]...), params[2]));
		iter->set_to_first();
		return iter_pool.get_id(iter);
	}
};

template <size_t... KeyIndices>
class key_at
{
	using key_ftype = typename dyn_factory<KeyIndices...>::type;

public:
	// native Iter:map_iter_at(Map:map, key, ...);
	template <key_ftype KeyFactory>
	static cell AMX_NATIVE_CALL map_iter_at(AMX *amx, cell *params)
	{
		std::shared_ptr<map_t> ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);

		auto &iter = iter_pool.add(std::make_unique<map_iterator_t>(ptr, ptr->find(KeyFactory(amx, params[KeyIndices]...))));
		return iter_pool.get_id(iter);
	}
};

// native bool:iter_set_cell(IterTag:iter, offset, AnyTag:value, ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL iter_set_cell(AMX *amx, cell *params)
{
	if(params[2] < 0) amx_LogicError(errors::out_of_range, "array offset");

	return value_write(params[1], [&](dyn_object &obj)
	{
		if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return false;
		return obj.set_cell(params[2], params[3]);
	});
}

// native bool:iter_set_cell_md(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), ...);
template <size_t TagIndex = 0>
static cell AMX_NATIVE_CALL iter_set_cell_md(AMX *amx, cell *params)
{
	return value_write(params[1], [&](dyn_object &obj)
	{
		if(TagIndex && !obj.tag_assignable(amx, params[TagIndex])) return false;
		cell offsets_size = params[4];
		cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
		return obj.set_cell(offsets_addr, offsets_size, params[3]);
	});
}

namespace Natives
{
	// native Iter:list_iter(List:list, index=0);
	AMX_DEFINE_NATIVE(list_iter, 1)
	{
		std::shared_ptr<list_t> ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		
		auto &iter = iter_pool.emplace_derived<list_iterator_t>(ptr);
		cell index = optparam(2, 0);
		if(index < 0)
		{
			iter->reset();
		}else{
			for(cell i = 0; i < index; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}
		return iter_pool.get_id(iter);
	}

	// native list_add_iter(List:list, Iter:iter, index=-1);
	AMX_DEFINE_NATIVE(list_add_iter, 2)
	{
		cell index = optparam(3, -1);
		if(index < -1) amx_LogicError(errors::out_of_range, "list index");
		list_t *ptr;
		if(!list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[2], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[2]);
		auto pos = ptr->end();
		if(index == -1)
		{
			index = ptr->size();
		}else if(static_cast<ucell>(index) > ptr->size())
		{
			amx_LogicError(errors::out_of_range, "list index");
		}else{
			pos = ptr->begin() + index;
		}
		while(iter->valid())
		{
			value_read(params[2], iter, [&](const dyn_object &obj)
			{
				pos = ptr->insert(pos, obj);
				++pos;
			});
			if(!iter->move_next())
			{
				break;
			}
		}
		return index;
	}

	// native Iter:map_iter(Map:map, index=0);
	AMX_DEFINE_NATIVE(map_iter, 1)
	{
		std::shared_ptr<map_t> ptr;
		if(!map_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "map", params[1]);
		
		auto &iter = iter_pool.emplace_derived<map_iterator_t>(ptr);
		cell index = optparam(2, 0);
		if(index < 0)
		{
			iter->reset();
		}else{
			for(cell i = 0; i < index; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}
		return iter_pool.get_id(iter);
	}

	// native Iter:map_iter_at(Map:map, AnyTag:key, TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_iter_at, 3)
	{
		return key_at<2, 3>::map_iter_at<dyn_func>(amx, params);
	}

	// native Iter:map_iter_at_arr(Map:map, const AnyTag:key[], key_size=sizeof(key), TagTag:key_tag_id=tagof(key));
	AMX_DEFINE_NATIVE(map_iter_at_arr, 4)
	{
		return key_at<2, 3, 4>::map_iter_at<dyn_func_arr>(amx, params);
	}

	// native Iter:map_iter_at_str(Map:map, const key[]);
	AMX_DEFINE_NATIVE(map_iter_at_str, 2)
	{
		return key_at<2>::map_iter_at<dyn_func_str>(amx, params);
	}

	// native Iter:map_iter_at_var(Map:map, ConstVariantTag:key);
	AMX_DEFINE_NATIVE(map_iter_at_var, 2)
	{
		return key_at<2>::map_iter_at<dyn_func_var>(amx, params);
	}

	// native Iter:linked_list_iter(LinkedList:linked_list, index=0);
	AMX_DEFINE_NATIVE(linked_list_iter, 1)
	{
		std::shared_ptr<linked_list_t> ptr;
		if(!linked_list_pool.get_by_id(params[1], ptr)) amx_LogicError(errors::pointer_invalid, "linked list", params[1]);
		
		auto &iter = iter_pool.emplace_derived<linked_list_iterator_t>(ptr);
		cell index = optparam(2, 0);
		if(index < 0)
		{
			iter->reset();
		}else{
			for(cell i = 0; i < index; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}
		return iter_pool.get_id(iter);
	}

	// native bool:iter_valid(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_valid, 1)
	{
		dyn_iterator *iter;
		return iter_pool.get_by_id(params[1], iter);
	}

	// native Iter:iter_acquire(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_acquire, 1)
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		if(!iter_pool.acquire_ref(*iter)) amx_LogicError(errors::cannot_acquire, "iterator", params[1]);
		return params[1];
	}

	// native Iter:iter_release(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_release, 1)
	{
		decltype(iter_pool)::ref_container *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		if(!iter_pool.release_ref(*iter)) amx_LogicError(errors::cannot_release, "iterator", params[1]);
		return params[1];
	}

	// native iter_delete(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_delete, 1)
	{
		if(!iter_pool.remove_by_id(params[1])) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		return 1;
	}

	// native bool:iter_linked(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_linked, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		
		return !iter->expired();
	}

	// native bool:iter_inside(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_inside, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		return iter->valid();
	}

	// native iter_type(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_type, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		return reinterpret_cast<cell>(&typeid(*iter));
	}

	// native String:iter_type_str_s(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_type_str_s, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		return strings::create(typeid(*iter).name());
	}

	// native Iterator:iter_erase(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_erase, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		if(!iter->erase()) amx_LogicError(errors::operation_not_supported, "iterator", params[1]);
		return params[1];
	}

	// native Iterator:iter_erase_deep(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_erase_deep, 1)
	{
		dyn_iterator *iter;
		key_value_access(params[1], iter, [&](const dyn_object &obj)
		{
			obj.release();
		});
		if(!iter->erase()) amx_LogicError(errors::operation_not_supported, "iterator", params[1]);
		return params[1];
	}

	// native Iterator:iter_reset(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_reset, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		if(!iter->reset()) amx_LogicError(errors::operation_not_supported, "iterator", params[1]);
		return params[1];
	}

	// native Iterator:iter_clone(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_clone, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);

		return iter_pool.get_id(iter_pool.add(std::dynamic_pointer_cast<object_pool<dyn_iterator>::ref_container>(iter->clone_shared())));
	}

	// native bool:iter_eq(IterTag:iter1, IterTag:iter2);
	AMX_DEFINE_NATIVE(iter_eq, 2)
	{
		dyn_iterator *iter1;
		if(!iter_pool.get_by_id(params[1], iter1)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		dyn_iterator *iter2;
		if(!iter_pool.get_by_id(params[2], iter2)) amx_LogicError(errors::pointer_invalid, "iterator", params[2]);
		return *iter1 == *iter2;
	}

	// native Iter:iter_range(AnyTag:start, count, skip=1, tag_id=tagof(start));
	AMX_DEFINE_NATIVE(iter_range, 4)
	{
		return value_at<1, 4>::iter_range<dyn_func>(amx, params);
	}

	// native Iter:iter_range_arr(AnyTag:start[], count, skip=1, size=sizeof(start), tag_id=tagof(start));
	AMX_DEFINE_NATIVE(iter_range_arr, 5)
	{
		return value_at<1, 4, 5>::iter_range<dyn_func_arr>(amx, params);
	}

	// native Iter:iter_range_var(ConstVariantTag:start, count, skip=1);
	AMX_DEFINE_NATIVE(iter_range_var, 3)
	{
		return value_at<1>::iter_range<dyn_func_var>(amx, params);
	}

	// native Iter:iter_repeat(AnyTag:value, count, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_repeat, 3)
	{
		return value_at<1, 3>::iter_repeat<dyn_func>(amx, params);
	}

	// native Iter:iter_repeat_arr(AnyTag:value[], count, size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_repeat_arr, 4)
	{
		return value_at<1, 3, 4>::iter_repeat<dyn_func_arr>(amx, params);
	}

	// native Iter:iter_repeat_var(ConstVariantTag:value, count);
	AMX_DEFINE_NATIVE(iter_repeat_var, 2)
	{
		return value_at<1>::iter_repeat<dyn_func_var>(amx, params);
	}

	// native Iterator:iter_move_next(IterTag:iter, steps=1);
	AMX_DEFINE_NATIVE(iter_move_next, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		
		cell steps = optparam(2, 1);
		if(steps < 0)
		{
			for(cell i = steps; i < 0; i++)
			{
				if(!iter->move_previous())
				{
					break;
				}
			}
		}else{
			for(cell i = 0; i < steps; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}
		return params[1];
	}

	// native Iterator:iter_move_previous(IterTag:iter, steps=1);
	AMX_DEFINE_NATIVE(iter_move_previous, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		
		cell steps = optparam(2, 1);
		if(steps < 0)
		{
			for(cell i = steps; i < 0; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}else{
			for(cell i = 0; i < steps; i++)
			{
				if(!iter->move_previous())
				{
					break;
				}
			}
		}
		return params[1];
	}

	// native Iterator:iter_to_first(IterTag:iter, index=0);
	AMX_DEFINE_NATIVE(iter_to_first, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		
		cell index = optparam(2, 0);
		if(index < 0)
		{
			iter->reset();
		}else{
			iter->set_to_first();
			for(cell i = 0; i < index; i++)
			{
				if(!iter->move_next())
				{
					break;
				}
			}
		}
		return params[1];
	}

	// native Iterator:iter_to_last(IterTag:iter, index=0);
	AMX_DEFINE_NATIVE(iter_to_last, 1)
	{
		dyn_iterator *iter;
		if(!iter_pool.get_by_id(params[1], iter)) amx_LogicError(errors::pointer_invalid, "iterator", params[1]);
		
		cell index = optparam(2, 0);
		if(index < 0)
		{
			iter->reset();
		}else{
			iter->set_to_last();
			for(cell i = 0; i < index; i++)
			{
				if(!iter->move_previous())
				{
					break;
				}
			}
		}
		return params[1];
	}

	// native iter_get(IterTag:iter, offset=0);
	AMX_DEFINE_NATIVE(iter_get, 2)
	{
		return value_at<2>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_arr, 3)
	{
		return value_at<2, 3>::iter_get<dyn_func_arr>(amx, params);
	}

	// native String:iter_get_str_s(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_str_s, 1)
	{
		return value_at<>::iter_get<dyn_func_str_s>(amx, params);
	}

	// native Variant:iter_get_var(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_var, 1)
	{
		return value_at<>::iter_get<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_safe(IterTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_arr_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get<dyn_func_arr>(amx, params);
	}

	// native iter_get_str_safe(IterTag:iter, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_str_safe, 5)
	{
		return value_at<2, 3>::iter_get<dyn_func_str>(amx, params);
	}

	// native String:iter_get_str_safe_s(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_str_safe_s, 3)
	{
		return value_at<0>::iter_get<dyn_func_str_s>(amx, params);
	}

	// native bool:iter_set(IterTag:iter, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set, 3)
	{
		return value_at<2, 3>::iter_set<dyn_func>(amx, params);
	}

	// native bool:iter_set_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set_arr, 4)
	{
		return value_at<2, 3, 4>::iter_set<dyn_func_arr>(amx, params);
	}

	// native bool:iter_set_str(IterTag:iter, const value[]);
	AMX_DEFINE_NATIVE(iter_set_str, 2)
	{
		return value_at<2>::iter_set<dyn_func_str>(amx, params);
	}

	// native bool:iter_set_var(IterTag:iter, VariantTag:value);
	AMX_DEFINE_NATIVE(iter_set_var, 2)
	{
		return value_at<2>::iter_set<dyn_func_var>(amx, params);
	}

	// native bool:iter_set_cell(IterTag:iter, offset, AnyTag:value);
	AMX_DEFINE_NATIVE(iter_set_cell, 3)
	{
		return ::iter_set_cell(amx, params);
	}

	// native bool:iter_set_cell_safe(IterTag:iter, offset, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set_cell_safe, 4)
	{
		return ::iter_set_cell<4>(amx, params);
	}

	// native iter_get_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_md, 3)
	{
		return value_at<2, 3>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_md_arr(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_md_arr, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get<dyn_func_arr>(amx, params);
	}

	// native String:iter_get_md_str_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_md_str_s, 3)
	{
		return value_at<2, 3>::iter_get<dyn_func_str_s>(amx, params);
	}

	// native bool:iter_get_md_safe(IterTag:iter, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_md_safe, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get<dyn_func>(amx, params);
	}

	// native iter_get_md_arr_safe(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_md_arr_safe, 6)
	{
		return value_at<3, 2, 4, 5, 6>::iter_get<dyn_func_arr>(amx, params);
	}

	// native iter_get_md_str_safe(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_md_str_safe, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get<dyn_func_str>(amx, params);
	}

	// native String:iter_get_md_str_safe_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_md_str_safe_s, 3)
	{
		return value_at<2, 3, 0>::iter_get<dyn_func_str_s>(amx, params);
	}

	// native bool:iter_set_cell_md(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_set_cell_md, 4)
	{
		return ::iter_set_cell_md(amx, params);
	}

	// native bool:iter_set_cell_md_safe(IterTag:iter, const offsets[], AnyTag:value, offsets_size=sizeof(offsets), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_set_cell_md_safe, 5)
	{
		return ::iter_set_cell_md<5>(amx, params);
	}

	// native bool:iter_insert(IterTag:iter, AnyTag:value, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_insert, 3)
	{
		return value_at<2, 3>::iter_insert<dyn_func>(amx, params);
	}

	// native bool:iter_insert_arr(IterTag:iter, const AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_insert_arr, 4)
	{
		return value_at<2, 3, 4>::iter_insert<dyn_func_arr>(amx, params);
	}

	// native bool:iter_insert_str(IterTag:iter, const value[]);
	AMX_DEFINE_NATIVE(iter_insert_str, 2)
	{
		return value_at<2>::iter_insert<dyn_func_str>(amx, params);
	}

	// native bool:iter_insert_var(IterTag:iter, VariantTag:value);
	AMX_DEFINE_NATIVE(iter_insert_var, 2)
	{
		return value_at<2>::iter_insert<dyn_func_var>(amx, params);
	}

	// native iter_get_key(IterTag:iter, offset=0);
	AMX_DEFINE_NATIVE(iter_get_key, 2)
	{
		return value_at<2>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr(IterTag:iter, AnyTag:value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_key_arr, 3)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native String:iter_get_key_str_s(IterTag:iter));
	AMX_DEFINE_NATIVE(iter_get_key_str_s, 1)
	{
		return value_at<>::iter_get_key<dyn_func_str_s>(amx, params);
	}

	// native Variant:iter_get_key_var(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_key_var, 1)
	{
		return value_at<>::iter_get_key<dyn_func_var>(amx, params);
	}

	// native bool:iter_get_key_safe(IterTag:iter, &AnyTag:value, offset=0, tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_arr_safe(IterTag:iter, AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_arr_safe, 4)
	{
		return value_at<2, 3, 4>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native iter_get_key_str_safe(IterTag:iter, value[], size=sizeof(value));
	AMX_DEFINE_NATIVE(iter_get_key_str_safe, 3)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_str>(amx, params);
	}

	// native String:iter_get_key_str_safe_s(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_get_key_str_safe_s, 1)
	{
		return value_at<0>::iter_get_key<dyn_func_str_s>(amx, params);
	}

	// native iter_get_key_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_key_md, 3)
	{
		return value_at<2, 3>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_md_arr(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_key_md_arr, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native String:iter_get_key_md_str_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_key_md_str_s, 3)
	{
		return value_at<2, 3>::iter_get_key<dyn_func_str_s>(amx, params);
	}

	// native bool:iter_get_key_md_safe(IterTag:iter, const offsets[], &AnyTag:value, offsets_size=sizeof(offsets), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_md_safe, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get_key<dyn_func>(amx, params);
	}

	// native iter_get_key_md_arr_safe(IterTag:iter, const offsets[], AnyTag:value[], size=sizeof(value), offsets_size=sizeof(offsets), tag_id=tagof(value));
	AMX_DEFINE_NATIVE(iter_get_key_md_arr_safe, 6)
	{
		return value_at<3, 2, 4, 5, 6>::iter_get_key<dyn_func_arr>(amx, params);
	}

	// native iter_get_key_md_str_safe(IterTag:iter, const offsets[], value[], size=sizeof(value), offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_key_md_str_safe, 5)
	{
		return value_at<3, 2, 4, 5>::iter_get_key<dyn_func_str>(amx, params);
	}

	// native String:iter_get_key_md_str_safe_s(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_get_key_md_str_safe_s, 3)
	{
		return value_at<2, 3, 0>::iter_get_key<dyn_func_str_s>(amx, params);
	}
	
	// native iter_tagof(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_tagof, 1)
	{
		return value_read(params[1], [&](const dyn_object &obj)
		{
			return obj.get_tag(amx);
		});
	}

	// native iter_sizeof(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_sizeof, 1)
	{
		return value_read(params[1], [&](const dyn_object &obj)
		{
			return obj.get_size();
		});
	}

	// native iter_sizeof_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_sizeof_md, 3)
	{
		return value_read(params[1], [&](const dyn_object &obj)
		{
			cell offsets_size = params[3];
			cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
			return obj.get_size(offsets_addr, offsets_size);
		});
	}

	// native iter_tagof_key(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_tagof_key, 1)
	{
		return key_read(params[1], [&](const dyn_object &obj)
		{
			return obj.get_tag(amx);
		});
	}

	// native iter_sizeof_key(IterTag:iter);
	AMX_DEFINE_NATIVE(iter_sizeof_key, 1)
	{
		return key_read(params[1], [&](const dyn_object &obj)
		{
			return obj.get_size();
		});
	}

	// native iter_sizeof_key_md(IterTag:iter, const offsets[], offsets_size=sizeof(offsets));
	AMX_DEFINE_NATIVE(iter_sizeof_key_md, 3)
	{
		return key_read(params[1], [&](const dyn_object &obj)
		{
			cell offsets_size = params[3];
			cell *offsets_addr = get_offsets(amx, params[2], offsets_size);
			return obj.get_size(offsets_addr, offsets_size);
		});
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(list_iter),
	AMX_DECLARE_NATIVE(list_add_iter),
	AMX_DECLARE_NATIVE(map_iter),
	AMX_DECLARE_NATIVE(map_iter_at),
	AMX_DECLARE_NATIVE(map_iter_at_arr),
	AMX_DECLARE_NATIVE(map_iter_at_str),
	AMX_DECLARE_NATIVE(map_iter_at_var),
	AMX_DECLARE_NATIVE(linked_list_iter),

	AMX_DECLARE_NATIVE(iter_valid),
	AMX_DECLARE_NATIVE(iter_acquire),
	AMX_DECLARE_NATIVE(iter_release),
	AMX_DECLARE_NATIVE(iter_delete),
	AMX_DECLARE_NATIVE(iter_linked),
	AMX_DECLARE_NATIVE(iter_inside),
	AMX_DECLARE_NATIVE(iter_type),
	AMX_DECLARE_NATIVE(iter_type_str_s),
	AMX_DECLARE_NATIVE(iter_erase),
	AMX_DECLARE_NATIVE(iter_erase_deep),
	AMX_DECLARE_NATIVE(iter_reset),
	AMX_DECLARE_NATIVE(iter_clone),
	AMX_DECLARE_NATIVE(iter_eq),

	AMX_DECLARE_NATIVE(iter_range),
	AMX_DECLARE_NATIVE(iter_range_arr),
	AMX_DECLARE_NATIVE(iter_range_var),
	AMX_DECLARE_NATIVE(iter_repeat),
	AMX_DECLARE_NATIVE(iter_repeat_arr),
	AMX_DECLARE_NATIVE(iter_repeat_var),

	AMX_DECLARE_NATIVE(iter_move_next),
	AMX_DECLARE_NATIVE(iter_move_previous),
	AMX_DECLARE_NATIVE(iter_to_first),
	AMX_DECLARE_NATIVE(iter_to_last),

	AMX_DECLARE_NATIVE(iter_get),
	AMX_DECLARE_NATIVE(iter_get_arr),
	AMX_DECLARE_NATIVE(iter_get_str_s),
	AMX_DECLARE_NATIVE(iter_get_var),
	AMX_DECLARE_NATIVE(iter_get_safe),
	AMX_DECLARE_NATIVE(iter_get_arr_safe),
	AMX_DECLARE_NATIVE(iter_get_str_safe),
	AMX_DECLARE_NATIVE(iter_get_str_safe_s),

	AMX_DECLARE_NATIVE(iter_set),
	AMX_DECLARE_NATIVE(iter_set_arr),
	AMX_DECLARE_NATIVE(iter_set_str),
	AMX_DECLARE_NATIVE(iter_set_var),

	AMX_DECLARE_NATIVE(iter_set_cell),
	AMX_DECLARE_NATIVE(iter_set_cell_safe),

	AMX_DECLARE_NATIVE(iter_get_md),
	AMX_DECLARE_NATIVE(iter_get_md_arr),
	AMX_DECLARE_NATIVE(iter_get_md_str_s),
	AMX_DECLARE_NATIVE(iter_get_md_safe),
	AMX_DECLARE_NATIVE(iter_get_md_arr_safe),
	AMX_DECLARE_NATIVE(iter_get_md_str_safe),
	AMX_DECLARE_NATIVE(iter_get_md_str_safe_s),

	AMX_DECLARE_NATIVE(iter_set_cell_md),
	AMX_DECLARE_NATIVE(iter_set_cell_md_safe),

	AMX_DECLARE_NATIVE(iter_insert),
	AMX_DECLARE_NATIVE(iter_insert_arr),
	AMX_DECLARE_NATIVE(iter_insert_str),
	AMX_DECLARE_NATIVE(iter_insert_var),

	AMX_DECLARE_NATIVE(iter_get_key),
	AMX_DECLARE_NATIVE(iter_get_key_arr),
	AMX_DECLARE_NATIVE(iter_get_key_str_s),
	AMX_DECLARE_NATIVE(iter_get_key_var),
	AMX_DECLARE_NATIVE(iter_get_key_safe),
	AMX_DECLARE_NATIVE(iter_get_key_arr_safe),
	AMX_DECLARE_NATIVE(iter_get_key_str_safe),
	AMX_DECLARE_NATIVE(iter_get_key_str_safe_s),

	AMX_DECLARE_NATIVE(iter_get_key_md),
	AMX_DECLARE_NATIVE(iter_get_key_md_arr),
	AMX_DECLARE_NATIVE(iter_get_key_md_str_s),
	AMX_DECLARE_NATIVE(iter_get_key_md_safe),
	AMX_DECLARE_NATIVE(iter_get_key_md_arr_safe),
	AMX_DECLARE_NATIVE(iter_get_key_md_str_safe),
	AMX_DECLARE_NATIVE(iter_get_key_md_str_safe_s),

	AMX_DECLARE_NATIVE(iter_sizeof),
	AMX_DECLARE_NATIVE(iter_sizeof_md),
	AMX_DECLARE_NATIVE(iter_tagof),
	AMX_DECLARE_NATIVE(iter_sizeof_key),
	AMX_DECLARE_NATIVE(iter_sizeof_key_md),
	AMX_DECLARE_NATIVE(iter_tagof_key),
};

int RegisterIterNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
