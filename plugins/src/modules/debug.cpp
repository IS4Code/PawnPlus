#include "debug.h"

#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
//#include <aclapi.h>
#else
#include <stdio.h>
#endif
#include <fcntl.h>

#include <thread>

static std::thread::id main_thread;

static subhook_t CreateFileA_Hook;
static decltype(&CreateFileA) CreateFileA_Trampoline;
static HANDLE LastFile = nullptr;

static HANDLE WINAPI HookCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	//if(std::this_thread::get_id() == main_thread)
	{
		if(LastFile)
		{
			CloseHandle(LastFile);
			LastFile = nullptr;
		}
		HANDLE hfile = CreateFileA_Trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if(hfile && (dwDesiredAccess & GENERIC_READ))
		{
			DuplicateHandle(GetCurrentProcess(), hfile, GetCurrentProcess(), &LastFile, 0, TRUE, DUPLICATE_SAME_ACCESS);
		}
		return hfile;
	}
	return CreateFileA_Trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

AMX_DBG *debug::create_last()
{
	if(!LastFile)
	{
		return nullptr;
	}
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(LastFile), _O_RDONLY);
	if(fd == -1)
	{
		CloseHandle(LastFile);
		LastFile = nullptr;
		return nullptr;
	}
	auto f = _fdopen(fd, "rb");
	if(!f)
	{
		_close(fd);
		LastFile = nullptr;
		return nullptr;
	}
	LastFile = nullptr;

	auto dbg = new AMX_DBG();
	if(dbg_LoadInfo(dbg, f) != AMX_ERR_NONE)
	{
		delete dbg;
		fclose(f);
		return nullptr;
	}
	fclose(f);

	return dbg;
}

void debug::init()
{
	main_thread = std::this_thread::get_id();
	CreateFileA_Hook = subhook_new(reinterpret_cast<void*>(CreateFileA), reinterpret_cast<void*>(HookCreateFileA), {});
	CreateFileA_Trampoline = reinterpret_cast<decltype(&CreateFileA)>(subhook_get_trampoline(CreateFileA_Hook));
	subhook_install(CreateFileA_Hook);
}
