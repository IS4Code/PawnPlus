#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "amxinfo.h"
#include "sdk/amx/amx.h"
#include <functional>
#include <stack>
#include <unordered_map>

constexpr cell SleepReturnTypeMask = 0xFF000000;
constexpr cell SleepReturnValueMask = 0x00FFFFFF;

constexpr cell SleepReturnAwait = 0xA0000000;
constexpr cell SleepReturnWaitMs = 0xA1000000;
constexpr cell SleepReturnWaitTicks = 0xA2000000;
constexpr cell SleepReturnWaitInf = 0xA3000000;

constexpr cell SleepReturnDetach = 0xB0000000;
constexpr cell SleepReturnAttach = 0xB1000000;
constexpr cell SleepReturnSync = 0xB2000000;

constexpr cell SleepReturnFork = 0xC0000000;
constexpr cell SleepReturnForkFlagsClone = 0x00000001;
constexpr cell SleepReturnForkFlagsCopyData = 0x00000002;
constexpr cell SleepReturnForkCommit = 0xC1000000;

namespace amx
{
	class context
	{
		AMX *_amx;
		int _index;
		std::unordered_map<std::type_index, std::unique_ptr<extra>> extras;

	public:
		context()
		{

		}

		context(AMX *amx, int index) : _amx(amx), _index(index)
		{

		}

		context(const context &obj) = delete;
		context(context &&obj) : _amx(obj._amx), _index(obj._index), extras(std::move(obj.extras))
		{
			obj._amx = nullptr;
			obj.extras.clear();
		}

		context &operator=(const context &obj) = delete;
		context &operator=(context &&obj)
		{
			if(this != &obj)
			{
				_amx = obj._amx;
				_index = obj._index;
				extras = std::move(obj.extras);
				obj._amx = nullptr;
				obj.extras.clear();
			}
			return *this;
		}

		int get_index() const
		{
			return _index;
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
	};

	int push(AMX *amx, int index);
	int pop(AMX *amx);

	void restore(AMX *amx, context &&context);
	bool has_context(AMX *amx);
	context &get_context(AMX *amx, object &obj);

	void on_bottom(const std::function<void(AMX*)> &callback);

	template <class Func>
	void on_bottom(const Func &callback)
	{
		return on_bottom(std::function<void(AMX*)>(callback));
	}
}

#endif
