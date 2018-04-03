#include "variants.h"

object_pool<dyn_object> variants::pool;

cell variants::create(dyn_object &&obj)
{
	return reinterpret_cast<cell>(pool.add(std::move(obj), true));
}

dyn_object variants::get(cell ptr)
{
	if(ptr != 0) return *reinterpret_cast<dyn_object*>(ptr);
	return dyn_object();
}
