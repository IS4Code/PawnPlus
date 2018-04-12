#ifndef TAGS_H_INCLUDED
#define TAGS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>

typedef const struct tag_info *tag_ptr;
struct tag_info
{
	cell uid;
	std::string name;
	tag_ptr base;

	tag_info(cell uid, std::string &&name, tag_ptr base) : uid(uid), name(std::move(name)), base(base)
	{

	}

	bool strong() const
	{
		if(name.size() == 0) return false;
		char c = name[0];
		return c >= 'A' && c <= 'Z';
	}

	bool inherits_from(cell parent) const
	{
		tag_ptr test = this;
		while(test != nullptr)
		{
			if(test->uid == parent) return true;
			test = test->base;
		}
		return false;
	}

	bool inherits_from(tag_ptr parent) const
	{
		tag_ptr test = this;
		while(test != nullptr)
		{
			if(test == parent) return true;
			test = test->base;
		}
		return false;
	}

	tag_ptr find_top_base() const
	{
		tag_ptr test = this;
		while(test->base != nullptr)
		{
			test = test->base;
		}
		return test;
	}

	bool same_base(tag_ptr tag) const
	{
		return find_top_base() == tag->find_top_base();
	}
};

namespace tags
{
	constexpr const cell tag_cell = 0;
	constexpr const cell tag_bool = 1;
	constexpr const cell tag_char = 2;
	constexpr const cell tag_float = 3;
	constexpr const cell tag_string = 4;
	constexpr const cell tag_variant = 5;

	void load(AMX *amx);
	void unload(AMX *amx);
	tag_ptr find_tag(const char *name, size_t sublen=-1);
	tag_ptr find_tag(AMX *amx, cell tag_id);
	tag_ptr find_tag(cell tag_uid);
}

#endif
