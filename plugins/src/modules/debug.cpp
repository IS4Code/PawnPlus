#include "debug.h"

#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#endif
#include <fcntl.h>

#ifdef _WIN32
static subhook_t CreateFileA_Hook;
static decltype(&CreateFileA) CreateFileA_Trampoline;

static thread_local HANDLE _last_file = nullptr;

static HANDLE WINAPI HookCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	auto &last_file = _last_file;
	if(last_file)
	{
		CloseHandle(last_file);
		last_file = nullptr;
	}
	HANDLE hfile = CreateFileA_Trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if(hfile && (dwDesiredAccess & GENERIC_READ))
	{
		DuplicateHandle(GetCurrentProcess(), hfile, GetCurrentProcess(), &last_file, 0, TRUE, DUPLICATE_SAME_ACCESS);
	}
	return hfile;
}

AMX_DBG *debug::create_last()
{
	auto &last_file = _last_file;
	if(!last_file)
	{
		return nullptr;
	}
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(last_file), _O_RDONLY);
	if(fd == -1)
	{
		CloseHandle(last_file);
		last_file = nullptr;
		return nullptr;
	}
	auto f = _fdopen(fd, "rb");
	if(!f)
	{
		_close(fd);
		last_file = nullptr;
		return nullptr;
	}
	last_file = nullptr;

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
	auto func = reinterpret_cast<void*>(CreateFileA);
	auto dst = subhook_read_dst(func);
	if(dst != nullptr)
	{
		func = dst; // crashdetect may remove its hook, so hook crashdetect instead
	}
	CreateFileA_Hook = subhook_new(dst, reinterpret_cast<void*>(HookCreateFileA), {});
	CreateFileA_Trampoline = reinterpret_cast<decltype(&CreateFileA)>(subhook_get_trampoline(CreateFileA_Hook));
	subhook_install(CreateFileA_Hook);
}
#else
static subhook_t fopen_hook;
static decltype(&fopen) fopen_trampoline;

static thread_local int _last_file = -1;

static FILE *hook_fopen(const char *pathname, const char *mode)
{
	auto &last_file = _last_file;
	if(last_file != -1)
	{
		close(last_file);
		last_file = -1;
	}
	auto file = fopen_trampoline(pathname, mode);
	if(file && !std::strcmp(mode, "rb"))
	{
		int fd = fileno(file);
		if(fd != -1)
		{
			last_file = dup(fd);
		}
	}
	return file;
}

AMX_DBG *debug::create_last()
{
	auto &last_file = _last_file;
	if(last_file == -1)
	{
		return nullptr;
	}
	auto f = fdopen(last_file, "rb");
	if(!f)
	{
		close(last_file);
		last_file = -1;
		return nullptr;
	}
	last_file = -1;

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
	auto func = reinterpret_cast<void*>(fopen);
	auto dst = subhook_read_dst(func);
	if(dst != nullptr)
	{
		func = dst; // crashdetect may remove its hook, so hook crashdetect instead
	}
	fopen_hook = subhook_new(func, reinterpret_cast<void*>(hook_fopen), {});
	fopen_trampoline = reinterpret_cast<decltype(&fopen)>(subhook_get_trampoline(fopen_hook));
	subhook_install(fopen_hook);
}
#endif
