#ifndef RESET_H_INCLUDED
#define RESET_H_INCLUDED

#include "context.h"
#include "amxinfo.h"

#include <memory>
#include "sdk/amx/amx.h"

namespace amx
{
	struct reset
	{
		cell cip, frm, pri, alt, hea, reset_hea, stk, reset_stk;
		std::unique_ptr<unsigned char[]> heap, stack;
		amx::context context;

		amx::handle amx;

		reset() = default;
		reset(AMX* amx, bool context);
		reset(reset &&obj);
		reset &operator=(reset &&obj);

		bool restore();
		bool restore_no_context() const;
	};
}

#endif
