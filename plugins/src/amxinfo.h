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

		virtual ~extra() = default;
	};

	typedef std::weak_ptr<class instance> handle;
	typedef std::shared_ptr<class instance> object;

	class instance
	{
		friend object load_lock(AMX *amx);

		AMX *_amx;
		std::unordered_map<std::type_index, std::unique_ptr<extra>> extras;

		explicit instance(AMX *amx) : _amx(amx)
		{

		}

	public:
		instance() : _amx(nullptr)
		{

		}

		instance(const instance &obj) = delete;

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

		instance *operator=(const instance &obj) = delete;

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
	bool unload(AMX *amx);
	void register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
	AMX_NATIVE find_native(AMX *amx, const char *name);
	AMX_NATIVE find_native(AMX *amx, const std::string &name);

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
