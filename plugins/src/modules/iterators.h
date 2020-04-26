#ifndef ITERATORS_H_INCLUDED
#define ITERATORS_H_INCLUDED

#include "errors.h"
#include "modules/containers.h"
#include "modules/expressions.h"

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

	virtual bool expired() const override;
	virtual bool valid() const override;
	virtual bool move_next() override;
	virtual bool move_previous() override;
	virtual bool set_to_first() override;
	virtual bool set_to_last() override;
	virtual bool reset() override;
	virtual bool erase(bool stay) override;
	virtual bool can_reset() const override;
	virtual bool can_erase() const override;
	virtual bool can_insert() const override;
	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const dyn_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
	virtual bool insert_dyn(const std::type_info &type, void *value) override;
	virtual bool insert_dyn(const std::type_info &type, const void *value) override;
	virtual dyn_iterator *get() override;
	virtual const dyn_iterator *get() const override;
};

class repeat_base_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
protected:
	cell index;
	cell count;

public:
	repeat_base_iterator(cell count) : count(count), index(-1)
	{

	}

	virtual bool expired() const override { return false; }
	virtual bool valid() const override { return !expired() && index != -1; }
	virtual bool move_next() override;
	virtual bool move_previous() override;
	virtual bool set_to_first() override;
	virtual bool set_to_last() override;
	virtual bool reset() override;
	virtual bool erase(bool stay) override { return false; }
	virtual bool can_reset() const override { return true; }
	virtual bool can_erase() const override { return false; }
	virtual bool can_insert() const override { return false; }
	virtual bool operator==(const dyn_iterator &obj) const override;
	virtual bool operator==(const repeat_base_iterator &obj) const;
	virtual bool insert_dyn(const std::type_info &type, void *value) override { return false; }
	virtual bool insert_dyn(const std::type_info &type, const void *value) override { return false; }
	virtual dyn_iterator *get() override { return this; }
	virtual const dyn_iterator *get() const override { return this; }
};

class repeat_iterator : public repeat_base_iterator
{
	dyn_object value;

public:
	repeat_iterator(dyn_object value, cell count) : repeat_base_iterator(count), value(std::move(value))
	{

	}

	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const repeat_base_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
};

class variant_iterator : public repeat_base_iterator
{
	std::weak_ptr<dyn_object> var;

public:
	variant_iterator(const std::shared_ptr<dyn_object> &var, cell count) : repeat_base_iterator(count), var(var)
	{

	}

	virtual bool expired() const override { return var.expired(); }
	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const repeat_base_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
};

class handle_iterator : public repeat_base_iterator
{
	std::weak_ptr<handle_t> handle;

public:
	handle_iterator(const std::shared_ptr<handle_t> &handle, cell count) : repeat_base_iterator(count), handle(handle)
	{

	}

	virtual bool expired() const override;
	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const repeat_base_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
};

template <class T>
class dyn_modifiable_const_ptr
{
	T *ptr;

public:
	dyn_modifiable_const_ptr() : ptr(nullptr)
	{

	}

	dyn_modifiable_const_ptr(T *ptr) : ptr(ptr)
	{

	}

	T &operator*() const
	{
		return *ptr;
	}

	T *operator->() const
	{
		return ptr;
	}
};

template <class Func>
auto value_write(dyn_iterator *iter, Func f) -> typename std::result_of<Func(dyn_object&)>::type
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
	amx_LogicError(errors::operation_not_supported, "iterator");
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto value_modify(dyn_iterator *iter, Func f) -> typename std::result_of<Func(dyn_object&)>::type
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
	dyn_modifiable_const_ptr<dyn_object> mobj;
	if(iter->extract(mobj))
	{
		return f(*mobj);
	}
	dyn_modifiable_const_ptr<std::pair<const dyn_object, dyn_object>> mpair;
	if(iter->extract(mpair))
	{
		return f(mpair->second);
	}
	amx_LogicError(errors::operation_not_supported, "iterator");
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto value_read(dyn_iterator *iter, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
{
	const dyn_object *cobj;
	if(iter->extract(cobj))
	{
		return f(*cobj);
	}
	std::shared_ptr<const dyn_object> scobj;
	if(iter->extract(scobj))
	{
		return f(*scobj);
	}
	const std::pair<const dyn_object, dyn_object> *cpair;
	if(iter->extract(cpair))
	{
		return f(cpair->second);
	}
	std::shared_ptr<const std::pair<const dyn_object, dyn_object>> scpair;
	if(iter->extract(scpair))
	{
		return f(scpair->second);
	}
	amx_LogicError(errors::operation_not_supported, "iterator");
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto key_read(dyn_iterator *iter, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
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
	amx_LogicError(errors::operation_not_supported, "iterator");
	dyn_object tmp;
	return f(tmp);
}

template <class Func>
auto key_value_access(dyn_iterator *iter, Func f) -> typename std::result_of<Func(const dyn_object&)>::type
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
	amx_LogicError(errors::operation_not_supported, "iterator");
	dyn_object tmp;
	return f(tmp);
}

class filter_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
	std::shared_ptr<dyn_iterator> source;
	expression_ptr expr;

	bool is_valid(expression::args_type args) const;
	void find_valid();

public:
	filter_iterator(std::shared_ptr<dyn_iterator> &&source, expression_ptr &&expr) : source(std::move(source)), expr(std::move(expr))
	{
		find_valid();
	}

	virtual bool expired() const override;
	virtual bool valid() const override;
	virtual bool move_next() override;
	virtual bool move_previous() override;
	virtual bool set_to_first() override;
	virtual bool set_to_last() override;
	virtual bool reset() override;
	virtual bool erase(bool stay) override;
	virtual bool can_reset() const override;
	virtual bool can_erase() const override;
	virtual bool can_insert() const override;
	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const dyn_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
	virtual bool insert_dyn(const std::type_info &type, void *value) override;
	virtual bool insert_dyn(const std::type_info &type, const void *value) override;
	virtual dyn_iterator *get() override;
	virtual const dyn_iterator *get() const override;
};

class project_iterator : public dyn_iterator, public object_pool<dyn_iterator>::ref_container_virtual
{
	std::shared_ptr<dyn_iterator> source;
	expression_ptr expr;

public:
	project_iterator(std::shared_ptr<dyn_iterator> &&source, expression_ptr &&expr) : source(std::move(source)), expr(std::move(expr))
	{

	}

	virtual bool expired() const override;
	virtual bool valid() const override;
	virtual bool move_next() override;
	virtual bool move_previous() override;
	virtual bool set_to_first() override;
	virtual bool set_to_last() override;
	virtual bool reset() override;
	virtual bool erase(bool stay) override;
	virtual bool can_reset() const override;
	virtual bool can_erase() const override;
	virtual bool can_insert() const override;
	virtual std::unique_ptr<dyn_iterator> clone() const override;
	virtual std::shared_ptr<dyn_iterator> clone_shared() const override;
	virtual size_t get_hash() const override;
	virtual bool operator==(const dyn_iterator &obj) const override;
	virtual bool extract_dyn(const std::type_info &type, void *value) const override;
	virtual bool insert_dyn(const std::type_info &type, void *value) override;
	virtual bool insert_dyn(const std::type_info &type, const void *value) override;
	virtual dyn_iterator *get() override;
	virtual const dyn_iterator *get() const override;
};

#endif
