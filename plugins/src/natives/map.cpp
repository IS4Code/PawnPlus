#include "../natives.h"
#include "../utils/dyn_object.h"
#include <unordered_map>

typedef std::unordered_map<dyn_object, dyn_object> map_t;

namespace Natives
{
	// native Map:map_new();
	static cell AMX_NATIVE_CALL map_new(AMX *amx, cell *params)
	{
		auto ptr = new map_t();
		return reinterpret_cast<cell>(ptr);
	}

	// native map_delete(Map:map);
	static cell AMX_NATIVE_CALL map_delete(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		delete ptr;
		return 1;
	}

	// native map_size(Map:map);
	static cell AMX_NATIVE_CALL map_size(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		return static_cast<cell>(ptr->size());
	}

	// native map_clear(Map:map);
	static cell AMX_NATIVE_CALL map_clear(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		ptr->clear();
		return 1;
	}

	// native bool:map_add(Map:map, AnyTag:key, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_add(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, params[2], params[4]), dyn_object(amx, params[3], params[5])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_add_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_add_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, params[2], params[5]), dyn_object(amx, addr, params[4], params[6])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_add_str(Map:map, AnyTag:key, const value[], key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_add_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, params[2], params[4]), dyn_object(amx, addr)));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_arr_add(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_add(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr, params[4], params[5]), dyn_object(amx, params[3], params[6])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_arr_add_arr(Map:map, const AnyTag:key[], const AnyTag:value[], key_size=sizeof(key), value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_add_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr, params[4], params[6]), dyn_object(amx, addr, params[5], params[7])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_arr_add_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_add_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr, params[4], params[5]), dyn_object(amx, addr)));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_str_add(Map:map, const key[], AnyTag:value, value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_add(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr), dyn_object(amx, params[3], params[4])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_str_add_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_add_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr), dyn_object(amx, addr, params[4], params[5])));
		return static_cast<cell>(ret.second);
	}

	// native bool:map_str_add_str(Map:map, const key[], const value[]);
	static cell AMX_NATIVE_CALL map_str_add_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return -1;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		auto ret = ptr->insert(std::make_pair(dyn_object(amx, key_addr), dyn_object(amx, addr)));
		return static_cast<cell>(ret.second);
	}

	// native map_add_map(Map:map, Map:other, bool:overwrite);
	static cell AMX_NATIVE_CALL map_add_map(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		auto ptr2 = reinterpret_cast<map_t*>(params[2]);
		if(ptr == nullptr || ptr2 == nullptr) return -1;
		if(params[3])
		{
			for(auto &pair : *ptr2)
			{
				(*ptr)[pair.first] = pair.second;
			}
		}else{
			ptr->insert(ptr2->begin(), ptr2->end());
		}
		return ptr->size() - ptr2->size();
	}

	// native bool:map_remove(Map:map, AnyTag:key, key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_remove(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[3]));
		if(it != ptr->end())
		{
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native bool:map_arr_remove(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_remove(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[3], params[4]));
		if(it != ptr->end())
		{
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native bool:map_str_remove(Map:map, const key[]);
	static cell AMX_NATIVE_CALL map_str_remove(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			ptr->erase(it);
			return 1;
		}
		return 0;
	}

	// native map_get(Map:map, AnyTag:key, offset=0, key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_get(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[4]));
		if(it != ptr->end())
		{
			cell result;
			if(it->second.get_cell(params[3], result))
			{
				return result;
			}
		}
		return 0;
	}

	// native map_get_arr(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_get_arr(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[5]));
		if(it != ptr->end())
		{
			cell *addr;
			amx_GetAddr(amx, params[3], &addr);
			return it->second.get_array(0, addr, params[4]);
		}
		return 0;
	}

	// native bool:map_get_checked(Map:map, AnyTag:key, &AnyTag:value, offset=0, key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_get_checked(AMX *amx, cell *params)
	{
		if(params[4] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[5]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[6]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				if(obj.get_cell(params[4], *addr))
				{
					return 1;
				}
			}
		}
		return 0;
	}

	// native map_get_arr_checked(Map:map, AnyTag:key, AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_get_arr_checked(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[5]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[6]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				return obj.get_array(0, addr, params[4]);
			}
		}
		return 0;
	}

	// native map_arr_get(Map:map, const AnyTag:key[], offset=0, key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_get(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[4], params[5]));
		if(it != ptr->end())
		{
			cell result;
			if(it->second.get_cell(params[3], result))
			{
				return result;
			}
		}
		return 0;
	}

	// native map_arr_get_arr(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_get_arr(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[5], params[6]));
		if(it != ptr->end())
		{
			cell *addr;
			amx_GetAddr(amx, params[3], &addr);
			return it->second.get_array(0, addr, params[4]);
		}
		return 0;
	}

	// native bool:map_arr_get_checked(Map:map, const AnyTag:key[], &AnyTag:value, offset=0, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_get_checked(AMX *amx, cell *params)
	{
		if(params[4] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[5], params[6]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[7]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				if(obj.get_cell(params[4], *addr))
				{
					return 1;
				}
			}
		}
		return 0;
	}

	// native map_arr_get_arr_checked(Map:map, const AnyTag:key[], AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_get_arr_checked(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[5], params[6]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[4]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				return obj.get_array(0, addr, params[4]);
			}
		}
		return 0;
	}

	// native map_str_get(Map:map, const key[], offset=0);
	static cell AMX_NATIVE_CALL map_str_get(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			cell result;
			if(it->second.get_cell(params[3], result))
			{
				return result;
			}
		}
		return 0;
	}

	// native map_str_get_arr(Map:map, const key[], AnyTag:value[], value_size=sizeof(value));
	static cell AMX_NATIVE_CALL map_str_get_arr(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			cell *addr;
			amx_GetAddr(amx, params[3], &addr);
			return it->second.get_array(0, addr, params[4]);
		}
		return 0;
	}

	// native bool:map_str_get_checked(Map:map, const key[], &AnyTag:value, offset=0, value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_get_checked(AMX *amx, cell *params)
	{
		if(params[4] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[5]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				if(obj.get_cell(params[4], *addr))
				{
					return 1;
				}
			}
		}
		return 0;
	}

	// native map_str_get_arr_checked(Map:map, const key[], AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_get_arr_checked(AMX *amx, cell *params)
	{
		if(params[4] <= 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(obj.check_tag(amx, params[5]))
			{
				cell *addr;
				amx_GetAddr(amx, params[3], &addr);
				return obj.get_array(0, addr, params[4]);
			}
		}
		return 0;
	}

	// native map_set(Map:map, AnyTag:key, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_set(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		(*ptr)[dyn_object(amx, params[2], params[4])] = dyn_object(amx, params[3], params[5]);
		return 1;
	}

	// native map_set_arr(Map:map, AnyTag:key, const AnyTag:value[], value_size=sizeof(value), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_set_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, params[2], params[5])] = dyn_object(amx, addr, params[4], params[6]);
		return 1;
	}

	// native map_set_str(Map:map, AnyTag:key, const value[], key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_set_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, params[2], params[5])] = dyn_object(amx, addr);
		return 1;
	}

	// native map_set_cell(Map:map, AnyTag:key, offset, AnyTag:value, key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_set_cell(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[5]));
		if(it != ptr->end())
		{
			return it->second.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native bool:map_set_cell_checked(Map:map, AnyTag:key, offset, AnyTag:value, key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_set_cell_checked(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[5]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(!obj.check_tag(amx, params[6])) return 0;
			return obj.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native map_arr_set(Map:map, const AnyTag:key[], AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_set(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		(*ptr)[dyn_object(amx, key_addr, params[4], params[5])] = dyn_object(amx, params[3], params[6]);
		return 1;
	}

	// native map_arr_set_arr(Map:map, const AnyTag:key[], const AnyTag:value[], value_size=sizeof(value), key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_set_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, key_addr, params[5], params[6])] = dyn_object(amx, addr, params[4], params[7]);
		return 1;
	}

	// native map_arr_set_str(Map:map, const AnyTag:key[], const value[], key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_set_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, key_addr, params[4], params[5])] = dyn_object(amx, addr);
		return 1;
	}

	// native map_arr_set_cell(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_set_cell(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[5], params[6]));
		if(it != ptr->end())
		{
			return it->second.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native bool:map_arr_set_cell_checked(Map:map, const AnyTag:key[], offset, AnyTag:value, key_size=sizeof(key), key_tag_id=tagof(key), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_arr_set_cell_checked(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[5], params[6]));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(!obj.check_tag(amx, params[7])) return 0;
			return obj.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native map_str_set(Map:map, const key[], AnyTag:value, value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_set(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		(*ptr)[dyn_object(amx, key_addr)] = dyn_object(amx, params[3], params[4]);
		return 1;
	}

	// native map_str_set_arr(Map:map, const key[], const AnyTag:value[], value_size=sizeof(value), value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_set_arr(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, key_addr)] = dyn_object(amx, addr, params[4], params[5]);
		return 1;
	}

	// native map_str_set_str(Map:map, const key[], const value[]);
	static cell AMX_NATIVE_CALL map_str_set_str(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		cell *addr;
		amx_GetAddr(amx, params[3], &addr);
		(*ptr)[dyn_object(amx, key_addr)] = dyn_object(amx, addr);
		return 1;
	}

	// native map_str_set_cell(Map:map, const key[], offset, AnyTag:value);
	static cell AMX_NATIVE_CALL map_str_set_cell(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			return it->second.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native bool:map_str_set_cell_checked(Map:map, const key[], offset, AnyTag:value, value_tag_id=tagof(value));
	static cell AMX_NATIVE_CALL map_str_set_cell_checked(AMX *amx, cell *params)
	{
		if(params[3] < 0) return 0;
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			auto &obj = it->second;
			if(!obj.check_tag(amx, params[5])) return 0;
			return obj.set_cell(params[3], params[4]);
		}
		return 0;
	}

	// native map_tagof(Map:map, AnyTag:key, key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_tagof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[3]));
		if(it != ptr->end())
		{
			return it->second.get_tag(amx);
		}
		return 0;
	}

	// native map_sizeof(Map:map, AnyTag:key, key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_sizeof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		auto it = ptr->find(dyn_object(amx, params[2], params[3]));
		if(it != ptr->end())
		{
			return it->second.get_size();
		}
		return 0;
	}

	// native map_arr_tagof(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_tagof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[3], params[4]));
		if(it != ptr->end())
		{
			return it->second.get_tag(amx);
		}
		return 0;
	}

	// native map_arr_sizeof(Map:map, const AnyTag:key[], key_size=sizeof(key), key_tag_id=tagof(key));
	static cell AMX_NATIVE_CALL map_arr_sizeof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr, params[3], params[4]));
		if(it != ptr->end())
		{
			return it->second.get_size();
		}
		return 0;
	}

	// native map_str_tagof(Map:map, const key[]);
	static cell AMX_NATIVE_CALL map_str_tagof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			return it->second.get_tag(amx);
		}
		return 0;
	}

	// native map_str_sizeof(Map:map, const key[]);
	static cell AMX_NATIVE_CALL map_str_sizeof(AMX *amx, cell *params)
	{
		auto ptr = reinterpret_cast<map_t*>(params[1]);
		if(ptr == nullptr) return 0;
		cell *key_addr;
		amx_GetAddr(amx, params[2], &key_addr);
		auto it = ptr->find(dyn_object(amx, key_addr));
		if(it != ptr->end())
		{
			return it->second.get_size();
		}
		return 0;
	}
}

static AMX_NATIVE_INFO native_list[] =
{
	AMX_DECLARE_NATIVE(map_new),
	AMX_DECLARE_NATIVE(map_delete),
	AMX_DECLARE_NATIVE(map_size),
	AMX_DECLARE_NATIVE(map_clear),
	AMX_DECLARE_NATIVE(map_add),
	AMX_DECLARE_NATIVE(map_add_arr),
	AMX_DECLARE_NATIVE(map_add_str),
	AMX_DECLARE_NATIVE(map_arr_add),
	AMX_DECLARE_NATIVE(map_arr_add_arr),
	AMX_DECLARE_NATIVE(map_arr_add_str),
	AMX_DECLARE_NATIVE(map_str_add),
	AMX_DECLARE_NATIVE(map_str_add_arr),
	AMX_DECLARE_NATIVE(map_str_add_str),
	AMX_DECLARE_NATIVE(map_add_map),
	AMX_DECLARE_NATIVE(map_remove),
	AMX_DECLARE_NATIVE(map_arr_remove),
	AMX_DECLARE_NATIVE(map_str_remove),
	AMX_DECLARE_NATIVE(map_get),
	AMX_DECLARE_NATIVE(map_get_arr),
	AMX_DECLARE_NATIVE(map_get_checked),
	AMX_DECLARE_NATIVE(map_get_arr_checked),
	AMX_DECLARE_NATIVE(map_arr_get),
	AMX_DECLARE_NATIVE(map_arr_get_arr),
	AMX_DECLARE_NATIVE(map_arr_get_checked),
	AMX_DECLARE_NATIVE(map_arr_get_arr_checked),
	AMX_DECLARE_NATIVE(map_str_get),
	AMX_DECLARE_NATIVE(map_str_get_arr),
	AMX_DECLARE_NATIVE(map_str_get_checked),
	AMX_DECLARE_NATIVE(map_str_get_arr_checked),
	AMX_DECLARE_NATIVE(map_set),
	AMX_DECLARE_NATIVE(map_set_arr),
	AMX_DECLARE_NATIVE(map_set_str),
	AMX_DECLARE_NATIVE(map_set_cell),
	AMX_DECLARE_NATIVE(map_set_cell_checked),
	AMX_DECLARE_NATIVE(map_arr_set),
	AMX_DECLARE_NATIVE(map_arr_set_arr),
	AMX_DECLARE_NATIVE(map_arr_set_str),
	AMX_DECLARE_NATIVE(map_arr_set_cell),
	AMX_DECLARE_NATIVE(map_arr_set_cell_checked),
	AMX_DECLARE_NATIVE(map_str_set),
	AMX_DECLARE_NATIVE(map_str_set_arr),
	AMX_DECLARE_NATIVE(map_str_set_str),
	AMX_DECLARE_NATIVE(map_str_set_cell),
	AMX_DECLARE_NATIVE(map_str_set_cell_checked),
	AMX_DECLARE_NATIVE(map_tagof),
	AMX_DECLARE_NATIVE(map_sizeof),
	AMX_DECLARE_NATIVE(map_arr_tagof),
	AMX_DECLARE_NATIVE(map_arr_sizeof),
	AMX_DECLARE_NATIVE(map_str_tagof),
	AMX_DECLARE_NATIVE(map_str_sizeof),
};

int RegisterMapNatives(AMX *amx)
{
	return amx_Register(amx, native_list, sizeof(native_list) / sizeof(*native_list));
}
