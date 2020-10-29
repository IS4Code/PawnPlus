#include "debug.h"

#include "sdk/amx/amx.h"
#include "sdk/amx/amxdbg.h"
#include "sdk/plugincommon.h"
#include "subhook/subhook.h"
#include "subhook/subhook_private.h"

#include "fixes/linux.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <stdio.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <cstring>
#include <memory>

static thread_local std::unique_ptr<char[]> _last_name(nullptr);

static void set_script_name(const char *name)
{
	size_t len = std::strlen(name);
	if(len > 0)
	{
		const char *last = name + len;
		const char *dot = nullptr;
		while(last > name)
		{
			last -= 1;
			if(*last == '.' && !dot)
			{
				dot = last;
			}else if(*last == '/' || *last == '\\')
			{
				name = last + 1;
				break;
			}
		}
		if(dot)
		{
			if(!std::strcmp(dot + 1, "amx"))
			{
				len = dot - name;
				_last_name = std::make_unique<char[]>(len + 1);
				std::memcpy(_last_name.get(), name, dot - name);
				_last_name[len] = '\0';
			}
		}
	}
}

struct amx_dbg_info
{
	AMX_DBG dbg;
	int err;

	amx_dbg_info(FILE *fp)
	{
		err = dbg_LoadInfo(&dbg, fp);
	}

	~amx_dbg_info()
	{
		if(err == AMX_ERR_NONE)
		{
			dbg_FreeInfo(&dbg);
		}
	}
};

#ifdef _WIN32
static subhook_t CreateFileA_Hook;
static decltype(&CreateFileA) CreateFileA_Trampoline;

struct handle_delete
{
	void operator()(HANDLE ptr) const
	{
		if(ptr)
		{
			CloseHandle(ptr);
		}
	}
};

static std::unique_ptr<typename std::remove_pointer<HANDLE>::type, handle_delete> last_file = nullptr;

static HANDLE WINAPI HookCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if(!is_main_thread)
	{
		return CreateFileA_Trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	last_file = nullptr;
	HANDLE hfile = CreateFileA_Trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if(hfile && (dwDesiredAccess & GENERIC_READ))
	{
		HANDLE handle;
		DuplicateHandle(GetCurrentProcess(), hfile, GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS);
		last_file = std::unique_ptr<void, handle_delete>(handle);
		set_script_name(lpFileName);
	}
	return hfile;
}

std::shared_ptr<AMX_DBG> debug::create_last(std::unique_ptr<char[]> &name)
{
	if(!last_file)
	{
		return nullptr;
	}
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(last_file.get()), _O_RDONLY);
	if(fd == -1)
	{
		last_file = nullptr;
		return nullptr;
	}
	last_file.release();
	auto f = _fdopen(fd, "rb");
	if(!f)
	{
		_close(fd);
		return nullptr;
	}

	name = std::move(_last_name);

	auto dbg = std::make_shared<amx_dbg_info>(f);
	fclose(f);
	if(dbg->err != AMX_ERR_NONE)
	{
		return nullptr;
	}
	return std::shared_ptr<AMX_DBG>(dbg, &dbg->dbg);
}

void debug::init()
{
	auto func = reinterpret_cast<void*>(CreateFileA);
	auto dst = subhook_read_dst(func);
	if(dst != nullptr)
	{
		func = dst; // crashdetect may remove its hook, so hook crashdetect instead
	}
	CreateFileA_Hook = subhook_new(func, reinterpret_cast<void*>(HookCreateFileA), {});
	CreateFileA_Trampoline = reinterpret_cast<decltype(&CreateFileA)>(subhook_get_trampoline(CreateFileA_Hook));
	if(!CreateFileA_Trampoline)
	{
		CreateFileA_Trampoline = [](LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
		{
			auto dst = subhook_read_dst(reinterpret_cast<void*>(CreateFileA));
			auto olddst = subhook_get_dst(CreateFileA_Hook);
			if(dst != olddst)
			{
				CreateFileA_Hook->dst = dst;
				subhook_remove(CreateFileA_Hook);
				auto ret = CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
				subhook_install(CreateFileA_Hook);
				CreateFileA_Hook->dst = olddst;
				return ret;
			}else if(!dst)
			{
				return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
			}else{
				subhook_remove(CreateFileA_Hook);
				auto ret = CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
				subhook_install(CreateFileA_Hook);
				return ret;
			}
		};
	}
	subhook_install(CreateFileA_Hook);
}
#else
static subhook_t fopen_hook;
static decltype(&fopen) fopen_trampoline;

struct descriptor_delete
{
	void operator()(void *ptr) const
	{
		if(ptr)
		{
			close(reinterpret_cast<int>(ptr) - 1);
		}
	}
};

static std::unique_ptr<void, descriptor_delete> last_file;

static FILE *hook_fopen(const char *pathname, const char *mode)
{
	if(!is_main_thread)
	{
		return fopen_trampoline(pathname, mode);
	}
	last_file = nullptr;
	auto file = fopen_trampoline(pathname, mode);
	if(file && !std::strcmp(mode, "rb"))
	{
		int fd = fileno(file);
		if(fd != -1)
		{
			last_file = std::unique_ptr<void, descriptor_delete>(reinterpret_cast<void*>(dup(fd) + 1));
			set_script_name(pathname);
		}
	}
	return file;
}

std::shared_ptr<AMX_DBG> debug::create_last(std::unique_ptr<char[]> &name)
{
	if(!last_file)
	{
		return nullptr;
	}
	auto f = fdopen(reinterpret_cast<int>(last_file.get()) - 1, "rb");
	if(!f)
	{
		last_file = nullptr;
		return nullptr;
	}
	last_file.release();

	name = std::move(_last_name);
	
	auto dbg = std::make_shared<amx_dbg_info>(f);
	fclose(f);
	if(dbg->err != AMX_ERR_NONE)
	{
		return nullptr;
	}
	return std::shared_ptr<AMX_DBG>(dbg, &dbg->dbg);
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
	if(!fopen_trampoline)
	{
		fopen_trampoline = [](const char *pathname, const char *mode)
		{
			auto dst = subhook_read_dst(reinterpret_cast<void*>(fopen));
			auto olddst = subhook_get_dst(fopen_hook);
			if(dst != olddst)
			{
				fopen_hook->dst = dst;
				subhook_remove(fopen_hook);
				auto ret = fopen(pathname, mode);
				subhook_install(fopen_hook);
				fopen_hook->dst = olddst;
				return ret;
			}else if(!dst)
			{
				return fopen(pathname, mode);
			}else{
				subhook_remove(fopen_hook);
				auto ret = fopen(pathname, mode);
				subhook_install(fopen_hook);
				return ret;
			}
		};
	}
	subhook_install(fopen_hook);
}
#endif

void debug::clear_file()
{
	last_file = nullptr;
}
