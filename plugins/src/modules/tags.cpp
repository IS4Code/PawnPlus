#include "tags.h"
#include "main.h"
#include "amxinfo.h"
#include "fixes/linux.h"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>

extern std::vector<std::unique_ptr<tag_info>> tag_list;

struct tag_map_info : public amx::extra
{
	std::unordered_map<cell, tag_ptr> tag_map;

	tag_map_info(AMX *amx) : amx::extra(amx)
	{
		char *tagname = amx_NameBuffer(amx);

		int num;
		amx_NumTags(amx, &num);
		for(int i = 0; i < num; i++)
		{
			cell tag_id;
			if(!amx_GetTag(amx, i, tagname, &tag_id))
			{
				auto info = tags::find_tag(tagname, -1);
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
		if(tag && tag->name == tag_name) return tag.get();
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
	if(base == nullptr)
	{
		base = ::tag_list[tags::tag_unknown].get();
	}
	cell id = ::tag_list.size();
	::tag_list.push_back(nullptr);

	auto ops = base->get_ops().derive(base, id, tag_name.c_str());
	::tag_list[id] = std::make_unique<tag_info>(id, std::move(tag_name), base, std::move(ops));
	return ::tag_list[id].get();
}

tag_ptr tags::find_existing_tag(const char *name, size_t sublen)
{
	std::string tag_name = sublen == -1 ? std::string(name) : std::string(name, sublen);

	for(auto &tag : ::tag_list)
	{
		if(tag && tag->name == tag_name) return tag.get();
	}

	return ::tag_list[tag_unknown].get();
}

tag_ptr tags::find_tag(AMX *amx, cell tag_id)
{
	if(tag_id != 0 && (tag_id & 0x80000000) == 0)
	{
		auto tag = find_tag(tag_id);
		if(tag->uid != tag_unknown) return tag;
	}
	tag_id &= 0x7FFFFFFF;
	if(tag_id == 0) return ::tag_list[tag_cell].get();

	auto obj = amx::load_lock(amx);
	auto &map = obj->get_extra<tag_map_info>().tag_map;
	auto it = map.find(tag_id);
	if(it != map.end())
	{
		return it->second;
	}
	char *tagname = amx_NameBuffer(amx);
	if(amx_FindTagId(amx, tag_id, tagname) == AMX_ERR_NONE)
	{
		return tags::find_tag(tagname, -1);
	}
	return ::tag_list[tag_unknown].get();
}

tag_ptr tags::find_tag(cell tag_uid)
{
	if(tag_uid < 0 || (ucell)tag_uid >= ::tag_list.size()) return ::tag_list[tag_unknown].get();
	return ::tag_list[tag_uid].get();
}

tag_ptr tags::new_tag(const char *name, cell base_id)
{
	std::string tag_name;
	tag_ptr base = tags::find_tag(base_id);
	if(base->uid != 0)
	{
		tag_name.append(base->name);
		tag_name.append(1, '@');
	}
	if(name)
	{
		tag_name.append(name);
	}
	tag_name.append(1, '+');

	cell id = ::tag_list.size();
	::tag_list.push_back(nullptr);
	auto ops = base->get_ops().derive(base, id, tag_name.c_str());
	auto tag = std::make_unique<tag_info>(id, std::move(tag_name), base, std::move(ops));
	tag->name.append(std::to_string(reinterpret_cast<std::uintptr_t>(tag.get())));
	while(find_existing_tag(tag->name.c_str())->uid != tag_unknown)
	{
		tag->name.append(1, '_');
	}
	auto ptr = tag.get();
	::tag_list[id] = std::move(tag);
	return ptr;
}

tag_info::tag_info(cell uid, std::string &&name, tag_ptr base, std::unique_ptr<tag_operations> &&ops) : uid(uid), name(std::move(name)), base(base), ops(std::move(ops))
{

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
	while(test->base != nullptr && test->base->uid != tags::tag_unknown)
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
	const auto &obj = amx::load_lock(amx);
	auto &map = obj->get_extra<tag_map_info>().tag_map;
	for(auto &pair : map)
	{
		if(pair.second == this) return pair.first | 0x80000000;
	}
	return uid;
}

const char *tag_info::format_name() const
{
	return uid == tags::tag_cell ? "_" : name.c_str();
}
