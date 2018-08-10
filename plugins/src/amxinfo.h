#ifndef AMXINFO_H_INCLUDED
#define AMXINFO_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <cstring>
#include <memory>

namespace amx
{
	class extra
	{
	protected:
		AMX *_amx;

	public:
		extra(AMX *amx) : _amx(amx)
		{
			
		}

		virtual std::unique_ptr<extra> clone()
		{
			return nullptr;
		}

		virtual ~extra() = default;
	};

	typedef std::weak_ptr<class instance> handle;
	typedef std::shared_ptr<class instance> object;

	class instance
	{
		friend object load_lock(AMX *amx);
		friend object clone_lock(AMX *amx, AMX *new_amx);
		friend bool invalidate(AMX *amx);

		AMX *_amx;
		std::unordered_map<std::type_index, std::unique_ptr<extra>> extras;

		explicit instance(AMX *amx) : _amx(amx)
		{

		}

		void invalidate()
		{
			_amx = nullptr;
		}

		instance(const instance &obj, AMX *new_amx) : _amx(new_amx)
		{
			for(const auto &pair : obj.extras)
			{
				if(pair.second)
				{
					auto clone = pair.second->clone();
					if(clone)
					{
						extras[pair.first] = std::move(clone);
					}
				}
			}
		}

	public:
		instance() : _amx(nullptr)
		{

		}

		instance(const instance &obj) = delete;
		instance(instance &&obj) : _amx(obj._amx), extras(std::move(obj.extras))
		{
			obj._amx = nullptr;
			obj.extras.clear();
		}

		bool valid() const
		{
			return _amx != nullptr;
		}

		instance &operator=(const instance &obj) = delete;
		instance &operator=(instance &&obj)
		{
			if(this != &obj)
			{
				_amx = obj._amx;
				extras = std::move(obj.extras);
				obj._amx = nullptr;
				obj.extras.clear();
			}
			return *this;
		}

		template <class ExtraType>
		ExtraType &get_extra()
		{
			std::type_index key = typeid(ExtraType);

			auto it = extras.find(key);
			if(it == extras.end())
			{
				it = extras.insert(std::make_pair(key, std::unique_ptr<ExtraType>(new ExtraType(_amx)))).first;
			}
			return static_cast<ExtraType&>(*it->second);
		}

		template <class ExtraType>
		bool has_extra() const
		{
			std::type_index key = typeid(ExtraType);

			return extras.find(key) != extras.end();
		}

		template <class ExtraType>
		bool remove_extra()
		{
			std::type_index key = typeid(ExtraType);

			auto it = extras.find(key);
			if(it != extras.end())
			{
				extras.erase(it);
				return true;
			}
			return false;
		}

		AMX *get()
		{
			return _amx;
		}

		operator AMX*()
		{
			return _amx;
		}
	};

	handle load(AMX *amx);
	object load_lock(AMX *amx);
	handle clone(AMX *amx, AMX *new_amx);
	object clone_lock(AMX *amx, AMX *new_amx);
	bool unload(AMX *amx);
	bool invalidate(AMX *amx);
	void register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
	AMX_NATIVE find_native(AMX *amx, const char *name);
	AMX_NATIVE find_native(AMX *amx, const std::string &name);
	size_t num_natives(AMX *amx);

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, cell arg, Args&&... args)
	{
		arglist[0]++;
		arglist.push_back(arg);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, float arg, Args&&... args)
	{
		arglist[0]++;
		arglist.push_back(amx_ftoc(arg));
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, cell &arg, Args&&... args)
	{
		cell amx_addr, *addr;
		amx_Allot(amx, 1, &amx_addr, &addr);
		*addr = arg;
		arglist[0]++;
		arglist.push_back(amx_addr);
		cell result = call_args(amx, native, arglist, std::forward<Args>(args)...);
		arg = *addr;
		return result;
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, float &arg, Args&&... args)
	{
		cell amx_addr, *addr;
		amx_Allot(amx, 1, &amx_addr, &addr);
		*addr = amx_ftoc(arg);
		arglist[0]++;
		arglist.push_back(amx_addr);
		cell result = call_args(amx, native, arglist, std::forward<Args>(args)...);
		arg = amx_ctof(*addr);
		return result;
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const std::string &arg, Args&&... args)
	{
		cell amx_addr, *addr;
		size_t size = arg.size() + 1;
		amx_Allot(amx, size, &amx_addr, &addr);
		amx_SetString(addr, arg.c_str(), 0, 0, size);
		arglist[0]++;
		arglist.push_back(amx_addr);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const char *arg, Args&&... args)
	{
		cell amx_addr, *addr;
		size_t size = std::strlen(arg) + 1;
		amx_Allot(amx, size, &amx_addr, &addr);
		amx_SetString(addr, arg, 0, 0, size);
		arglist[0]++;
		arglist.push_back(amx_addr);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	template <size_t Size, class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const cell (&arg)[Size], Args&&... args)
	{
		cell amx_addr, *addr;
		amx_Allot(amx, Size, &amx_addr, &addr);
		std::memcpy(addr, arg, Size * sizeof(cell));
		arglist[0]++;
		arglist.push_back(amx_addr);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	inline cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist)
	{
		return native(amx, &arglist[0]);
	}

	template <class... Args>
	cell call_native(AMX *amx, AMX_NATIVE native, Args&&... args)
	{
		cell reset_hea, *tmp;
		amx_Allot(amx, 0, &reset_hea, &tmp);
		std::vector<cell> arglist = {0};
		cell result = call_args(amx, native, arglist, std::forward<Args>(args)...);
		amx_Release(amx, reset_hea);
		return result;
	}
}

#endif
