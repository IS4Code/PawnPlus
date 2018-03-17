#ifndef RESET_H_INCLUDED
#define RESET_H_INCLUDED

#include "context.h"

#include <memory>
#include "sdk/amx/amx.h"

class AMX_RESET
{
	cell cip, frm, pri, alt, hea, reset_hea, stk, reset_stk;
	std::unique_ptr<unsigned char[]> heap, stack;
	AMX_CONTEXT context;

public:
	AMX* amx;

	AMX_RESET() = default;
	AMX_RESET(AMX* amx);
	AMX_RESET(AMX_RESET &&obj);
	AMX_RESET &operator=(AMX_RESET &&obj);

	void restore() const;
	void restore_no_context() const;
};

#endif
