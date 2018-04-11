#include "tags.h"
#include <unordered_map>
#include <vector>

std::unordered_map<AMX*, std::unordered_map<cell, tag_ptr>> tag_map;
std::vector<tag_info*> tag_list = {
	new tag_info(0, "", nullptr),
	new tag_info(1, "bool", nullptr),
	new tag_info(2, "char", nullptr),
	new tag_info(3, "Float", nullptr),
	new tag_info(4, "String", nullptr),
	new tag_info(5, "Variant", nullptr),
};

void tags::load(AMX *amx)
{
	auto &map = tag_map[amx];

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
			auto info = find_tag(tag_name, -1);
			map[tag_id & 0x7FFFFFFF] = info;
		}
	}
}

void tags::unload(AMX *amx)
{
	tag_map.erase(amx);
}

tag_ptr tags::find_tag(const char *name, size_t sublen)
{
	std::string tag_name = sublen == -1 ? std::string(name) : std::string(name, sublen);

	for(auto &tag : tag_list)
	{
		if(tag->name == tag_name) return tag;
	}

	size_t pos = tag_name.find('@');
	tag_ptr base = nullptr;
	if(pos != std::string::npos)
	{
		base = find_tag(name, pos);
	}
	cell id = tag_list.size();
	tag_list.push_back(new tag_info(id, std::move(tag_name), base));
	return tag_list[id];
}

tag_ptr tags::find_tag(AMX *amx, cell tag_id)
{
	tag_id &= 0x7FFFFFFF;
	if(tag_id == 0) return tag_list[tag_cell];

	auto &map = tag_map[amx];
	auto it = map.find(tag_id);
	if(it != map.end())
	{
		return it->second;
	}
	return nullptr;
}

tag_ptr tags::find_tag(cell tag_uid)
{
	if(tag_uid < 0 || (ucell)tag_uid >= tag_list.size()) return nullptr;
	return tag_list[tag_uid];
}
