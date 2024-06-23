#include "amxinfo.h"
#include "natives.h"
#include "modules/tags.h"
#include "modules/amxutils.h"
#include <unordered_map>

#include "subhook/subhook.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#else
#include <signal.h>
#include <setjmp.h>
#endif

struct natives_extra : public amx::extra
{
	natives_extra(AMX *amx) : extra(amx)
	{

	}

	natives_extra(AMX *amx, const std::unordered_map<std::string, AMX_NATIVE> &natives) : extra(amx), natives(natives)
	{

	}

	std::unordered_map<std::string, AMX_NATIVE> natives;

	virtual std::unique_ptr<extra> clone() override
	{
		return std::unique_ptr<extra>(new natives_extra(_amx, natives));
	}
};

static std::unordered_map<AMX*, std::shared_ptr<amx::instance>> amx_map;

bool amx::valid(AMX *amx)
{
	return amx_map.find(amx) != amx_map.end();
}

void amx::call_all(void(*func)(void *cookie, AMX *amx), void *cookie)
{
	for(const auto &pair : amx_map)
	{
		if(pair.second->valid())
		{
			func(cookie, pair.first);
		}
	}
}

// Nothing should be loaded from the AMX here, since it may not even be initialized yet
const amx::object &amx::load_lock(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		return it->second;
	}
	return amx_map.emplace(amx, std::make_shared<instance>(amx)).first->second;
}

const amx::object &amx::clone_lock(AMX *amx, AMX *new_amx)
{
	const auto &obj = load_lock(amx);
	return amx_map.emplace(new_amx, std::make_shared<instance>(*obj, new_amx)).first->second;
}


bool amx::unload(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		amx_map.erase(it);
		return true;
	}
	return false;
}

bool amx::invalidate(AMX *amx)
{
	auto it = amx_map.find(amx);
	if(it != amx_map.end())
	{
		it->second->invalidate();
		return true;
	}
	return false;
}

void amx::register_natives(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number)
{
	const auto &obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;
	for(int i = 0; (i < number || number == -1) && nativelist[i].name != nullptr; i++)
	{
		natives.insert(std::make_pair(nativelist[i].name, nativelist[i].func));
	}
	obj->run_initializers();
}

AMX_NATIVE amx::try_decode_native(const char *str)
{
	if(str[0] == 0x1C)
	{
		auto name = reinterpret_cast<const unsigned char *>(str + 1);
		ucell value;
		if(amx_decode_value(name, value))
		{
			return reinterpret_cast<AMX_NATIVE>(value);
		}
	}
	return nullptr;
}

AMX_NATIVE amx::find_native(AMX *amx, const char *name)
{
	auto decoded = try_decode_native(name);
	if(decoded != nullptr)
	{
		return decoded;
	}
	return amx::find_native(amx, std::string(name));
}

AMX_NATIVE amx::find_native(AMX *amx, const std::string &name)
{
	auto decoded = try_decode_native(name.c_str());
	if(decoded != nullptr)
	{
		return decoded;
	}
	const auto &obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;

	auto it = natives.find(name);
	if(it != natives.end())
	{
		return it->second;
	}
	int index;
	if(!amx_FindNative(amx, name.c_str(), &index))
	{
		auto amxhdr = (AMX_HEADER*)amx->base;
		auto func = (AMX_FUNCSTUB*)((unsigned char*)amxhdr+ amxhdr->natives + index* amxhdr->defsize);
		auto f = reinterpret_cast<AMX_NATIVE>(func->address);
		natives.insert(std::make_pair(name, f));
		return f;
	}
	return nullptr;
}

size_t amx::num_natives(AMX *amx)
{
	const auto &obj = load_lock(amx);
	auto &natives = obj->get_extra<natives_extra>().natives;
	return natives.size();
}

namespace
{
	constexpr const char *corrupted_error = " (possibly corrupted memory)";

#ifdef _WIN32
	struct local_deleter
	{
		void operator()(LPSTR str)
		{
			LocalFree(str);
		}
	};

	std::unique_ptr<CHAR[], local_deleter> status_message(LONG status)
	{
		HMODULE ntdll = LoadLibrary("NTDLL.DLL");
		LPSTR buffer = nullptr;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, ntdll, status, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
		FreeLibrary(ntdll);
		return {buffer, local_deleter()};
	}

	bool is_fatal(DWORD code)
	{
		switch(code)
		{
			case EXCEPTION_ACCESS_VIOLATION:
			case EXCEPTION_ILLEGAL_INSTRUCTION:
			case EXCEPTION_STACK_OVERFLOW:
				return true;
			default:
				return false;
		}
	}

	[[noreturn]] void status_error(LONG status)
	{
		auto message = status_message(status);
		amx_LogicError(errors::unhandled_system_exception, status, message.get(), is_fatal(status) ? corrupted_error : "");
	}

	bool filter_exception(DWORD code)
	{
		switch(code)
		{
			case EXCEPTION_ACCESS_VIOLATION:
			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			case EXCEPTION_DATATYPE_MISALIGNMENT:
			case EXCEPTION_FLT_DENORMAL_OPERAND:
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			case EXCEPTION_FLT_INEXACT_RESULT:
			case EXCEPTION_FLT_INVALID_OPERATION:
			case EXCEPTION_FLT_OVERFLOW:
			case EXCEPTION_FLT_STACK_CHECK:
			case EXCEPTION_FLT_UNDERFLOW:
			case EXCEPTION_ILLEGAL_INSTRUCTION:
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
			case EXCEPTION_INT_OVERFLOW:
			case EXCEPTION_INVALID_HANDLE:
			case EXCEPTION_STACK_OVERFLOW:
				return true;
			default:
				return false;
		}
	}
#else
	sigjmp_buf jmp;

	void signal_handler(int signal)
	{
		siglongjmp(jmp, signal);
	}

	static int checked_signals[] = {SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGBUS, SIGPIPE};
	constexpr const int num_checked_signals = sizeof(checked_signals) / sizeof(int);

	class signals_guard
	{
		struct sigaction oldact[num_checked_signals], act;

	public:
		signals_guard()
		{
			memset(&act, 0, sizeof(act));
			act.sa_handler = signal_handler;
			act.sa_flags = SA_RESETHAND;
			for(int i = 0; i < num_checked_signals; i++)
			{
				sigaction(checked_signals[i], &act, &oldact[i]);
			}
		}

		~signals_guard()
		{
			for(int i = 0; i < num_checked_signals; i++)
			{
				sigaction(checked_signals[i], &oldact[i], nullptr);
			}
		}
	};

	bool is_fatal(int signal)
	{
		switch(signal)
		{
			case SIGILL:
			case SIGSEGV:
			case SIGBUS:
				return true;
			default:
				return false;
		}
	}

#endif

	cell call_external_native(AMX *amx, AMX_NATIVE native, cell *params)
	{
#ifdef _DEBUG
		return native(amx, params);
#elif defined _WIN32
		DWORD error;
		__try{
			return native(amx, params);
		}__except(filter_exception(error = GetExceptionCode()) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			if(error == EXCEPTION_STACK_OVERFLOW && !_resetstkoflw())
			{
				RaiseException(EXCEPTION_STACK_OVERFLOW, EXCEPTION_NONCONTINUABLE, 0, nullptr);
			}
			status_error(error);
		}
#else
		if(!is_main_thread)
		{
			return native(amx, params);
		}
		int error = sigsetjmp(jmp, true);
		if(error)
		{
			amx_LogicError(errors::unhandled_system_exception, error, sigdescr_np(error), is_fatal(error) ? corrupted_error : "");
		}
		signals_guard guard;
		return native(amx, params);
#endif
	}
}

cell amx::dynamic_call(AMX *amx, AMX_NATIVE native, cell *params, tag_ptr &out_tag)
{
	cell result;
	amx->error = AMX_ERR_NONE;
	native_return_tag = nullptr;
	auto it = impl::runtime_native_map().find(native);
	if(it != impl::runtime_native_map().end() && subhook_read_dst(reinterpret_cast<void*>(native)) == nullptr)
	{
		const auto &info = it->second;
		try{
			if(params[0] < info.arg_count * static_cast<cell>(sizeof(cell)))
			{
				amx_FormalError(errors::not_enough_args, info.arg_count, params[0] / static_cast<cell>(sizeof(cell)));
			}
			result = call_external_native(amx, info.inner, params);
			if(info.tag_uid != tags::tag_unknown)
			{
				native_return_tag = tags::find_tag(info.tag_uid);
			}
		}catch(const errors::end_of_arguments_error &err)
		{
			amx_FormalError(errors::not_enough_args, err.argbase - params - 1 + err.required, params[0] / static_cast<cell>(sizeof(cell)));
		}
#ifndef _DEBUG
		catch(const std::exception &err)
		{
			amx_LogicError(errors::unhandled_exception, err.what());
		}
#endif
	}else{
		result = call_external_native(amx, native, params);
	}
	out_tag = native_return_tag;
	if(amx->error != AMX_ERR_NONE)
	{
		int code = amx->error;
		amx->error = AMX_ERR_NONE;
		throw errors::amx_error(code);
	}
	return result;
}

void amx::instance::run_initializers()
{
	if(_amx && loaded && !initialized && (_amx->flags & AMX_FLAG_NTVREG))
	{
		int num;
		if(amx_NumPublics(_amx, &num) == AMX_ERR_NONE)
		{
			char *funcname = amx_NameBuffer(_amx);
			cell ret;
			for(int i = 0; i < num; i++)
			{
				if(amx_GetPublic(_amx, i, funcname) == AMX_ERR_NONE)
				{
					if(!std::memcmp(funcname, "_pp@i", 5) || !std::memcmp(funcname, "_pp@on_init@", 12))
					{
						amx_Exec(_amx, &ret, i);
					}
				}
			}
			initialized = true;
		}
	}
}

void amx::instance::run_finalizers()
{
	if(_amx && initialized && (_amx->flags & AMX_FLAG_NTVREG))
	{
		int num;
		if(amx_NumPublics(_amx, &num) == AMX_ERR_NONE)
		{
			char *funcname = amx_NameBuffer(_amx);
			cell ret;
			for(int i = num - 1; i >= 0; i--)
			{
				if(amx_GetPublic(_amx, i, funcname) == AMX_ERR_NONE)
				{
					if(!std::memcmp(funcname, "_pp@e", 5) || !std::memcmp(funcname, "_pp@on_exit@", 12))
					{
						amx_Exec(_amx, &ret, i);
					}
				}
			}
		}
	}
}
