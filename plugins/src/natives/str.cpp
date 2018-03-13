#include "../natives.h"
#include "../strings.h"
#include <cstring>

namespace Natives
{
	// native String:str_new(const str[], bool:wide = true);
	static cell AMX_NATIVE_CALL str_new(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		int flags = params[2];
		auto str = strings::create(addr, true, flags & 1, flags & 2);
		return reinterpret_cast<cell>(str);
	}

	// native String:str_new_arr(const arr[], size=sizeof(arr), bool:wide=true);
	static cell AMX_NATIVE_CALL str_new_arr(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		int flags = params[2];
		auto str = strings::create(addr, true, static_cast<size_t>(params[2]), flags & 1, flags & 2);
		return reinterpret_cast<cell>(str);
	}

	// native AmxString:str_addr(StringTag:str);
	static cell AMX_NATIVE_CALL str_addr(AMX *amx, cell *params)
	{
		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		return strings::pool.get_address(amx, str);
	}

	// native AmxStringBuffer:str_buf_addr(StringTag:str);
	static cell AMX_NATIVE_CALL str_buf_addr(AMX *amx, cell *params)
	{
		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		return strings::pool.get_inner_address(amx, str);
	}

	// native GlobalString:str_to_global(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_global(AMX *amx, cell *params)
	{
		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		strings::pool.move_to_global(str);
		return params[1];
	}

	// native String:str_to_local(StringTag:str);
	static cell AMX_NATIVE_CALL str_to_local(AMX *amx, cell *params)
	{
		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		strings::pool.move_to_local(str);
		return params[1];
	}

	// native bool:str_free(&StringTag:str);
	static cell AMX_NATIVE_CALL str_free(AMX *amx, cell *params)
	{
		cell *addr;
		amx_GetAddr(amx, params[1], &addr);
		bool ok = strings::pool.free(reinterpret_cast<strings::cell_string*>(*addr));
		*addr = 0;
		return static_cast<cell>(ok);
	}

	// native bool:str_is_valid(StringTag:str);
	static cell AMX_NATIVE_CALL str_is_valid(AMX *amx, cell *params)
	{
		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		return static_cast<cell>(strings::pool.is_valid(str));
	}


	// native String:str_cat(StringTag:str1, StringTag:str2);
	static cell AMX_NATIVE_CALL str_cat(AMX *amx, cell *params)
	{
		if(params[1] == 0)
		{
			auto str = *reinterpret_cast<strings::cell_string*>(params[2]);
			return reinterpret_cast<cell>(strings::pool.add(std::move(str), true));
		}
		if(params[2] == 0)
		{
			auto str = *reinterpret_cast<strings::cell_string*>(params[1]);
			return reinterpret_cast<cell>(strings::pool.add(std::move(str), true));
		}

		auto str = *reinterpret_cast<strings::cell_string*>(params[1]) + *reinterpret_cast<strings::cell_string*>(params[2]);
		return reinterpret_cast<cell>(strings::pool.add(std::move(str), true));
	}

	// native String:str_int(val);
	static cell AMX_NATIVE_CALL str_int(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(strings::create(std::to_string(static_cast<int>(params[1])), true));
	}

	// native String:str_float(Float:val);
	static cell AMX_NATIVE_CALL str_float(AMX *amx, cell *params)
	{
		return reinterpret_cast<cell>(strings::create(std::to_string(amx_ctof(params[1])), true));
	}

	// native str_len(StringTag:str);
	static cell AMX_NATIVE_CALL str_len(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		return static_cast<cell>(str->size());
	}

	// native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_get(AMX *amx, cell *params)
	{
		if(params[3] == 0) return 0;

		cell *addr;
		amx_GetAddr(amx, params[2], &addr);

		if(params[1] == 0)
		{
			addr[0] = 0;
			return 0;
		}

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);

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
		if(params[1] == 0) return 0;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);

		if(strings::clamp_pos(*str, params[2]))
		{
			return (*str)[params[2]];
		}
		return 0xFFFFFF00;
	}

	// native str_setc(StringTag:str, pos, value);
	static cell AMX_NATIVE_CALL str_setc(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);

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
		if(params[1] == 0) return params[1];

		auto str1 = reinterpret_cast<strings::cell_string*>(params[1]);

		if(params[2] == 0)
		{
			str1->clear();
		} else {
			auto str2 = reinterpret_cast<strings::cell_string*>(params[2]);
			str1->assign(*str2);
		}
		return params[1];
	}

	// native String:str_append(StringTag:target, StringTag:other);
	static cell AMX_NATIVE_CALL str_append(AMX *amx, cell *params)
	{
		if(params[1] == 0 || params[2] == 0) return params[1];

		auto str1 = reinterpret_cast<strings::cell_string*>(params[1]);
		auto str2 = reinterpret_cast<strings::cell_string*>(params[2]);
		str1->append(*str2);
		return params[1];
	}

	// native String:str_del(StringTag:target, start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_del(AMX *amx, cell *params)
	{
		if(params[1] == 0) return params[1];

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);

		if(strings::clamp_range(*str, params[2], params[3]))
		{
			str->erase(params[2], params[3] - params[2]);
		} else {
			str->erase(params[2], -1);
			str->erase(0, params[3]);
		}
		return params[1];
	}

	// native String:str_sub(StringTag:str, start=0, end=cellmax);
	static cell AMX_NATIVE_CALL str_sub(AMX *amx, cell *params)
	{
		if(params[1] == 0) return reinterpret_cast<cell>(strings::pool.add(true));

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		if(strings::clamp_range(*str, params[2], params[3]))
		{
			auto substr = str->substr(params[2], params[3] - params[2]);
			return reinterpret_cast<cell>(strings::pool.add(std::move(substr), true));
		}
		return 0;
	}

	// native bool:str_cmp(StringTag:str1, StringTag:str2);
	static cell AMX_NATIVE_CALL str_cmp(AMX *amx, cell *params)
	{
		if(params[1] == 0 && params[2] == 0) return true;

		auto str1 = reinterpret_cast<strings::cell_string*>(params[1]);
		auto str2 = reinterpret_cast<strings::cell_string*>(params[2]);
		if(str1 == nullptr)
		{
			return str2->size() == 0;
		}
		if(str2 == nullptr)
		{
			return str1->size() == 0;
		}
		return static_cast<cell>(str1->compare(*str2));
	}

	// native bool:str_empty(StringTag:str);
	static cell AMX_NATIVE_CALL str_empty(AMX *amx, cell *params)
	{
		if(params[1] == 0) return true;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		return static_cast<cell>(str->empty());
	}

	// native String:str_clear(StringTag:str);
	static cell AMX_NATIVE_CALL str_clear(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		str->clear();
		return params[1];
	}

	// native String:str_clone(StringTag:str);
	static cell AMX_NATIVE_CALL str_clone(AMX *amx, cell *params)
	{
		if(params[1] == 0) return reinterpret_cast<cell>(strings::pool.add(true));

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
		auto str2 = *str;
		return reinterpret_cast<cell>(strings::pool.add(std::move(str2), true));
	}

	// native String:str_resize(StringTag:str, capacity, padding=0);
	static cell AMX_NATIVE_CALL str_resize(AMX *amx, cell *params)
	{
		if(params[1] == 0) return 0;

		auto str = reinterpret_cast<strings::cell_string*>(params[1]);
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

		auto str = strings::format(amx, format, flen, params[0] - 1, params + 2, true);
		return reinterpret_cast<cell>(str);
	}

	// native String:str_format_s(StringTag:format, {StringTags,Float,_}:...);
	static cell AMX_NATIVE_CALL str_format_s(AMX *amx, cell *params)
	{
		auto strformat = reinterpret_cast<strings::cell_string*>(params[1]);
		auto str = strings::format(amx, &(*strformat)[0], strformat->size(), params[0] - 1, params + 2, true);
		return reinterpret_cast<cell>(str);
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(str_new),
	AMX_DECLARE_NATIVE(str_new_arr),
	AMX_DECLARE_NATIVE(str_addr),
	AMX_DECLARE_NATIVE(str_buf_addr),
	AMX_DECLARE_NATIVE(str_to_global),
	AMX_DECLARE_NATIVE(str_free),
	AMX_DECLARE_NATIVE(str_is_valid),

	AMX_DECLARE_NATIVE(str_len),
	AMX_DECLARE_NATIVE(str_get),
	AMX_DECLARE_NATIVE(str_getc),
	AMX_DECLARE_NATIVE(str_setc),
	AMX_DECLARE_NATIVE(str_cmp),
	AMX_DECLARE_NATIVE(str_empty),

	AMX_DECLARE_NATIVE(str_int),
	AMX_DECLARE_NATIVE(str_float),
	AMX_DECLARE_NATIVE(str_cat),
	AMX_DECLARE_NATIVE(str_sub),
	AMX_DECLARE_NATIVE(str_clone),

	AMX_DECLARE_NATIVE(str_set),
	AMX_DECLARE_NATIVE(str_append),
	AMX_DECLARE_NATIVE(str_del),
	AMX_DECLARE_NATIVE(str_clear),
	AMX_DECLARE_NATIVE(str_resize),

	AMX_DECLARE_NATIVE(str_format),
	AMX_DECLARE_NATIVE(str_format_s),
};

int RegisterStringsNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
