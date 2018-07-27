#include "natives.h"
#include "modules/containers.h"
#include "modules/strings.h"
#include "objects/dyn_object.h"
#include <cstring>
#include <algorithm>

typedef strings::cell_string cell_string;

namespace Natives
{
	// native String:str_new(const str[], str_create_mode:mode=str_preserve);
	static cell AMX_NATIVE_CALL str_new(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		int flags = params[2];
		auto str = strings::create(addr, true, flags & 1, flags & 2);
		return strings::pool.get_id(str);
	}

	// native String:str_new_arr(const arr[], size=sizeof(arr), str_create_mode:mode=str_preserve);
	static cell AMX_NATIVE_CALL str_new_arr(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		int flags = params[3];
		auto str = strings::create(addr, true, static_cast<size_t>(params[2]), false, flags & 1, flags & 2);
		return strings::pool.get_id(str);
	}

	// native String:str_new_static(const str[], str_create_mode:mode=str_preserve, size=sizeof(str));
	static cell AMX_NATIVE_CALL str_new_static(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		int flags = params[2];
		cell size = (addr[0] & 0xFF000000) ? params[3] : params[3] - 1;
		if(size < 0) size = 0;
		auto str = strings::create(addr, true, static_cast<size_t>(size), true, flags & 1, flags & 2);
		return strings::pool.get_id(str);
	}

	// native String:str_new_buf(size);
	static cell AMX_NATIVE_CALL str_new_buf(AMX *amx, cell *params)
	{
		cell size = params[1];
		return strings::pool.get_id(strings::pool.add(cell_string(size - 1, 0), true));
	}

	// native AmxString:str_addr(StringTag:str);
	static cell AMX_NATIVE_CALL str_addr(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		return strings::pool.get_address(amx, str);
	}

	// native AmxStringBuffer:str_buf_addr(StringTag:str);
	static cell AMX_NATIVE_CALL str_buf_addr(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		return strings::pool.get_inner_address(amx, str);
	}

	// native GlobalString:str_to_global(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_global(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		strings::pool.move_to_global(str);
		return params[1];
	}

	// native String:str_to_local(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_local(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		strings::pool.move_to_local(str);
		return params[1];
	}

	// native bool:str_delete(StringTag:str);
	static cell AMX_NATIVE_CALL str_delete(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		return strings::pool.remove(str);
	}

	// native bool:str_valid(StringTag:str);
	static cell AMX_NATIVE_CALL str_valid(AMX *amx, cell *params)
	{
		cell_string *str;
		return strings::pool.get_by_id(params[1], str);
	}

	// native String:str_clone(StringTag:str);
	static cell AMX_NATIVE_CALL str_clone(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		return strings::pool.get_id(strings::pool.clone(str));
	}


	// native String:str_cat(StringTag:str1, StringTag:str2);
	static cell AMX_NATIVE_CALL str_cat(AMX *amx, cell *params)
	{
		cell_string *str1, *str2;
		if((!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) || (!strings::pool.get_by_id(params[2], str2) && str2 != nullptr))
		{
			return 0;
		}
		if(str1 == nullptr && str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add(true));
		}
		if(str1 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add(cell_string(*str2), true));
		}
		if(str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add(cell_string(*str1), true));
		}

		auto str = *str1 + *str2;
		return strings::pool.get_id(strings::pool.add(std::move(str), true));
	}

	// native String:str_val(AnyTag:val, tag=tagof(value));
	static cell AMX_NATIVE_CALL str_val(AMX *amx, cell *params)
	{
		dyn_object obj{amx, params[1], params[2]};
		return strings::pool.get_id(strings::pool.add(obj.to_string(), true));
	}

	// native String:str_val_arr(AnyTag:value[], size=sizeof(value), tag_id=tagof(value));
	static cell AMX_NATIVE_CALL str_val_arr(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		dyn_object obj{amx, addr, params[2], params[3]};
		return strings::pool.get_id(strings::pool.add(obj.to_string(), true));
	}
	static cell AMX_NATIVE_CALL str_split_base(AMX *amx, cell_string *str, const cell_string &delims)
	{
		auto list = list_pool.add();

		cell_string::size_type last_pos = 0;
		while(last_pos != cell_string::npos)
		{
			auto pos = str->find_first_of(delims, last_pos);

			auto sub = &(*str)[last_pos];
			cell_string::size_type size;
			if(pos != cell_string::npos)
			{
				size = pos - last_pos;
			}else{
				size = str->size() - last_pos;
			}
			cell old = sub[size];
			sub[size] = 0;
			list->push_back(dyn_object(sub, size + 1, tags::find_tag(tags::tag_char)));
			sub[size] = old;

			if(pos != cell_string::npos)
			{
				last_pos = pos + 1;
			}else{
				last_pos = cell_string::npos;
			}
		}

		return list_pool.get_id(list);
	}

	// native List:str_split(StringTag:str, const delims[]);
	static cell AMX_NATIVE_CALL str_split(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		if(str == nullptr)
		{
			return list_pool.get_id(list_pool.add());
		}
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		cell_string delims(addr);
		return str_split_base(amx, str, delims);
	}

	// native List:str_split_s(StringTag:str, StringTag:delims);
	static cell AMX_NATIVE_CALL str_split_s(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		cell_string *delims;
		if(!strings::pool.get_by_id(params[2], delims) && delims != nullptr) return 0;
		if(str == nullptr)
		{
			return list_pool.get_id(list_pool.add());
		}
		if(delims == nullptr)
		{
			return str_split_base(amx, str, cell_string());
		}else{
			return str_split_base(amx, str, *delims);
		}
	}

	static cell AMX_NATIVE_CALL str_join_base(AMX *amx, list_t *list, const cell_string &delim)
	{
		auto str = strings::pool.add(true);
		bool first = true;
		for(auto &obj : *list)
		{
			if(first)
			{
				first = false;
			} else {
				str->append(delim);
			}
			str->append(obj.to_string());
		}

		return strings::pool.get_id(str);
	}

	// native String:str_join(List:list, const delim[]);
	static cell AMX_NATIVE_CALL str_join(AMX *amx, cell *params)
	{
		list_t *list;
		if(!list_pool.get_by_id(params[1], list)) return 0;
		cell *addr;
		amx_GetAddr(amx, params[2], &addr);
		cell_string delim(addr);
		return str_join_base(amx, list, delim);
	}

	// native String:str_join_s(List:list, StringTag:delim);
	static cell AMX_NATIVE_CALL str_join_s(AMX *amx, cell *params)
	{
		list_t *list;
		if(!list_pool.get_by_id(params[1], list)) return 0;
		cell_string *delim;
		if(!strings::pool.get_by_id(params[1], delim) && delim != nullptr) return 0;
		if(delim == nullptr)
		{
			return str_join_base(amx, list, cell_string());
		}else{
			return str_join_base(amx, list, *delim);
		}
	}

	// native str_len(StringTag:str);
	static cell AMX_NATIVE_CALL str_len(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		return static_cast<cell>(str->size());
	}

	// native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_get(AMX *amx, cell *params)
	{
		if(params[3] == 0) return 0;

		cell *addr;
		amx_GetAddr(amx, params[2], &addr);

		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;

		if(str == nullptr)
		{
			addr[0] = 0;
			return 0;
		}

		if(!strings::clamp_range(*str, params[4], params[5]))
		{
			return 0;
		}

		cell len = params[5] - params[4];
		if(len >= params[3])
		{
			len = params[3] - 1;
		}

		if(len >= 0)
		{
			std::memcpy(addr, &(*str)[0], len * sizeof(cell));
			addr[len] = 0;
			return len;
		}
		return 0;
	}

	// native str_getc(StringTag:str, pos);
	static cell AMX_NATIVE_CALL str_getc(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0xFFFFFF00;

		if(strings::clamp_pos(*str, params[2]))
		{
			return (*str)[params[2]];
		}
		return 0xFFFFFF00;
	}

	// native str_setc(StringTag:str, pos, value);
	static cell AMX_NATIVE_CALL str_setc(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0xFFFFFF00;

		if(strings::clamp_pos(*str, params[2]))
		{
			cell c = (*str)[params[2]];
			(*str)[params[2]] = params[3];
			return c;
		}
		return 0xFFFFFF00;
	}

	// native String:str_set(StringTag:target, StringTag:other);
	static cell AMX_NATIVE_CALL str_set(AMX *amx, cell *params)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1)) return 0;
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) return 0;

		if(str2 == nullptr)
		{
			str1->clear();
		}else{
			str1->assign(*str2);
		}
		return params[1];
	}

	// native String:str_append(StringTag:target, StringTag:other);
	static cell AMX_NATIVE_CALL str_append(AMX *amx, cell *params)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1)) return 0;
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) return 0;
		if(str2 != nullptr)
		{
			str1->append(*str2);
		}
		return params[1];
	}

	// native String:str_del(StringTag:target, start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_del(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;

		if(strings::clamp_range(*str, params[2], params[3]))
		{
			str->erase(params[2], params[3] - params[2]);
		}else{
			str->erase(params[2], -1);
			str->erase(0, params[3]);
		}
		return params[1];
	}

	// native String:str_sub(StringTag:str, start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_sub(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		if(str == nullptr) return strings::pool.get_id(strings::pool.add(true));

		if(strings::clamp_range(*str, params[2], params[3]))
		{
			auto substr = str->substr(params[2], params[3] - params[2]);
			return strings::pool.get_id(strings::pool.add(std::move(substr), true));
		}
		return 0;
	}

	// native bool:str_cmp(StringTag:str1, StringTag:str2);
	static cell AMX_NATIVE_CALL str_cmp(AMX *amx, cell *params)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) return 0;
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) return 0;

		if(str1 == nullptr && str2 == nullptr) return 1;
		if(str1 == nullptr)
		{
			return str2->size() == 0;
		}
		if(str2 == nullptr)
		{
			return str1->size() == 0;
		}
		return str1->compare(*str2);
	}

	// native bool:str_empty(StringTag:str);
	static cell AMX_NATIVE_CALL str_empty(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		if(str == nullptr) return 1;
		return str->empty();
	}

	// native bool:str_equal(StringTag:str1, StringTag:str2);
	static cell AMX_NATIVE_CALL str_equal(AMX *amx, cell *params)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) return 0;
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) return 0;

		if(str1 == nullptr && str2 == nullptr) return 1;
		if(str1 == nullptr)
		{
			return str2->size() == 0;
		}
		if(str2 == nullptr)
		{
			return str1->size() == 0;
		}
		return *str1 == *str2;
	}

	// native str_findc(StringTag:str, value, offset=0);
	static cell AMX_NATIVE_CALL str_findc(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		if(str == nullptr) return -1;

		strings::clamp_pos(*str, params[3]);
		return str->find(params[2], static_cast<size_t>(params[3]));
	}

	// native str_find(StringTag:str, StringTag:value, offset=0);
	static cell AMX_NATIVE_CALL str_find(AMX *amx, cell *params)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) return 0;
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2)) return 0;
		if(str1 == nullptr) return str2->empty() ? 0 : -1;

		strings::clamp_pos(*str1, params[3]);

		return static_cast<cell>(str1->find(*str2, static_cast<size_t>(params[3])));
	}

	// native String:str_clear(StringTag:str);
	static cell AMX_NATIVE_CALL str_clear(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) return 0;
		str->clear();
		return params[1];
	}

	// native String:str_resize(StringTag:str, capacity, padding=0);
	static cell AMX_NATIVE_CALL str_resize(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		str->resize(static_cast<size_t>(params[2]), params[3]);
		return params[1];
	}

	// native String:str_format(const format[], {StringTags,Float,_}:...);
	static cell AMX_NATIVE_CALL str_format(AMX *amx, cell *params)
	{
		cell *format;
		amx_GetAddr(amx, params[1], &format);
		int flen;
		amx_StrLen(format, &flen);

		auto str = strings::pool.add(true);
		strings::format(amx, *str, format, flen, params[0] - 1, params + 2);
		return strings::pool.get_id(str);
	}

	// native String:str_format_s(StringTag:format, {StringTags,Float,_}:...);
	static cell AMX_NATIVE_CALL str_format_s(AMX *amx, cell *params)
	{
		cell_string *strformat;
		if(!strings::pool.get_by_id(params[1], strformat) && strformat != nullptr) return 0;
		auto str = strings::pool.add(true);
		if(strformat != nullptr)
		{
			strings::format(amx, *str, &(*strformat)[0], strformat->size(), params[0] / sizeof(cell) - 1, params + 2);
		}
		return strings::pool.get_id(str);
	}

	// native String:str_set_format(StringTag:target, const format[], {StringTags,Float,_}:...);
	static cell AMX_NATIVE_CALL str_set_format(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		str->clear();

		cell *format;
		amx_GetAddr(amx, params[2], &format);
		int flen;
		amx_StrLen(format, &flen);


		strings::format(amx, *str, format, flen, params[0] / sizeof(cell) - 2, params + 3);
		return params[1];
	}

	// native String:str_set_format_s(StringTag:target, StringTag:format, {StringTags,Float,_}:...);
	static cell AMX_NATIVE_CALL str_set_format_s(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		str->clear();
		
		cell_string *strformat;
		if(!strings::pool.get_by_id(params[2], strformat) && strformat != nullptr) return 0;
		if(strformat != nullptr)
		{
			strings::format(amx, *str, &(*strformat)[0], strformat->size(), params[0] / sizeof(cell) - 2, params + 3);
		}
		return params[1];
	}

	static cell to_lower(cell c)
	{
		if(65 <= c && c <= 90) return c + 32;
		return c;
	}

	static cell to_upper(cell c)
	{
		if(97 <= c && c <= 122) return c - 32;
		return c;
	}

	// native String:str_to_lower(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_lower(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		auto str2 = *str;
		std::transform(str2.begin(), str2.end(), str2.begin(), to_lower);
		return strings::pool.get_id(strings::pool.add(std::move(str2), true));
	}

	// native String:str_to_upper(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_upper(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		auto str2 = *str;
		std::transform(str2.begin(), str2.end(), str2.begin(), to_upper);
		return strings::pool.get_id(strings::pool.add(std::move(str2), true));
	}

	// native String:str_set_to_lower(StringTag:str);
	static cell AMX_NATIVE_CALL str_set_to_lower(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		std::transform(str->begin(), str->end(), str->begin(), to_lower);
		return params[1];
	}

	// native String:str_set_to_upper(StringTag:str);
	static cell AMX_NATIVE_CALL str_set_to_upper(AMX *amx, cell *params)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) return 0;
		std::transform(str->begin(), str->end(), str->begin(), to_upper);
		return params[1];
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(str_new),
	AMX_DECLARE_NATIVE(str_new_arr),
	AMX_DECLARE_NATIVE(str_new_static),
	AMX_DECLARE_NATIVE(str_new_buf),
	AMX_DECLARE_NATIVE(str_addr),
	AMX_DECLARE_NATIVE(str_buf_addr),
	AMX_DECLARE_NATIVE(str_to_global),
	AMX_DECLARE_NATIVE(str_delete),
	AMX_DECLARE_NATIVE(str_valid),
	AMX_DECLARE_NATIVE(str_clone),

	AMX_DECLARE_NATIVE(str_len),
	AMX_DECLARE_NATIVE(str_get),
	AMX_DECLARE_NATIVE(str_getc),
	AMX_DECLARE_NATIVE(str_setc),
	AMX_DECLARE_NATIVE(str_cmp),
	AMX_DECLARE_NATIVE(str_empty),
	AMX_DECLARE_NATIVE(str_equal),
	AMX_DECLARE_NATIVE(str_findc),
	AMX_DECLARE_NATIVE(str_find),

	AMX_DECLARE_NATIVE(str_cat),
	AMX_DECLARE_NATIVE(str_sub),
	AMX_DECLARE_NATIVE(str_val),
	AMX_DECLARE_NATIVE(str_val_arr),
	AMX_DECLARE_NATIVE(str_split),
	AMX_DECLARE_NATIVE(str_split_s),
	AMX_DECLARE_NATIVE(str_join),
	AMX_DECLARE_NATIVE(str_join_s),
	AMX_DECLARE_NATIVE(str_to_lower),
	AMX_DECLARE_NATIVE(str_to_upper),

	AMX_DECLARE_NATIVE(str_set),
	AMX_DECLARE_NATIVE(str_append),
	AMX_DECLARE_NATIVE(str_del),
	AMX_DECLARE_NATIVE(str_clear),
	AMX_DECLARE_NATIVE(str_resize),
	AMX_DECLARE_NATIVE(str_set_to_lower),
	AMX_DECLARE_NATIVE(str_set_to_upper),

	AMX_DECLARE_NATIVE(str_format),
	AMX_DECLARE_NATIVE(str_format_s),
	AMX_DECLARE_NATIVE(str_set_format),
	AMX_DECLARE_NATIVE(str_set_format_s),
};

int RegisterStringsNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
