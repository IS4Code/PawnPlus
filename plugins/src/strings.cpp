#include "strings.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

using namespace Strings;

typedef std::vector<std::unique_ptr<cell_string>> list_type;

static list_type string_list;
static list_type tmp_string_list;

static std::unordered_map<const cell*, const cell_string*> string_cache;

typedef void(*logprintf_t)(char* format, ...);
extern logprintf_t logprintf;

cell Strings::NullValue1[1] = {0};
cell Strings::NullValue2[2] = {0, 1};

bool FindPtrInList(const list_type &list, const void *ptr);
cell_string *FindStrInList(const list_type &list, const cell *str);
bool FreeInList(list_type &list, const cell_string *str);

cell_string *Strings::Create(const cell *addr, bool temp, bool truncate, bool fixnulls)
{
	if(!addr[0])
	{
		return nullptr;
	}else if(addr[0] & 0xFF000000)
	{
		const char *c;
		size_t pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const char*>(addr) + pos / 4 + ipos;
		}while(*c);

		cell_string *str;
		if(temp)
		{
			str = tmp_string_list.emplace_back(new cell_string(pos, 0)).get();
		}else{
			str = string_list.emplace_back(new cell_string(pos, 0)).get();
		}

		pos = -1;
		do{
			pos++;
			size_t ipos = 3 - pos % 4;
			c = reinterpret_cast<const char*>(addr) + pos / 4 + ipos;
			(*str)[pos] = *c;
		}while(*c);

		return str;
	}else{
		size_t pos = -1;
		const cell *c;
		do{
			pos++;
			c = addr + pos;
		}while (*c);

		cell_string *str = new cell_string(addr, pos);
		if(truncate)
		{
			for(size_t i = 0; i < pos; i++)
			{
				cell &c = (*str)[i];
				c &= 0xFF;
				if(fixnulls && c == 0) c = 0x00FFFF00;
			}
		}
		if(temp)
		{
			tmp_string_list.emplace_back(str);
		}else{
			string_list.emplace_back(str);
		}

		return str;
	}
}

cell_string *Strings::Create(const cell *addr, bool temp, size_t length, bool truncate, bool fixnulls)
{
	cell_string *str = new cell_string(addr, length);
	if(truncate)
	{
		for(size_t i = 0; i < length; i++)
		{
			cell &c = (*str)[i];
			c &= 0xFF;
			if(fixnulls && c == 0) c = 0x00FFFF00;
		}
	}else if(fixnulls)
	{
		for(size_t i = 0; i < length; i++)
		{
			cell &c = (*str)[i];
			if(c == 0) c = 0x00FFFF00;
		}
	}
	if(temp)
	{
		tmp_string_list.emplace_back(str);
	}else{
		string_list.emplace_back(str);
	}
	return str;
}

cell_string *Strings::Add(const std::string &str, bool temp)
{
	size_t len = str.size();
	cell_string *cstr = new cell_string(len, '\0');
	for(size_t i = 0; i < len; i++)
	{
		(*cstr)[i] = static_cast<cell>(str[i]);
	}
	if(temp)
	{
		tmp_string_list.emplace_back(cstr);
	}else{
		string_list.emplace_back(cstr);
	}
	return cstr;
}

cell_string *Strings::Add(cell_string &&str, bool temp)
{
	cell_string *cstr = new cell_string(std::move(str));
	if(temp)
	{
		tmp_string_list.emplace_back(cstr);
	}else{
		string_list.emplace_back(cstr);
	}
	return cstr;
}

cell_string *Strings::Get(AMX *amx, cell addr)
{
	void *ptr = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat) + addr;

	if(FindPtrInList(tmp_string_list, ptr) || FindPtrInList(string_list, ptr))
	{
		return static_cast<cell_string*>(ptr);
	}
	return nullptr;
}

cell Strings::GetAddress(AMX *amx, const cell_string *str)
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(str) - reinterpret_cast<cell>(data);
}

bool Strings::IsNullAddress(AMX *amx, cell addr)
{
	unsigned char *data = (amx->data != nullptr ? amx->data : amx->base + ((AMX_HEADER*)amx->base)->dat);
	return reinterpret_cast<cell>(data) == -addr;
}

bool Strings::MoveToGlobal(const cell_string *str)
{
	auto it = tmp_string_list.begin();
	while(it != tmp_string_list.end())
	{
		if(it->get() == str)
		{
			auto ptr = std::move(*it);
			tmp_string_list.erase(it);
			string_list.emplace_back(std::move(ptr));
			return true;
		}
		it++;
	}
	return false;
}

bool Strings::MoveToLocal(const cell_string *str)
{
	auto it = string_list.begin();
	while(it != string_list.end())
	{
		if(it->get() == str)
		{
			auto ptr = std::move(*it);
			string_list.erase(it);
			tmp_string_list.emplace_back(std::move(ptr));
			return true;
		}
		it++;
	}
	return false;
}

void Strings::SetCache(const cell_string *str)
{
	string_cache[&(*str)[0]] = str;
}

const cell_string *Strings::FindCache(const cell *str)
{
	auto it = string_cache.find(str);
	if(it != string_cache.end())
	{
		return it->second;
	}
	return nullptr;
}

bool Strings::Free(const cell_string *str)
{
	if(!FreeInList(string_list, str))
	{
		return FreeInList(tmp_string_list, str);
	}
	return true;
}

bool Strings::IsValid(const cell_string *str)
{
	return FindPtrInList(tmp_string_list, str) || FindPtrInList(string_list, str);
}

void Strings::ClearCacheAndTemporary()
{
	string_cache.clear();
	tmp_string_list.clear();
}

bool Strings::ClampRange(const cell_string &str, cell &start, cell &end)
{
	ClampPos(str, start);
	ClampPos(str, end);

	return start <= end;
}

bool Strings::ClampPos(const cell_string &str, cell &pos)
{
	size_t size = str.size();
	if(pos < 0) pos += size;
	if(static_cast<size_t>(pos) >= size)
	{
		pos = size;
		return false;
	}
	return true;
}



bool FindPtrInList(const list_type &list, const void *ptr)
{
	auto it = list.begin();
	while(it != list.end())
	{
		if (it->get() == ptr)
		{
			return true;
		}
		it++;
	}
	return false;
}

cell_string *FindStrInList(const list_type &list, const cell *str)
{
	auto it = list.begin();
	while(it != list.end())
	{
		if (it->get()->c_str() == str)
		{
			return it->get();
		}
		it++;
	}
	return nullptr;
}

bool FreeInList(std::vector<std::unique_ptr<cell_string>> &list, const cell_string *str)
{
	auto it = list.begin();
	while(it != list.end())
	{
		if (it->get() == str)
		{
			list.erase(it);
			return true;
		}
		it++;
	}
	return false;
}
