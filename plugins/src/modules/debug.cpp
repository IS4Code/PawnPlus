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
#include <cstdlib>
#else
#include <stdio.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <cstring>
#include <memory>

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

template <class CharType, class StoreFunc>
static bool get_file_name(const CharType *name, size_t len, StoreFunc &&store)
{
	if(len > 0)
	{
		const CharType *last = name + len;
		const CharType *dot = nullptr;
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
			return store(name, dot);
		}
	}
	return false;
}

static std::unique_ptr<char[]> last_name;

static bool set_script_name(const char *name)
{
	return get_file_name(name, std::strlen(name), [](const char *name, const char *dot)
	{
		if(!std::strcmp(dot + 1, "amx"))
		{
			auto len = dot - name;
			last_name = std::make_unique<char[]>(len + 1);
			std::memcpy(last_name.get(), name, dot - name);
			last_name[len] = '\0';
			return true;
		}
		return false;
	});
}

#ifdef _WIN32
static bool set_script_name(const wchar_t *name)
{
	return get_file_name(name, std::wcslen(name), [](const wchar_t *name, const wchar_t *dot)
	{
		if(!std::wcscmp(dot + 1, L"amx"))
		{
			size_t len = dot - name;
			last_name = std::make_unique<char[]>(len * MB_LEN_MAX + 1);
			std::mbstate_t state{};
			len = 0;
			while(name != dot)
			{
				auto dst = last_name.get() + len;
				auto result = std::wcrtomb(dst, *name, &state);
				if(result != (size_t)-1)
				{
					len += result;
				}else{
					*dst = static_cast<char>(*name);
					len++;
				}
				++name;
			}
			last_name[len] = '\0';
			return true;
		}
		return false;
	});
}
#endif

template <class HookType>
class hook_data
{
public:
	static HookType &get_inst()
	{
		static HookType inst{};
		return inst;
	}
};

#ifdef _WIN32
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

std::unique_ptr<typename std::remove_pointer<HANDLE>::type, handle_delete> last_handle;

class win_hook
{
public:
	static std::shared_ptr<AMX_DBG> store_last(std::unique_ptr<char[]> &name)
	{
		if(!last_handle)
		{
			return nullptr;
		}
		int fd = _open_osfhandle(reinterpret_cast<intptr_t>(last_handle.get()), _O_RDONLY);
		if(fd == -1)
		{
			last_handle = nullptr;
			return nullptr;
		}
		last_handle.release();
		auto f = _fdopen(fd, "rb");
		if(!f)
		{
			_close(fd);
			return nullptr;
		}

		name = std::move(last_name);

		auto dbg = std::make_shared<amx_dbg_info>(f);
		fclose(f);
		if(dbg->err != AMX_ERR_NONE)
		{
			return nullptr;
		}
		return std::shared_ptr<AMX_DBG>(dbg, &dbg->dbg);
	}

	static void reset()
	{
		last_handle = nullptr;
	}
};

template <class CreateFileType, CreateFileType CreateFileFunc, class StringType>
class win_hook_data : public hook_data<win_hook_data<CreateFileType, CreateFileFunc, StringType>>
{
	subhook_t hook;
	CreateFileType trampoline;

	static HANDLE WINAPI hook_func(StringType lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
	{
		auto &inst = get_inst();
		if(!is_main_thread)
		{
			return inst.trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		}
		last_handle = nullptr;
		HANDLE hfile = inst.trampoline(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		if(hfile && (dwDesiredAccess & GENERIC_READ) && !last_handle)
		{
			if(set_script_name(lpFileName))
			{
				HANDLE handle;
				DuplicateHandle(GetCurrentProcess(), hfile, GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS);
				last_handle = std::unique_ptr<void, handle_delete>(handle);
			}
		}
		return hfile;
	}

public:
	void init()
	{
		auto func = reinterpret_cast<void*>(CreateFileFunc);
		auto dst = subhook_read_dst(func);
		if(dst != nullptr)
		{
			func = dst; // crashdetect may remove its hook, so hook crashdetect instead
		}
		hook = subhook_new(func, reinterpret_cast<void*>(static_cast<CreateFileType>(hook_func)), {});
		trampoline = reinterpret_cast<CreateFileType>(subhook_get_trampoline(hook));
		if(!trampoline)
		{
			trampoline = [](StringType lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
			{
				auto &inst = get_inst();
				auto dst = subhook_read_dst(reinterpret_cast<void*>(CreateFileFunc));
				auto olddst = subhook_get_dst(inst.hook);
				if(dst != olddst)
				{
					inst.hook->dst = dst;
					subhook_remove(inst.hook);
					auto ret = CreateFileFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
					subhook_install(inst.hook);
					inst.hook->dst = olddst;
					return ret;
				}else if(!dst)
				{
					return CreateFileFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
				}else{
					subhook_remove(inst.hook);
					auto ret = CreateFileFunc(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
					subhook_install(inst.hook);
					return ret;
				}
			};
		}
		subhook_install(hook);
	}
};

using win_hook_a = win_hook_data<decltype(&CreateFileA), &CreateFileA, LPCSTR>;
using win_hook_w = win_hook_data<decltype(&CreateFileW), &CreateFileW, LPCWSTR>;
#else
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

std::unique_ptr<void, descriptor_delete> last_file_descriptor;

class fopen_hook : public hook_data<fopen_hook>
{
	subhook_t hook;
	decltype(&fopen) trampoline;

	static FILE *hook_fopen(const char *pathname, const char *mode)
	{
		auto &inst = get_inst();
		if(!is_main_thread)
		{
			return inst.trampoline(pathname, mode);
		}
		last_file_descriptor = nullptr;
		auto file = inst.trampoline(pathname, mode);
		if(file && !std::strcmp(mode, "rb") && !last_file_descriptor)
		{
			if(set_script_name(pathname))
			{
				int fd = fileno(file);
				if(fd != -1)
				{
					last_file_descriptor = std::unique_ptr<void, descriptor_delete>(reinterpret_cast<void*>(dup(fd) + 1));
				}
			}
		}
		return file;
	}

public:
	static std::shared_ptr<AMX_DBG> store_last(std::unique_ptr<char[]> &name)
	{
		if(!last_file_descriptor)
		{
			return nullptr;
		}
		auto f = fdopen(reinterpret_cast<int>(last_file_descriptor.get()) - 1, "rb");
		if(!f)
		{
			last_file_descriptor = nullptr;
			return nullptr;
		}
		last_file_descriptor.release();

		name = std::move(last_name);
	
		auto dbg = std::make_shared<amx_dbg_info>(f);
		fclose(f);
		if(dbg->err != AMX_ERR_NONE)
		{
			return nullptr;
		}
		return std::shared_ptr<AMX_DBG>(dbg, &dbg->dbg);
	}

	static void reset()
	{
		last_file_descriptor = nullptr;
	}

	void init()
	{
		auto func = reinterpret_cast<void*>(fopen);
		auto dst = subhook_read_dst(func);
		if(dst != nullptr)
		{
			func = dst; // crashdetect may remove its hook, so hook crashdetect instead
		}
		hook = subhook_new(func, reinterpret_cast<void*>(static_cast<decltype(&fopen)>(hook_fopen)), {});
		trampoline = reinterpret_cast<decltype(&fopen)>(subhook_get_trampoline(hook));
		if(!trampoline)
		{
			trampoline = [](const char *pathname, const char *mode)
			{
				auto &inst = get_inst();
				auto dst = subhook_read_dst(reinterpret_cast<void*>(fopen));
				auto olddst = subhook_get_dst(inst.hook);
				if(dst != olddst)
				{
					inst.hook->dst = dst;
					subhook_remove(inst.hook);
					auto ret = fopen(pathname, mode);
					subhook_install(inst.hook);
					inst.hook->dst = olddst;
					return ret;
				}else if(!dst)
				{
					return fopen(pathname, mode);
				}else{
					subhook_remove(inst.hook);
					auto ret = fopen(pathname, mode);
					subhook_install(inst.hook);
					return ret;
				}
			};
		}
		subhook_install(hook);
	}
};
#endif

void debug::init()
{
#ifdef _WIN32
	win_hook_a::get_inst().init();
	win_hook_w::get_inst().init();
#else
	fopen_hook::get_inst().init();
#endif
}

std::shared_ptr<AMX_DBG> debug::create_last(std::unique_ptr<char[]> &name)
{
#ifdef _WIN32
	return win_hook::store_last(name);
#else
	return fopen_hook::store_last(name);
#endif
}

void debug::clear_file()
{
#ifdef _WIN32
	win_hook::reset();
#else
	fopen_hook::reset();
#endif
}
