#include "tags.h"
#include "amxinfo.h"
#include <unordered_map>
#include <vector>

std::vector<tag_info*> tag_list = {
	new tag_info(0, " ", nullptr),
	new tag_info(1, "", nullptr),
	new tag_info(2, "bool", nullptr),
	new tag_info(3, "char", nullptr),
	new tag_info(4, "Float", nullptr),
	new tag_info(5, "String", nullptr),
	new tag_info(6, "Variant", nullptr),
	new tag_info(7, "List", nullptr),
	new tag_info(8, "Map", nullptr),
	new tag_info(9, "ListIterator", nullptr),
	new tag_info(10, "MapIterator", nullptr),
	new tag_info(11, "Ref", nullptr),
};

struct tag_map_info : public amx::extra
{
	std::unordered_map<cell, tag_ptr> tag_map;

	tag_map_info(AMX *amx) : amx::extra(amx)
	{
		int len;
		amx_NameLength(amx, &len);
		char *tag_name = static_cast<char*>(alloca(len + 1));

		int num;
		amx_NumTags(amx, &num);
		for(int i = 0; i < num; i++)
		{
			cell tag_id;
			if(!amx_GetTag(amx, i, tag_name, &tag_id))
			{
				auto info = tags::find_tag(tag_name, -1);
				tag_map[tag_id & 0x7FFFFFFF] = info;
			}
		}
	}
};

tag_ptr tags::find_tag(const char *name, size_t sublen)
{
	std::string tag_name = sublen == -1 ? std::string(name) : std::string(name, sublen);

	for(auto &tag : ::tag_list)
	{
		if(tag->name == tag_name) return tag;
	}

	size_t pos = std::string::npos, npos = -1;
	while(true)
	{
		npos = tag_name.find('@', npos + 1);
		if(npos == std::string::npos) break;
		pos = npos;
	}

	tag_ptr base = nullptr;
	if(pos != std::string::npos)
	{
		base = find_tag(name, pos);
	}
	cell id = ::tag_list.size();
	::tag_list.push_back(new tag_info(id, std::move(tag_name), base));
	return ::tag_list[id];
}

tag_ptr tags::find_tag(AMX *amx, cell tag_id)
{
	tag_id &= 0x7FFFFFFF;
	if(tag_id == 0) return ::tag_list[tag_cell];

	auto obj = amx::load_lock(amx);
	auto &map = obj->get_extra<tag_map_info>().tag_map;
	auto it = map.find(tag_id);
	if(it != map.end())
	{
		return it->second;
	}
	return ::tag_list[tag_unknown];
}

tag_ptr tags::find_tag(cell tag_uid)
{
	if(tag_uid < 0 || (ucell)tag_uid >= ::tag_list.size()) return ::tag_list[tag_unknown];
	return ::tag_list[tag_uid];
}

bool tag_info::strong() const
{
	if(name.size() == 0) return false;
	char c = name[0];
	return c >= 'A' && c <= 'Z';
}

bool tag_info::inherits_from(cell parent) const
{
	tag_ptr test = this;
	while(test != nullptr)
	{
		if(test->uid == parent) return true;
		test = test->base;
	}
	return false;
}

bool tag_info::inherits_from(tag_ptr parent) const
{
	tag_ptr test = this;
	while(test != nullptr)
	{
		if(test == parent) return true;
		test = test->base;
	}
	return false;
}

tag_ptr tag_info::find_top_base() const
{
	tag_ptr test = this;
	while(test->base != nullptr)
	{
		test = test->base;
	}
	return test;
}

bool tag_info::same_base(tag_ptr tag) const
{
	return find_top_base() == tag->find_top_base();
}

cell tag_info::get_id(AMX *amx) const
{
	if(uid == tags::tag_cell) return 0x80000000;
	auto obj = amx::load_lock(amx);
	auto &map = obj->get_extra<tag_map_info>().tag_map;
	for(auto &pair : map)
	{
		if(pair.second == this) return pair.first | 0x80000000;
	}
	return 0;
}
