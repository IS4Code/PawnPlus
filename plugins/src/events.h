#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "sdk/amx/amx.h"
#include <string>
#include <vector>

namespace Events
{
	ptrdiff_t Register(const char *callback, AMX *amx, const char *function, const char *format, const cell *params, int numargs);
	bool Remove(AMX *amx, ptrdiff_t index);
	int GetCallbackId(AMX *amx, const char *callback);
	const char *Invoke(int id, AMX *amx, cell *retval);

	enum class ParamType
	{
		Cell, EventId, String
	};

	struct EventParam
	{
		ParamType Type;
		union {
			cell CellValue;
			std::basic_string<cell> StringValue;
		};

		EventParam();
		EventParam(cell val);
		EventParam(std::basic_string<cell> &&str);
		EventParam(const EventParam &obj);
		~EventParam();
	};

	class EventInfo
	{
		bool active;
		std::vector<EventParam> argvalues;
		std::string handler;
	public:
		EventInfo(AMX *amx, const char *function, const char *format, const cell *args, size_t numargs);
		void Deactivate();
		bool IsActive();
		bool Invoke(AMX *amx, cell *retval, ptrdiff_t id);
	};


}

#endif
