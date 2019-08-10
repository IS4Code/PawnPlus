#ifndef AMXINFO_H_INCLUDED
#define AMXINFO_H_INCLUDED

#include "errors.h"
#include "modules/tags.h"
#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"
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

		extra(const extra&) = delete;

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
		friend const object &load_lock(AMX *amx);
		friend const object &clone_lock(AMX *amx, AMX *new_amx);
		friend bool invalidate(AMX *amx);

		AMX *_amx;
		std::unordered_map<std::type_index, std::unique_ptr<extra>> extras;
		bool initialized = false;

		void invalidate()
		{
			_amx = nullptr;
		}

	public:
		std::string name;
		std::shared_ptr<AMX_DBG> dbg;

		instance() : _amx(nullptr)
		{

		}

		explicit instance(AMX *amx) : _amx(amx)
		{

		}

		instance(const instance &obj, AMX *new_amx) : _amx(new_amx), name(obj.name), dbg(obj.dbg)
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

		instance(const instance &obj) = delete;
		instance(instance &&obj) : _amx(obj._amx), extras(std::move(obj.extras)), name(std::move(obj.name)), dbg(std::move(obj.dbg))
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
				name = std::move(obj.name);
				dbg = std::move(obj.dbg);
				obj._amx = nullptr;
				obj.extras.clear();
				obj.dbg = {};
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

		bool loaded = false;
		void run_initializers();
		void run_finalizers();
	};

	bool valid(AMX *amx);

	void call_all(void(*func)(void *cookie, AMX *amx), void *cookie);

	template <class Func>
	void call_all(Func func)
	{
		call_all([](void *cookie, AMX *amx)
		{
			(*reinterpret_cast<Func*>(cookie))(amx);
		}, &func);
	}

	const object &load_lock(AMX *amx);

	inline handle load(AMX *amx)
	{
		return load_lock(amx);
	}

	const object &clone_lock(AMX *amx, AMX *new_amx);

	inline handle clone(AMX *amx, AMX *new_amx)
	{
		return clone_lock(amx, new_amx);
	}

	bool unload(AMX *amx);
	bool invalidate(AMX *amx);
	void register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
	AMX_NATIVE try_decode_native(const char *str);
	AMX_NATIVE find_native(AMX *amx, const char *name);
	AMX_NATIVE find_native(AMX *amx, const std::string &name);
	size_t num_natives(AMX *amx);

	cell dynamic_call(AMX *amx, AMX_NATIVE native, cell *params, tag_ptr &out_tag);

	inline cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist)
	{
		return native(amx, &arglist[0]);
	}

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, cell arg, Args&&... args);

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, float arg, Args&&... args);

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, cell &arg, Args&&... args);

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, float &arg, Args&&... args);
	
	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const std::string &arg, Args&&... args);

	template <class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const char *arg, Args&&... args);

	template <size_t Size, class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const cell(&arg)[Size], Args&&... args);

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
		amx_AllotSafe(amx, 1, &amx_addr, &addr);
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
		amx_AllotSafe(amx, 1, &amx_addr, &addr);
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
		amx_AllotSafe(amx, size, &amx_addr, &addr);
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
		amx_AllotSafe(amx, size, &amx_addr, &addr);
		amx_SetString(addr, arg, 0, 0, size);
		arglist[0]++;
		arglist.push_back(amx_addr);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	template <size_t Size, class... Args>
	cell call_args(AMX *amx, AMX_NATIVE native, std::vector<cell> &arglist, const cell (&arg)[Size], Args&&... args)
	{
		cell amx_addr, *addr;
		amx_AllotSafe(amx, Size, &amx_addr, &addr);
		std::memcpy(addr, arg, Size * sizeof(cell));
		arglist[0]++;
		arglist.push_back(amx_addr);
		return call_args(amx, native, arglist, std::forward<Args>(args)...);
	}

	struct guard
	{
		AMX *amx;
		cell hea;
		cell stk;
		int paramcount;

		guard(AMX *amx) : amx(amx)
		{
			hea = amx->hea;
			stk = amx->stk;
			paramcount = amx->paramcount;
		}

		~guard()
		{
			amx->hea = hea;
			amx->stk = stk;
			amx->paramcount = paramcount;
		}
	};

	template <class... Args>
	cell call_native(AMX *amx, AMX_NATIVE native, Args&&... args)
	{
		amx::guard guard(amx);
		std::vector<cell> arglist = {0};
		cell result = call_args(amx, native, arglist, std::forward<Args>(args)...);
		return result;
	}
}

#endif
