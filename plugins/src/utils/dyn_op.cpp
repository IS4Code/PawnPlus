#include "dyn_op.h"

cell op_add<cell>::operator()(cell a, cell b) const
{
	return a + b;
}

float op_add<float>::operator()(float a, float b) const
{
	return a + b;
}

strings::cell_string *op_add<strings::cell_string*>::operator()(strings::cell_string *a, strings::cell_string *b) const
{
	if(a == nullptr && b == nullptr)
	{
		return strings::pool.add(true);
	}
	if(a == nullptr)
	{
		auto str = *b;
		return strings::pool.add(std::move(str), true);
	}
	if(b == nullptr)
	{
		auto str = *a;
		return strings::pool.add(std::move(str), true);
	}
	auto str = *a + *b;
	return strings::pool.add(std::move(str), true);
}

dyn_object *op_add<dyn_object*>::operator()(dyn_object *a, dyn_object *b) const
{
	if(a == nullptr || b == nullptr)
	{
		return nullptr;
	}
	auto var = *a + *b;
	if(var.empty()) return nullptr;
	return variants::pool.add(std::move(var), true);
}

cell op_sub<cell>::operator()(cell a, cell b) const
{
	return a - b;
}

float op_sub<float>::operator()(float a, float b) const
{
	return a - b;
}

dyn_object *op_sub<dyn_object*>::operator()(dyn_object *a, dyn_object *b) const
{
	if(a == nullptr || b == nullptr)
	{
		return nullptr;
	}
	auto var = *a - *b;
	if(var.empty()) return nullptr;
	return variants::pool.add(std::move(var), true);
}

cell op_mul<cell>::operator()(cell a, cell b) const
{
	return a * b;
}

float op_mul<float>::operator()(float a, float b) const
{
	return a * b;
}

dyn_object *op_mul<dyn_object*>::operator()(dyn_object *a, dyn_object *b) const
{
	if(a == nullptr || b == nullptr)
	{
		return nullptr;
	}
	auto var = *a * *b;
	if(var.empty()) return nullptr;
	return variants::pool.add(std::move(var), true);
}

cell op_div<cell>::operator()(cell a, cell b) const
{
	return a / b;
}

float op_div<float>::operator()(float a, float b) const
{
	return a / b;
}

dyn_object *op_div<dyn_object*>::operator()(dyn_object *a, dyn_object *b) const
{
	if(a == nullptr || b == nullptr)
	{
		return nullptr;
	}
	auto var = *a / *b;
	if(var.empty()) return nullptr;
	return variants::pool.add(std::move(var), true);
}

cell op_mod<cell>::operator()(cell a, cell b) const
{
	return a % b;
}

float op_mod<float>::operator()(float a, float b) const
{
	return std::fmod(a, b);
}

strings::cell_string *op_mod<strings::cell_string*>::operator()(strings::cell_string *a, strings::cell_string *b) const
{
	if(a == nullptr && b == nullptr)
	{
		return strings::pool.add(true);
	}
	if(a == nullptr)
	{
		auto str = *b;
		return strings::pool.add(std::move(str), true);
	}
	if(b == nullptr)
	{
		auto str = *a;
		return strings::pool.add(std::move(str), true);
	}
	auto str = *a + *b;
	return strings::pool.add(std::move(str), true);
}

dyn_object *op_mod<dyn_object*>::operator()(dyn_object *a, dyn_object *b) const
{
	if(a == nullptr || b == nullptr)
	{
		return nullptr;
	}
	auto var = *a % *b;
	if(var.empty()) return nullptr;
	return variants::pool.add(std::move(var), true);
}

strings::cell_string op_strval<cell>::operator()(cell obj) const
{
	return strings::convert(std::to_string(obj));
}

strings::cell_string op_strval<bool>::operator()(cell obj) const
{
	if(obj == 0)
	{
		return strings::convert("false");
	}else if(obj == 1)
	{
		return strings::convert("true");
	}else{
		return strings::convert("bool:"+std::to_string(obj));
	}
}

strings::cell_string op_strval<char>::operator()(cell obj) const
{
	return strings::cell_string(1, obj);
}

strings::cell_string op_strval<float>::operator()(float obj) const
{
	return strings::convert(std::to_string(obj));
}

strings::cell_string op_strval<strings::cell_string*>::operator()(strings::cell_string *obj) const
{
	return *obj;
}

strings::cell_string op_strval<dyn_object*>::operator()(dyn_object *obj) const
{
	strings::cell_string str;
	str.append({'('});
	str.append(obj->to_string());
	str.append({')'});
	return str;
}
