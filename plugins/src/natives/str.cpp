#include "natives.h"
#include "errors.h"
#include "modules/containers.h"
#include "modules/strings.h"
#include "modules/format.h"
#include "modules/regex.h"
#include "modules/variants.h"
#include "modules/expressions.h"
#include "objects/dyn_object.h"

#include <cstring>
#include <algorithm>
#include <limits>

typedef strings::cell_string cell_string;

namespace Natives
{
	// native print_s(ConstStringTag:string);
	AMX_DEFINE_NATIVE_TAG(print_s, 1, cell)
	{
		if(params[1] == 0)
		{
			logprintf("");
		}else{
			cell_string *str;
			if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
			if(str->size() == 0)
			{
				logprintf("");
			}else{
				std::string msg;
				msg.reserve(str->size());
				for(const auto &c : *str)
				{
					msg.append(1, static_cast<unsigned char>(c));
				}
				logprintf("%s", msg.c_str());
			}
		}
		return 1;
	}

	// native String:str_new(const str[], str_create_mode:mode=str_preserve);
	AMX_DEFINE_NATIVE_TAG(str_new, 1, string)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		int flags = optparam(2, 0);
		return strings::create(addr, flags & 1, flags & 2);
	}

	// native String:str_new_arr(const arr[], size=sizeof(arr), str_create_mode:mode=str_preserve);
	AMX_DEFINE_NATIVE_TAG(str_new_arr, 2, string)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		cell size = params[2];
		if(size < 0) amx_LogicError(errors::out_of_range, "size");
		if(size == 0) return strings::pool.get_id(strings::pool.add());
		int flags = optparam(3, 0);
		return strings::create(addr, size, false, flags & 1, flags & 2);
	}

	// native String:str_new_static(const str[], str_create_mode:mode=str_preserve, size=sizeof(str));
	AMX_DEFINE_NATIVE_TAG(str_new_static, 3, string)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		int flags = params[2];
		cell size = params[3];
		if(size < 0) amx_LogicError(errors::out_of_range, "size");
		if(size == 0) return strings::pool.get_id(strings::pool.add());
		bool packed = static_cast<ucell>(*addr) > UNPACKEDMAX;
		if(packed)
		{
			cell last = addr[size - 1];
			size_t rem = 0;
			if(last & 0xFF) rem = 4;
			else if(last & 0xFF00) rem = 3;
			else if(last & 0xFF0000) rem = 2;
			else if(last & 0xFF000000) rem = 1;

			size = (size - 1) * sizeof(cell) + rem;
		}else{
			size -= 1;
		}
		if(size < 0) size = 0;
		return strings::create(addr, size, packed, flags & 1, flags & 2);
	}

	// native String:str_new_buf(size);
	AMX_DEFINE_NATIVE_TAG(str_new_buf, 1, string)
	{
		cell size = params[1];
		if(size <= 0) amx_LogicError(errors::out_of_range, "size");
		return strings::pool.get_id(strings::pool.emplace(size - 1, 0));
	}

	// native AmxString:str_addr(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_addr, 1, cell)
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return strings::pool.get_null_address(amx);
		}
		return strings::pool.get_address(amx, *str);
	}

	// native ConstAmxString:str_addr_const(ConstStringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_addr_const, 1, cell)
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return strings::pool.get_null_address(amx);
		}
		return strings::pool.get_address(amx, *str);
	}

	// native AmxStringBuffer:str_buf_addr(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_buf_addr, 1, cell)
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		return strings::pool.get_inner_address(amx, *str);
	}

	// native String:str_acquire(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_acquire, 1, string)
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(!strings::pool.acquire_ref(*str)) amx_LogicError(errors::cannot_acquire, "string", params[1]);
		return params[1];
	}

	// native String:str_release(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_release, 1, string)
	{
		decltype(strings::pool)::ref_container *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(!strings::pool.release_ref(*str)) amx_LogicError(errors::cannot_release, "string", params[1]);
		return params[1];
	}

	// native str_delete(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_delete, 1, cell)
	{
		if(!strings::pool.remove_by_id(params[1])) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		return 1;
	}

	// native bool:str_valid(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_valid, 1, bool)
	{
		cell_string *str;
		return strings::pool.get_by_id(params[1], str);
	}

	// native String:str_clone(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_clone, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		return strings::pool.get_id(strings::pool.emplace(*str));
	}


	// native String:str_cat(StringTag:str1, StringTag:str2);
	AMX_DEFINE_NATIVE_TAG(str_cat, 2, string)
	{
		cell_string *str1, *str2;
		if((!strings::pool.get_by_id(params[1], str1) && str1 != nullptr)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if((!strings::pool.get_by_id(params[2], str2) && str2 != nullptr)) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		
		if(str1 == nullptr && str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		if(str1 == nullptr)
		{
			return strings::pool.get_id(strings::pool.emplace(*str2));
		}
		if(str2 == nullptr)
		{
			return strings::pool.get_id(strings::pool.emplace(*str1));
		}

		auto str = *str1 + *str2;
		return strings::pool.get_id(strings::pool.add(std::move(str)));
	}

	// native String:str_val(AnyTag:val, TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(str_val, 2, string)
	{
		dyn_object obj{amx, params[1], params[2]};
		return strings::pool.get_id(strings::pool.add(obj.to_string()));
	}

	// native String:str_val_arr(const AnyTag:value[], size=sizeof(value), TagTag:tag_id=tagof(value));
	AMX_DEFINE_NATIVE_TAG(str_val_arr, 3, string)
	{
		cell *addr = amx_GetAddrSafe(amx, params[1]);
		dyn_object obj{amx, addr, params[2], params[3]};
		return strings::pool.get_id(strings::pool.add(obj.to_string()));
	}

	// native String:str_val_var(ConstVariantTag:value);
	AMX_DEFINE_NATIVE_TAG(str_val_var, 1, string)
	{
		dyn_object *var;
		if(!variants::pool.get_by_id(params[1], var) && var != nullptr) amx_LogicError(errors::pointer_invalid, "variant", params[1]);
		if(!var)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		return strings::pool.get_id(strings::pool.add(var->to_string()));
	}

	template <class Iter>
	struct str_split_base
	{
		cell operator()(Iter delims_begin, Iter delims_end, AMX *amx, cell_string *str) const
		{
			auto list = list_pool.add();

			cell_string::size_type last_pos = 0;
			while(last_pos != cell_string::npos)
			{
				auto it = std::find_first_of(str->begin() + last_pos, str->end(), delims_begin, delims_end);

				size_t pos;
				if(it == str->end())
				{
					pos = cell_string::npos;
				}else{
					pos = std::distance(str->begin(), it);
				}

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
	};

	// native List:str_split(StringTag:str, const delims[]);
	AMX_DEFINE_NATIVE_TAG(str_split, 2, list)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return list_pool.get_id(list_pool.add());
		}
		cell *delims = amx_GetAddrSafe(amx, params[2]);
		return strings::select_iterator<str_split_base>(delims, amx, str);
	}

	// native List:str_split_s(StringTag:str, StringTag:delims);
	AMX_DEFINE_NATIVE_TAG(str_split_s, 2, list)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *delims;
		if(!strings::pool.get_by_id(params[2], delims) && delims != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		return strings::select_iterator<str_split_base>(delims, amx, str);
	}

	template <class Iter>
	struct str_join_base
	{
		cell operator()(Iter delim_begin, Iter delim_end, AMX *amx, list_t *list) const
		{
			auto &str = strings::pool.add();
			bool first = true;
			for(auto &obj : *list)
			{
				if(first)
				{
					first = false;
				}else{
					str->append(delim_begin, delim_end);
				}
				str->append(obj.to_string());
			}

			return strings::pool.get_id(str);
		}
	};

	// native String:str_join(List:list, const delim[]);
	AMX_DEFINE_NATIVE_TAG(str_join, 2, string)
	{
		list_t *list;
		if(!list_pool.get_by_id(params[1], list)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell *delim = amx_GetAddrSafe(amx, params[2]);
		return strings::select_iterator<str_join_base>(delim, amx, list);
	}

	// native String:str_join_s(List:list, StringTag:delim);
	AMX_DEFINE_NATIVE_TAG(str_join_s, 2, string)
	{
		list_t *list;
		if(!list_pool.get_by_id(params[1], list)) amx_LogicError(errors::pointer_invalid, "list", params[1]);
		cell_string *delim;
		if(!strings::pool.get_by_id(params[1], delim) && delim != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		return strings::select_iterator<str_join_base>(delim, amx, list);
	}

	// native str_len(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_len, 1, cell)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr) return 0;
		return static_cast<cell>(str->size());
	}

	// native str_get(StringTag:str, buffer[], size=sizeof(buffer), start=0, end=cellmax);
	AMX_DEFINE_NATIVE_TAG(str_get, 3, cell)
	{
		if(params[3] == 0) return 0;

		cell *addr = amx_GetAddrSafe(amx, params[2]);

		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		if(str == nullptr)
		{
			addr[0] = 0;
			return 0;
		}

		cell start = optparam(4, 0);
		cell end = optparam(5, std::numeric_limits<cell>::max());

		if(!strings::clamp_range(*str, start, end))
		{
			return 0;
		}

		cell len = end - start;
		if(len >= params[3])
		{
			len = params[3] - 1;
		}

		if(len >= 0)
		{
			std::memcpy(addr, &(*str)[start], len * sizeof(cell));
			addr[len] = 0;
			return len;
		}
		return 0;
	}

	// native str_getc(StringTag:str, pos);
	AMX_DEFINE_NATIVE_TAG(str_getc, 2, cell)
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
	AMX_DEFINE_NATIVE_TAG(str_setc, 3, cell)
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
	AMX_DEFINE_NATIVE_TAG(str_set, 2, string)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		if(str2 == nullptr)
		{
			str1->clear();
		}else{
			str1->assign(*str2);
		}
		return params[1];
	}

	// native String:str_append(StringTag:target, StringTag:other);
	AMX_DEFINE_NATIVE_TAG(str_append, 2, string)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		if(str2 != nullptr)
		{
			str1->append(*str2);
		}
		return params[1];
	}

	// native String:str_ins(StringTag:target, StringTag:other, pos);
	AMX_DEFINE_NATIVE_TAG(str_ins, 3, string)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		if(str2 != nullptr)
		{
			strings::clamp_pos(*str1, params[3]);
			str1->insert(params[3], *str2);
		}
		return params[1];
	}

	// native String:str_del(StringTag:target, start=0, end=cellmax);
	AMX_DEFINE_NATIVE_TAG(str_del, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell start = optparam(2, 0);
		cell end = optparam(3, std::numeric_limits<cell>::max());

		if(strings::clamp_range(*str, start, end))
		{
			str->erase(start, end - start);
		}else{
			str->erase(start, -1);
			str->erase(0, end);
		}
		return params[1];
	}

	// native String:str_sub(StringTag:str, start=0, end=cellmax);
	AMX_DEFINE_NATIVE_TAG(str_sub, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr) return strings::pool.get_id(strings::pool.add());

		cell start = optparam(2, 0);
		cell end = optparam(3, std::numeric_limits<cell>::max());

		if(strings::clamp_range(*str, start, end))
		{
			auto substr = str->substr(start, end - start);
			return strings::pool.get_id(strings::pool.add(std::move(substr)));
		}
		return 0;
	}

	// native bool:str_cmp(StringTag:str1, StringTag:str2);
	AMX_DEFINE_NATIVE_TAG(str_cmp, 2, bool)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

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
	AMX_DEFINE_NATIVE_TAG(str_empty, 1, bool)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr) return 1;
		return str->empty();
	}

	// native bool:str_eq(StringTag:str1, StringTag:str2);
	AMX_DEFINE_NATIVE_TAG(str_eq, 2, bool)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2) && str2 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

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
	AMX_DEFINE_NATIVE_TAG(str_findc, 2, cell)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr) return -1;

		cell offset = optparam(3, 0);
		strings::clamp_pos(*str, offset);
		return str->find(params[2], static_cast<size_t>(offset));
	}

	// native str_find(StringTag:str, StringTag:value, offset=0);
	AMX_DEFINE_NATIVE_TAG(str_find, 2, cell)
	{
		cell_string *str1;
		if(!strings::pool.get_by_id(params[1], str1) && str1 != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string *str2;
		if(!strings::pool.get_by_id(params[2], str2)) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		if(str1 == nullptr) return str2->empty() ? 0 : -1;

		cell offset = optparam(3, 0);
		strings::clamp_pos(*str1, offset);

		return static_cast<cell>(str1->find(*str2, static_cast<size_t>(offset)));
	}

	// native String:str_clear(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_clear, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str != nullptr)
		{
			str->clear();
		}
		return params[1];
	}

	// native String:str_resize(StringTag:str, size, padding=0);
	AMX_DEFINE_NATIVE_TAG(str_resize, 2, string)
	{
		if(params[2] < 0) amx_LogicError(errors::out_of_range, "size");
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && (params[2] != 0 || str != nullptr)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str != nullptr)
		{
			str->resize(static_cast<size_t>(params[2]), optparam(3, 0));
		}
		return params[1];
	}

	// native String:str_format(const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_format, 1, string)
	{
		cell *format = amx_GetAddrSafe(amx, params[1]);

		cell_string target;
		strings::format(amx, target, format, params[0] / sizeof(cell) - 1, params + 2);
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_format_s(StringTag:format, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_format_s, 1, string)
	{
		cell_string *strformat;
		if(!strings::pool.get_by_id(params[1], strformat) && strformat != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		cell_string target;
		if(strformat != nullptr)
		{
			strings::format(amx, target, *strformat, params[0] / sizeof(cell) - 1, params + 2);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_set_format(StringTag:target, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_set_format, 2, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		str->clear();

		cell *format = amx_GetAddrSafe(amx, params[2]);

		strings::format(amx, *str, format, params[0] / sizeof(cell) - 2, params + 3);
		return params[1];
	}

	// native String:str_set_format_s(StringTag:target, StringTag:format, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_set_format_s, 2, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		str->clear();
		
		cell_string *strformat;
		if(!strings::pool.get_by_id(params[2], strformat) && strformat != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		if(strformat != nullptr)
		{
			strings::format(amx, *str, *strformat, params[0] / sizeof(cell) - 2, params + 3);
		}
		return params[1];
	}

	// native String:str_append_format(StringTag:target, const format[], AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_append_format, 2, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell *format = amx_GetAddrSafe(amx, params[2]);

		strings::format(amx, *str, format, params[0] / sizeof(cell) - 2, params + 3);
		return params[1];
	}

	// native String:str_append_format_s(StringTag:target, StringTag:format, AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_append_format_s, 2, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell_string *strformat;
		if(!strings::pool.get_by_id(params[2], strformat) && strformat != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);
		if(strformat != nullptr)
		{
			strings::format(amx, *str, *strformat, params[0] / sizeof(cell) - 2, params + 3);
		}
		return params[1];
	}

	// native String:str_to_lower(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_to_lower, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		auto str2 = *str;
		std::transform(str2.begin(), str2.end(), str2.begin(), strings::to_lower);
		return strings::pool.get_id(strings::pool.add(std::move(str2)));
	}

	// native String:str_to_upper(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_to_upper, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str == nullptr)
		{
			return strings::pool.get_id(strings::pool.add());
		}
		auto str2 = *str;
		std::transform(str2.begin(), str2.end(), str2.begin(), strings::to_upper);
		return strings::pool.get_id(strings::pool.add(std::move(str2)));
	}

	// native String:str_set_to_lower(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_set_to_lower, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str != nullptr)
		{
			std::transform(str->begin(), str->end(), str->begin(), strings::to_lower);
		}
		return params[1];
	}

	// native String:str_set_to_upper(StringTag:str);
	AMX_DEFINE_NATIVE_TAG(str_set_to_upper, 1, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		if(str != nullptr)
		{
			std::transform(str->begin(), str->end(), str->begin(), strings::to_upper);
		}
		return params[1];
	}

	// native bool:str_match(ConstStringTag:str, const pattern[], &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_match, 2, bool)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		cell *pos = optparamref(3, 0);
		cell options = optparam(4, 0);

		if(str != nullptr)
		{
			return strings::regex_search(*str, pattern, pos, options);
		}else{
			return strings::regex_search(cell_string(), pattern, pos, options);
		}
	}

	// native bool:str_match_s(ConstStringTag:str, ConstStringTag:pattern, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_match_s, 2, bool)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pos = optparamref(3, 0);
		cell options = optparam(4, 0);

		if(str != nullptr && pattern != nullptr)
		{
			return strings::regex_search(*str, *pattern, pos, options);
		}else{
			cell_string blank;
			return strings::regex_search(str != nullptr ? *str : blank, pattern != nullptr ? *pattern : blank, pos, options);
		}
	}

	// native List:str_extract(ConstStringTag:str, const pattern[], &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_extract, 2, list)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		cell *pos = optparamref(3, 0);
		cell options = optparam(4, 0);

		if(str != nullptr)
		{
			return strings::regex_extract(*str, pattern, pos, options);
		}else{
			return strings::regex_extract(cell_string(), pattern, pos, options);
		}
	}

	// native List:str_extract_s(ConstStringTag:str, ConstStringTag:pattern, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_extract_s, 2, list)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pos = optparamref(3, 0);
		cell options = optparam(4, 0);

		if(str != nullptr && pattern != nullptr)
		{
			return strings::regex_extract(*str, *pattern, pos, options);
		}else{
			cell_string empty;
			return strings::regex_extract(str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, pos, options);
		}
	}

	// native String:str_replace(ConstStringTag:str, const pattern[], const replacement[], &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		cell *replacement = amx_GetAddrSafe(amx, params[3]);

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr)
		{
			strings::regex_replace(target, *str, pattern, replacement, pos, options);
		}else{
			strings::regex_replace(target, cell_string(), pattern, replacement, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_s(ConstStringTag:str, ConstStringTag:pattern, ConstStringTag:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace_s, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell_string *replacement;
		if(!strings::pool.get_by_id(params[3], replacement) && replacement != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[3]);

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr && pattern != nullptr && replacement != nullptr)
		{
			strings::regex_replace(target, *str, *pattern, *replacement, pos, options);
		}else{
			cell_string empty;
			strings::regex_replace(target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, replacement != nullptr ? *replacement : empty, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_list(ConstStringTag:str, const pattern[], List:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace_list, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		list_t *replacement;
		if(!list_pool.get_by_id(params[3], replacement)) amx_LogicError(errors::pointer_invalid, "list", params[3]);

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr)
		{
			strings::regex_replace(target, *str, pattern, *replacement, pos, options);
		}else{
			strings::regex_replace(target, cell_string(), pattern, *replacement, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_list_s(ConstStringTag:str, ConstStringTag:pattern, List:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace_list_s, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		list_t *replacement;
		if(!list_pool.get_by_id(params[3], replacement)) amx_LogicError(errors::pointer_invalid, "list", params[3]);

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr && pattern != nullptr)
		{
			strings::regex_replace(target, *str, *pattern, *replacement, pos, options);
		}else{
			cell_string empty;
			strings::regex_replace(target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, *replacement, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_func(ConstStringTag:str, const pattern[], const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_replace_func, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		const char *fname;
		amx_StrParam(amx, params[3], fname);
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		int index;
		if(amx_FindPublicSafe(amx, fname, &index) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public", fname);
		}

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		const char *format;
		amx_OptStrParam(amx, 6, format, nullptr);

		cell_string target;
		if(str != nullptr)
		{
			strings::regex_replace(target, *str, pattern, amx, index, pos, options, format, params + 7, params[0] / sizeof(cell) - 6);
		}else{
			strings::regex_replace(target, cell_string(), pattern, amx, index, pos, options, format, params + 7, params[0] / sizeof(cell) - 6);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_func_s(ConstStringTag:str, ConstStringTag:pattern, const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_replace_func_s, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		const char *fname;
		amx_StrParam(amx, params[3], fname);
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		int index;
		if(amx_FindPublicSafe(amx, fname, &index) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public", fname);
		}

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		const char *format;
		amx_OptStrParam(amx, 6, format, nullptr);

		cell_string target;
		if(str != nullptr && pattern != nullptr)
		{
			strings::regex_replace(target, *str, *pattern, amx, index, pos, options, format, params + 7, params[0] / sizeof(cell) - 6);
		}else{
			cell_string empty;
			strings::regex_replace(target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, amx, index, pos, options, format, params + 7, params[0] / sizeof(cell) - 6);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_expr(ConstStringTag:str, const pattern[], Expression:expr, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace_expr, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell *pattern = amx_GetAddrSafe(amx, params[2]);

		expression *expr;
		if(!expression_pool.get_by_id(params[3], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[3]);
		
		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr)
		{
			strings::regex_replace(target, *str, pattern, amx, *expr, pos, options);
		}else{
			strings::regex_replace(target, cell_string(), pattern, amx, *expr, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_replace_expr_s(ConstStringTag:str, ConstStringTag:pattern, Expression:expr, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_replace_expr_s, 3, string)
	{
		cell_string *str;
		if(!strings::pool.get_by_id(params[1], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[2], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		expression *expr;
		if(!expression_pool.get_by_id(params[3], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[3]);

		cell *pos = optparamref(4, 0);
		cell options = optparam(5, 0);

		cell_string target;
		if(str != nullptr && pattern != nullptr)
		{
			strings::regex_replace(target, *str, *pattern, amx, *expr, pos, options);
		}else{
			cell_string empty;
			strings::regex_replace(target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, amx, *expr, pos, options);
		}
		return strings::pool.get_id(strings::pool.add(std::move(target)));
	}

	// native String:str_set_replace(StringTag:target, ConstStringTag:str, const pattern[], const replacement[], &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pattern = amx_GetAddrSafe(amx, params[3]);

		cell *replacement = amx_GetAddrSafe(amx, params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			strings::regex_replace(*target, tmp, pattern, replacement, pos, options);
		}else{
			target->clear();
			if(str != nullptr)
			{
				strings::regex_replace(*target, *str, pattern, replacement, pos, options);
			}else{
				strings::regex_replace(*target, cell_string(), pattern, replacement, pos, options);
			}
		}
		return params[1];
	}

	// native String:str_set_replace_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, ConstStringTag:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_s, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[3], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[3]);

		cell_string *replacement;
		if(!strings::pool.get_by_id(params[4], replacement) && replacement != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			if(pattern != nullptr && replacement != nullptr)
			{
				strings::regex_replace(*target, tmp, *pattern, *replacement, pos, options);
			}else{
				cell_string empty;
				strings::regex_replace(*target, tmp, pattern != nullptr ? *pattern : empty, replacement != nullptr ? *replacement : empty, pos, options);
			}
		}else{
			target->clear();
			if(str != nullptr && pattern != nullptr && replacement != nullptr)
			{
				strings::regex_replace(*target, *str, *pattern, *replacement, pos, options);
			}else{
				cell_string empty;
				strings::regex_replace(*target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, replacement != nullptr ? *replacement : empty, pos, options);
			}
		}

		return params[1];
	}

	// native String:str_set_replace_list(StringTag:target, ConstStringTag:str, const pattern[], List:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_list, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pattern = amx_GetAddrSafe(amx, params[3]);

		list_t *replacement;
		if(!list_pool.get_by_id(params[4], replacement)) amx_LogicError(errors::pointer_invalid, "list", params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			strings::regex_replace(*target, tmp, pattern, *replacement, pos, options);
		}else{
			target->clear();
			if(str != nullptr)
			{
				strings::regex_replace(*target, *str, pattern, *replacement, pos, options);
			}else{
				strings::regex_replace(*target, cell_string(), pattern, *replacement, pos, options);
			}
		}
		return params[1];
	}

	// native String:str_set_replace_list_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, List:replacement, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_list_s, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[3], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[3]);

		list_t *replacement;
		if(!list_pool.get_by_id(params[4], replacement)) amx_LogicError(errors::pointer_invalid, "list", params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			if(pattern != nullptr)
			{
				strings::regex_replace(*target, tmp, *pattern, *replacement, pos, options);
			}else{
				strings::regex_replace(*target, tmp, cell_string(), *replacement, pos, options);
			}
		}else{
			target->clear();
			if(str != nullptr && pattern != nullptr)
			{
				strings::regex_replace(*target, *str, *pattern, *replacement, pos, options);
			}else{
				cell_string empty;
				strings::regex_replace(*target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, *replacement, pos, options);
			}
		}

		return params[1];
	}

	// native String:str_set_replace_func(StringTag:target, ConstStringTag:str, const pattern[], const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_func, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pattern = amx_GetAddrSafe(amx, params[3]);

		const char *fname;
		amx_StrParam(amx, params[4], fname);
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		int index;
		if(amx_FindPublicSafe(amx, fname, &index) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public", fname);
		}

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		const char *format;
		amx_OptStrParam(amx, 7, format, nullptr);
		
		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			strings::regex_replace(*target, tmp, pattern, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
		}else{
			target->clear();
			if(str != nullptr)
			{
				strings::regex_replace(*target, *str, pattern, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}else{
				strings::regex_replace(*target, cell_string(), pattern, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}
		}
		return params[1];
	}

	// native String:str_set_replace_func_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, const function[], &pos=0, regex_options:options=regex_default, const additional_format[]="", AnyTag:...);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_func_s, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[3], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[3]);

		const char *fname;
		amx_StrParam(amx, params[4], fname);
		if(fname == nullptr)
		{
			amx_FormalError(errors::arg_empty, "function");
		}
		int index;
		if(amx_FindPublicSafe(amx, fname, &index) != AMX_ERR_NONE)
		{
			amx_FormalError(errors::func_not_found, "public", fname);
		}

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		const char *format;
		amx_OptStrParam(amx, 7, format, nullptr);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			if(pattern != nullptr)
			{
				strings::regex_replace(*target, tmp, *pattern, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}else{
				strings::regex_replace(*target, tmp, cell_string(), amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}
		}else{
			target->clear();
			if(str != nullptr && pattern != nullptr)
			{
				strings::regex_replace(*target, *str, *pattern, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}else{
				cell_string empty;
				strings::regex_replace(*target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, amx, index, pos, options, format, params + 8, params[0] / sizeof(cell) - 7);
			}
		}

		return params[1];
	}

	// native String:str_set_replace_expr(StringTag:target, ConstStringTag:str, const pattern[], Expression:expr, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_expr, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);
		
		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell *pattern = amx_GetAddrSafe(amx, params[3]);

		expression *expr;
		if(!expression_pool.get_by_id(params[4], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			strings::regex_replace(*target, tmp, pattern, amx, *expr, pos, options);
		}else{
			target->clear();
			if(str != nullptr)
			{
				strings::regex_replace(*target, *str, pattern, amx, *expr, pos, options);
			}else{
				strings::regex_replace(*target, cell_string(), pattern, amx, *expr, pos, options);
			}
		}
		return params[1];
	}

	// native String:str_set_replace_expr_s(StringTag:target, ConstStringTag:str, ConstStringTag:pattern, Expression:expr, &pos=0, regex_options:options=regex_default);
	AMX_DEFINE_NATIVE_TAG(str_set_replace_expr_s, 4, string)
	{
		cell_string *target;
		if(!strings::pool.get_by_id(params[1], target)) amx_LogicError(errors::pointer_invalid, "string", params[1]);

		cell_string *str;
		if(!strings::pool.get_by_id(params[2], str) && str != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[2]);

		cell_string *pattern;
		if(!strings::pool.get_by_id(params[3], pattern) && pattern != nullptr) amx_LogicError(errors::pointer_invalid, "string", params[3]);

		expression *expr;
		if(!expression_pool.get_by_id(params[4], expr)) amx_LogicError(errors::pointer_invalid, "expression", params[4]);

		cell *pos = optparamref(5, 0);
		cell options = optparam(6, 0);

		if(target == str)
		{
			cell_string tmp;
			std::swap(tmp, *target);
			if(pattern != nullptr)
			{
				strings::regex_replace(*target, tmp, *pattern, amx, *expr, pos, options);
			}else{
				strings::regex_replace(*target, *str, cell_string(), amx, *expr, pos, options);
			}
		}else{
			target->clear();
			if(str != nullptr && pattern != nullptr)
			{
				strings::regex_replace(*target, *str, *pattern, amx, *expr, pos, options);
			}else{
				cell_string empty;
				strings::regex_replace(*target, str != nullptr ? *str : empty, pattern != nullptr ? *pattern : empty, amx, *expr, pos, options);
			}
		}

		return params[1];
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(print_s),

	AMX_DECLARE_NATIVE(str_new),
	AMX_DECLARE_NATIVE(str_new_arr),
	AMX_DECLARE_NATIVE(str_new_static),
	AMX_DECLARE_NATIVE(str_new_buf),
	AMX_DECLARE_NATIVE(str_addr),
	AMX_DECLARE_NATIVE(str_addr_const),
	AMX_DECLARE_NATIVE(str_buf_addr),
	AMX_DECLARE_NATIVE(str_acquire),
	AMX_DECLARE_NATIVE(str_release),
	AMX_DECLARE_NATIVE(str_delete),
	AMX_DECLARE_NATIVE(str_valid),
	AMX_DECLARE_NATIVE(str_clone),

	AMX_DECLARE_NATIVE(str_len),
	AMX_DECLARE_NATIVE(str_get),
	AMX_DECLARE_NATIVE(str_getc),
	AMX_DECLARE_NATIVE(str_setc),
	AMX_DECLARE_NATIVE(str_cmp),
	AMX_DECLARE_NATIVE(str_empty),
	AMX_DECLARE_NATIVE(str_eq),
	AMX_DECLARE_NATIVE(str_findc),
	AMX_DECLARE_NATIVE(str_find),

	AMX_DECLARE_NATIVE(str_cat),
	AMX_DECLARE_NATIVE(str_sub),
	AMX_DECLARE_NATIVE(str_val),
	AMX_DECLARE_NATIVE(str_val_arr),
	AMX_DECLARE_NATIVE(str_val_var),
	AMX_DECLARE_NATIVE(str_split),
	AMX_DECLARE_NATIVE(str_split_s),
	AMX_DECLARE_NATIVE(str_join),
	AMX_DECLARE_NATIVE(str_join_s),
	AMX_DECLARE_NATIVE(str_to_lower),
	AMX_DECLARE_NATIVE(str_to_upper),

	AMX_DECLARE_NATIVE(str_set),
	AMX_DECLARE_NATIVE(str_append),
	AMX_DECLARE_NATIVE(str_ins),
	AMX_DECLARE_NATIVE(str_del),
	AMX_DECLARE_NATIVE(str_clear),
	AMX_DECLARE_NATIVE(str_resize),
	AMX_DECLARE_NATIVE(str_set_to_lower),
	AMX_DECLARE_NATIVE(str_set_to_upper),

	AMX_DECLARE_NATIVE(str_format),
	AMX_DECLARE_NATIVE(str_format_s),
	AMX_DECLARE_NATIVE(str_set_format),
	AMX_DECLARE_NATIVE(str_set_format_s),
	AMX_DECLARE_NATIVE(str_append_format),
	AMX_DECLARE_NATIVE(str_append_format_s),

	AMX_DECLARE_NATIVE(str_match),
	AMX_DECLARE_NATIVE(str_match_s),
	AMX_DECLARE_NATIVE(str_extract),
	AMX_DECLARE_NATIVE(str_extract_s),
	AMX_DECLARE_NATIVE(str_replace),
	AMX_DECLARE_NATIVE(str_replace_s),
	AMX_DECLARE_NATIVE(str_replace_list),
	AMX_DECLARE_NATIVE(str_replace_list_s),
	AMX_DECLARE_NATIVE(str_replace_func),
	AMX_DECLARE_NATIVE(str_replace_func_s),
	AMX_DECLARE_NATIVE(str_replace_expr),
	AMX_DECLARE_NATIVE(str_replace_expr_s),
	AMX_DECLARE_NATIVE(str_set_replace),
	AMX_DECLARE_NATIVE(str_set_replace_s),
	AMX_DECLARE_NATIVE(str_set_replace_list),
	AMX_DECLARE_NATIVE(str_set_replace_list_s),
	AMX_DECLARE_NATIVE(str_set_replace_func),
	AMX_DECLARE_NATIVE(str_set_replace_func_s),
	AMX_DECLARE_NATIVE(str_set_replace_expr),
	AMX_DECLARE_NATIVE(str_set_replace_expr_s),
};

int RegisterStringsNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
