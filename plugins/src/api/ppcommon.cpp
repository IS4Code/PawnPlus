#include "ppcommon.h"

/*
PawnPlus hooks amx_Flags and responds to certain values.
The AMX header has magic set to hook_magic, which causes
the standard implementation to reject the call.
The plugin shall respond to hook_magic, and if the initial
flags value is set to hook_flags_initial, the pointer to
the function table is returned, and hook_flags_final is
stored as the flags.
*/

namespace pp
{
	main_table main;
	tag_table tag;
	dyn_object_table dyn_object;
	list_table list;
	linked_list_table linked_list;
	map_table map;
	string_table string;
	variant_table variant;
	task_table task;
}

bool pp::load()
{
	AMX fake_amx;
	AMX_HEADER fake_header;
	fake_amx.base = reinterpret_cast<unsigned char*>(&fake_header);
	fake_header.magic = hook_magic;
	uint16_t flags = hook_flags_initial;

	int result = amx_Flags(&fake_amx, &flags);
	if(flags == hook_flags_final)
	{
		auto table = reinterpret_cast<void***>(result);

		main.load(table[0]);
		tag.load(table[1]);
		dyn_object.load(table[2]);
		list.load(table[3]);
		linked_list.load(table[4]);
		map.load(table[5]);
		string.load(table[6]);
		variant.load(table[7]);
		task.load(table[8]);
		return true;
	}
	return false;
}

void pp::api_table::load(void **ptr)
{
	_ptr = ptr;
	for(_size = 0; ptr[_size]; _size++);
}
