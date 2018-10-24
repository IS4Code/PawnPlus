#include "reset.h"
#include "fixes/linux.h"
#include <cstring>

namespace amx
{
	reset::reset(AMX* amx, bool context, restore_range restore_heap, restore_range restore_stack) : amx(amx::load(amx)), cip(amx->cip), frm(amx->frm), pri(amx->pri), alt(amx->alt), hea(amx->hea), reset_hea(amx->reset_hea), stk(amx->stk), reset_stk(amx->reset_stk), restore_heap(restore_heap), restore_stack(restore_stack)
	{
		if(context)
		{
			amx::object owner;
			this->context = std::move(amx::get_context(amx, owner));
		}

		unsigned char *dat;

		auto amxhdr = (AMX_HEADER*)amx->base;
		dat = amx->data != nullptr ? amx->data : amx->base + amxhdr->dat;

		size_t heap_size = hea - amx->hlw;
		if(heap_size > 0)
		{
			unsigned char *h;
			switch(restore_heap)
			{
				case restore_range::none:
					h = dat;
					heap_size = 0;
					break;
				case restore_range::frame:
				case restore_range::context:
					h = dat + reset_hea;
					heap_size -= reset_hea - amx->hlw;
					if(heap_size < 0)
					{
						heap_size = 0;
					}
					break;
				case restore_range::full:
					h = dat + amx->hlw;
					break;
			}
			heap = std::make_unique<unsigned char[]>(heap_size);
			std::memcpy(heap.get(), h, heap_size);
		}

		size_t stack_size = amx->stp - stk;
		if(stack_size > 0)
		{
			unsigned char *s = dat + stk;
			switch(restore_stack)
			{
				case restore_range::none:
					stack_size = 0;
					break;
				case restore_range::frame:
					stack_size -= amx->stp - frm;
					break;
				case restore_range::context:
					stack_size -= amx->stp - reset_stk;
					if(stack_size < 0)
					{
						stack_size = 0;
					}
					break;
			}
			stack = std::make_unique<unsigned char[]>(stack_size);
			std::memcpy(stack.get(), s, stack_size);
		}
	}

	bool reset::restore()
	{
		if(!restore_no_context()) return false;
		auto obj = amx.lock();
		if(!obj || !obj->valid()) return false;
		auto amx = obj->get();
		amx::restore(amx, std::move(context));
		return true;
	}

	bool reset::restore_no_context() const
	{
		auto obj = amx.lock();
		if(!obj || !obj->valid()) return false;

		auto amx = obj->get();

		unsigned char *dat;

		auto amxhdr = (AMX_HEADER*)amx->base;
		dat = amx->data != nullptr ? amx->data : amx->base + amxhdr->dat;

		amx->cip = cip;
		amx->frm = frm;
		amx->pri = pri;
		amx->alt = alt;
		amx->hea = hea;
		amx->reset_hea = reset_hea;
		amx->stk = stk;
		amx->reset_stk = reset_stk;

		if(heap)
		{
			unsigned char *h;
			size_t heap_size;
			switch(restore_heap)
			{
				case restore_range::none:
					h = dat;
					heap_size = 0;
					break;
				case restore_range::frame:
				case restore_range::context:
					h = dat + reset_hea;
					heap_size = hea - reset_hea;
					if(heap_size < 0)
					{
						heap_size = 0;
					}
					break;
				case restore_range::full:
					h = dat + amx->hlw;
					heap_size = hea - amx->hlw;
					break;
			}
			std::memcpy(h, heap.get(), heap_size);
		}

		if(stack)
		{
			unsigned char *s = dat + stk;
			size_t stack_size;
			switch(restore_stack)
			{
				case restore_range::none:
					stack_size = 0;
					break;
				case restore_range::frame:
					stack_size = frm - stk;
					break;
				case restore_range::context:
					stack_size = reset_stk - stk;
					break;
				case restore_range::full:
					stack_size = amx->stp - stk;
					break;
			}
			std::memcpy(s, stack.get(), stack_size);
		}

		return true;
	}

	reset::reset(reset &&obj) : context(std::move(obj.context)), amx(obj.amx), cip(obj.cip), frm(obj.frm), pri(obj.pri), alt(obj.alt), hea(obj.hea), reset_hea(obj.reset_hea), heap(std::move(obj.heap)), stk(obj.stk), reset_stk(obj.reset_stk), stack(std::move(obj.stack)), restore_heap(obj.restore_heap), restore_stack(obj.restore_stack)
	{

	}

	reset &reset::operator=(reset &&obj)
	{
		if(this == &obj) return *this;

		context = std::move(obj.context);
		amx = obj.amx;
		cip = obj.cip, frm = obj.frm, pri = obj.pri, alt = obj.alt;
		stk = obj.stk, reset_stk = obj.reset_stk;
		hea = obj.hea, reset_hea = obj.reset_hea;
		heap = std::move(obj.heap);
		stack = std::move(obj.stack);
		restore_heap = obj.restore_heap;
		restore_stack = obj.restore_stack;
		return *this;
	}
}
