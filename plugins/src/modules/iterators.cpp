#include "iterators.h"

bool range_iterator::expired() const
{
	return false;
}

bool range_iterator::valid() const
{
	return index != -1;
}

bool range_iterator::move_next()
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

bool range_iterator::move_previous()
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

bool range_iterator::set_to_first()
{
	if(count == 0)
	{
		return false;
	}
	current = begin;
	index = 0;
	return true;
}

bool range_iterator::set_to_last()
{
	return false;
}

bool range_iterator::reset()
{
	index = -1;
	return true;
}

bool range_iterator::erase(bool stay)
{
	return false;
}

bool range_iterator::can_reset() const
{
	return true;
}

bool range_iterator::can_erase() const
{
	return false;
}

bool range_iterator::can_insert() const
{
	return false;
}

std::unique_ptr<dyn_iterator> range_iterator::clone() const
{
	return std::make_unique<range_iterator>(*this);
}

std::shared_ptr<dyn_iterator> range_iterator::clone_shared() const
{
	return std::make_shared<range_iterator>(*this);
}

size_t range_iterator::get_hash() const
{
	if(valid())
	{
		return current.get_hash();
	}else{
		return begin.get_hash();
	}
}

bool range_iterator::operator==(const dyn_iterator &obj) const
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

bool range_iterator::extract_dyn(const std::type_info &type, void *value) const
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

bool range_iterator::insert_dyn(const std::type_info &type, void *value)
{
	return false;
}

bool range_iterator::insert_dyn(const std::type_info &type, const void *value)
{
	return false;
}

dyn_iterator *range_iterator::get()
{
	return this;
}

const dyn_iterator *range_iterator::get() const
{
	return this;
}

bool repeat_base_iterator::move_next()
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

bool repeat_base_iterator::move_previous()
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

bool repeat_base_iterator::set_to_first()
{
	if(count == 0 || expired())
	{
		return false;
	}
	index = 0;
	return true;
}

bool repeat_base_iterator::set_to_last()
{
	if(count == 0 || expired())
	{
		return false;
	}
	index = count - 1;
	return true;
}

bool repeat_base_iterator::reset()
{
	if(!expired())
	{
		index = -1;
		return true;
	}
	return false;
}

bool repeat_base_iterator::operator==(const dyn_iterator &obj) const
{
	auto other = dynamic_cast<const repeat_base_iterator*>(&obj);
	if(other != nullptr)
	{
		return *this == *other;
	}
	return false;
}

bool repeat_base_iterator::operator==(const repeat_base_iterator &other) const
{
	if(valid())
	{
		return other.valid() && index == other.index;
	} else {
		return !other.valid();
	}
}

std::unique_ptr<dyn_iterator> repeat_iterator::clone() const
{
	return std::make_unique<repeat_iterator>(*this);
}

std::shared_ptr<dyn_iterator> repeat_iterator::clone_shared() const
{
	return std::make_shared<repeat_iterator>(*this);
}

size_t repeat_iterator::get_hash() const
{
	return value.get_hash();
}

bool repeat_iterator::operator==(const repeat_base_iterator &obj) const
{
	if(repeat_base_iterator::operator==(obj))
	{
		auto other = dynamic_cast<const repeat_iterator*>(&obj);
		if(other != nullptr)
		{
			return !other->valid() || value == other->value;
		}
	}
	return false;
}

bool repeat_iterator::extract_dyn(const std::type_info &type, void *value) const
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

std::unique_ptr<dyn_iterator> variant_iterator::clone() const
{
	return std::make_unique<variant_iterator>(*this);
}

std::shared_ptr<dyn_iterator> variant_iterator::clone_shared() const
{
	return std::make_shared<variant_iterator>(*this);
}

size_t variant_iterator::get_hash() const
{
	return std::hash<dyn_object*>()(&*var.lock());
}

bool variant_iterator::operator==(const repeat_base_iterator &obj) const
{
	if(repeat_base_iterator::operator==(obj))
	{
		auto other = dynamic_cast<const variant_iterator*>(&obj);
		if(other != nullptr)
		{
			return !var.owner_before(other->var) && !other->var.owner_before(var);
		}
	}
	return false;
}

bool variant_iterator::extract_dyn(const std::type_info &type, void *value) const
{
	if(index != -1)
	{
		if(auto obj = var.lock())
		{
			if(type == typeid(const dyn_object*))
			{
				*reinterpret_cast<const dyn_object**>(value) = obj.get();
				return true;
			}else if(type == typeid(std::shared_ptr<const dyn_object>))
			{
				*reinterpret_cast<std::shared_ptr<const dyn_object>*>(value) = obj;
				return true;
			}else if(type == typeid(dyn_modifiable_const_ptr<dyn_object>))
			{
				*reinterpret_cast<dyn_modifiable_const_ptr<dyn_object>*>(value) = obj.get();
				return true;
			}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
			{
				*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(std::pair<const dyn_object, dyn_object>(dyn_object(index, tags::find_tag(tags::tag_cell)), *obj));
				return true;
			}
		}
	}
	return false;
}


bool handle_iterator::expired() const
{
	if(auto obj = handle.lock())
	{
		return !obj->alive();
	}
	return true;
}

std::unique_ptr<dyn_iterator> handle_iterator::clone() const
{
	return std::make_unique<handle_iterator>(*this);
}

std::shared_ptr<dyn_iterator> handle_iterator::clone_shared() const
{
	return std::make_shared<handle_iterator>(*this);
}

size_t handle_iterator::get_hash() const
{
	return std::hash<handle_t*>()(&*handle.lock());
}

bool handle_iterator::operator==(const repeat_base_iterator &obj) const
{
	if(repeat_base_iterator::operator==(obj))
	{
		auto other = dynamic_cast<const handle_iterator*>(&obj);
		if(other != nullptr)
		{
			return !handle.owner_before(other->handle) && !other->handle.owner_before(handle);
		}
	}
	return false;
}

bool handle_iterator::extract_dyn(const std::type_info &type, void *value) const
{
	if(index != -1)
	{
		if(auto obj = handle.lock())
		{
			if(obj->alive())
			{
				if(type == typeid(const dyn_object*))
				{
					*reinterpret_cast<const dyn_object**>(value) = &obj->get();
					return true;
				}else if(type == typeid(std::shared_ptr<const dyn_object>))
				{
					if(auto bond = obj->get_bond().lock())
					{
						*reinterpret_cast<std::shared_ptr<const dyn_object>*>(value) = std::shared_ptr<const dyn_object>(bond, &obj->get());
					} else {
						*reinterpret_cast<std::shared_ptr<const dyn_object>*>(value) = std::make_shared<const dyn_object>(obj->get());
					}
					return true;
				}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
				{
					*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(std::pair<const dyn_object, dyn_object>(dyn_object(index, tags::find_tag(tags::tag_cell)), obj->get()));
					return true;
				}
			}
		}
	}
	return false;
}

bool filter_iterator::is_valid(expression::args_type args) const
{
	return value_read(source.get(), [&](const dyn_object &val)
	{
		if(args.size() == 0)
		{
			args.push_back(std::ref(val));
		}else{
			args[0] = std::ref(val);
		}
		try{
			return key_read(source.get(), [&](const dyn_object &key)
			{
				if(args.size() == 1)
				{
					args.push_back(std::ref(key));
				}else{
					args[1] = std::ref(key);
				}
				return expr->execute_bool(args, expression::exec_info());
			});
		}catch(const errors::native_error&)
		{
			args.erase(std::next(args.begin()), args.end());
			return expr->execute_bool(args, expression::exec_info());
		}
	});
}

void filter_iterator::find_valid()
{
	expression::args_type args;
	do{
		if(is_valid(args))
		{
			return;
		}
	}while(source->move_next());
}

bool filter_iterator::expired() const
{
	return source->expired();
}

bool filter_iterator::valid() const
{
	return source->valid();
}

bool filter_iterator::move_next()
{
	expression::args_type args;
	while(source->move_next())
	{
		if(is_valid(args))
		{
			return true;
		}
	}
	return false;
}

bool filter_iterator::move_previous()
{
	expression::args_type args;
	while(source->move_previous())
	{
		if(is_valid(args))
		{
			return true;
		}
	}
	return false;
}

bool filter_iterator::set_to_first()
{
	if(source->set_to_first())
	{
		expression::args_type args;
		do{
			if(is_valid(args))
			{
				return true;
			}
		}while(source->move_next());
	}
	return false;
}

bool filter_iterator::set_to_last()
{
	if(source->set_to_last())
	{
		expression::args_type args;
		do{
			if(is_valid(args))
			{
				return true;
			}
		}while(source->move_previous());
	}
	return false;
}

bool filter_iterator::reset()
{
	return source->reset();
}

bool filter_iterator::erase(bool stay)
{
	return source->erase(stay);
}

bool filter_iterator::can_reset() const
{
	return source->can_reset();
}

bool filter_iterator::can_erase() const
{
	return source->can_erase();
}

bool filter_iterator::can_insert() const
{
	return source->can_insert();
}

std::unique_ptr<dyn_iterator> filter_iterator::clone() const
{
	return std::make_unique<filter_iterator>(*this);
}

std::shared_ptr<dyn_iterator> filter_iterator::clone_shared() const
{
	return std::make_shared<filter_iterator>(*this);
}

size_t filter_iterator::get_hash() const
{
	return source->get_hash();
}

bool filter_iterator::operator==(const dyn_iterator &obj) const
{
	auto other = dynamic_cast<const filter_iterator*>(&obj);
	if(other != nullptr)
	{
		if(valid())
		{
			return other->valid() && source == other->source && expr == other->expr;
		}else{
			return !other->valid();
		}
	}
	return false;
}

bool filter_iterator::extract_dyn(const std::type_info &type, void *value) const
{
	return source->extract_dyn(type, value);
}

bool filter_iterator::insert_dyn(const std::type_info &type, void *value)
{
	if(type == typeid(dyn_object))
	{
		auto &obj = *reinterpret_cast<dyn_object*>(value);
		if(expr->execute_bool({std::cref(obj)}, expression::exec_info()))
		{
			return source->insert_dyn(type, value);
		}
	}else if(type == typeid(std::pair<const dyn_object, dyn_object>))
	{
		auto &obj = *reinterpret_cast<std::pair<const dyn_object, dyn_object>*>(value);
		if(expr->execute_bool({std::cref(obj.second), std::cref(obj.first)}, expression::exec_info()))
		{
			return source->insert_dyn(type, value);
		}
	}
	return false;
}

bool filter_iterator::insert_dyn(const std::type_info &type, const void *value)
{
	if(type == typeid(dyn_object))
	{
		auto &obj = *reinterpret_cast<const dyn_object*>(value);
		if(expr->execute_bool({std::ref(obj)}, expression::exec_info()))
		{
			return source->insert_dyn(type, value);
		}
	}else if(type == typeid(std::pair<const dyn_object, dyn_object>))
	{
		auto &obj = *reinterpret_cast<const std::pair<const dyn_object, dyn_object>*>(value);
		if(expr->execute_bool({std::ref(obj.second), std::ref(obj.first)}, expression::exec_info()))
		{
			return source->insert_dyn(type, value);
		}
	}
	return false;
}

dyn_iterator *filter_iterator::get()
{
	return this;
}

const dyn_iterator *filter_iterator::get() const
{
	return this;
}

bool project_iterator::expired() const
{
	return source->expired();
}

bool project_iterator::valid() const
{
	return source->valid();
}

bool project_iterator::move_next()
{
	return source->move_next();
}

bool project_iterator::move_previous()
{
	return source->move_previous();
}

bool project_iterator::set_to_first()
{
	return source->set_to_first();
}

bool project_iterator::set_to_last()
{
	return source->set_to_last();
}

bool project_iterator::reset()
{
	return source->reset();
}

bool project_iterator::erase(bool stay)
{
	return source->erase(stay);
}

bool project_iterator::can_reset() const
{
	return source->can_reset();
}

bool project_iterator::can_erase() const
{
	return source->can_erase();
}

bool project_iterator::can_insert() const
{
	return false;
}

std::unique_ptr<dyn_iterator> project_iterator::clone() const
{
	return std::make_unique<project_iterator>(*this);
}

std::shared_ptr<dyn_iterator> project_iterator::clone_shared() const
{
	return std::make_shared<project_iterator>(*this);
}

size_t project_iterator::get_hash() const
{
	return source->get_hash();
}

bool project_iterator::operator==(const dyn_iterator &obj) const
{
	auto other = dynamic_cast<const project_iterator*>(&obj);
	if(other != nullptr)
	{
		if(valid())
		{
			return other->valid() && source == other->source && expr == other->expr;
		}else{
			return !other->valid();
		}
	}
	return false;
}

bool project_iterator::extract_dyn(const std::type_info &type, void *value) const
{
	if(type == typeid(std::shared_ptr<const dyn_object>))
	{
		return value_read(source.get(), [&](const dyn_object &obj)
		{
			*reinterpret_cast<std::shared_ptr<const dyn_object>*>(value) = std::make_shared<dyn_object>(expr->execute({std::ref(obj)}, expression::exec_info()));
			return true;
		});
	}else if(type == typeid(std::shared_ptr<const std::pair<const dyn_object, dyn_object>>))
	{
		return value_read(source.get(), [&](const dyn_object &val)
		{
			return key_read(source.get(), [&](const dyn_object &key)
			{
				*reinterpret_cast<std::shared_ptr<const std::pair<const dyn_object, dyn_object>>*>(value) = std::make_shared<std::pair<const dyn_object, dyn_object>>(key, expr->execute({std::ref(val), std::ref(key)}, expression::exec_info()));
				return true;
			});
		});
	}
	return false;
}

bool project_iterator::insert_dyn(const std::type_info &type, void *value)
{
	return false;
}

bool project_iterator::insert_dyn(const std::type_info &type, const void *value)
{
	return false;
}

dyn_iterator *project_iterator::get()
{
	return this;
}

const dyn_iterator *project_iterator::get() const
{
	return this;
}
