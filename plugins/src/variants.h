#ifndef VARIANTS_H_INCLUDED
#define VARIANTS_H_INCLUDED

#include "utils/object_pool.h"
#include "utils/dyn_object.h"
#include "sdk/amx/amx.h"

namespace variants
{
	extern object_pool<dyn_object> pool;

	cell create(dyn_object &&obj);
	dyn_object get(cell ptr);

	template <class... Args>
	cell create(Args&&... args)
	{
		return create(dyn_object(std::forward<Args>(args)...));
	}
}

#endif
