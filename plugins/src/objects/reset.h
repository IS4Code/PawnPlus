#ifndef RESET_H_INCLUDED
#define RESET_H_INCLUDED

#include "context.h"
#include "amxinfo.h"

#include <memory>
#include "sdk/amx/amx.h"

namespace amx
{
	enum class restore_range
	{
		none = 0,
		frame = 1,
		context = 2,
		full = 3,
	};

	struct reset
	{
		cell cip, frm, pri, alt, hea, reset_hea, stk, reset_stk;
		restore_range restore_heap, restore_stack;
		std::unique_ptr<unsigned char[]> heap, stack;
		amx::context context;

		amx::handle amx;

		reset() = default;
		reset(AMX *amx, bool context, restore_range restore_heap, restore_range restore_stack);
		reset(AMX *amx, bool context) : reset(amx, context, restore_range::full, restore_range::full)
		{

		}
		reset(reset &&obj);
		reset &operator=(reset &&obj);

		bool restore();
		bool restore_no_context() const;
	};
}

#endif
