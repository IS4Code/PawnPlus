#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include "sdk/amx/amx.h"

namespace Threads
{
	enum class SyncFlags : cell
	{
		SyncExplicit = 0,
		SyncAuto = 1,
		SyncInterrupt = 2
	};
	constexpr SyncFlags operator|(SyncFlags lhs, SyncFlags rhs)
	{
		return (SyncFlags)((cell)lhs | (cell)rhs);
	}
	constexpr SyncFlags operator&(SyncFlags lhs, SyncFlags rhs)
	{
		return (SyncFlags)((cell)lhs & (cell)rhs);
	}

	void DetachThread(AMX *amx, SyncFlags flags);
	void PauseThreads(AMX *amx);
	void ResumeThreads(AMX *amx);
	void JoinThreads(AMX *amx);
	void StartThreads();
	void SyncThreads();

	void QueueAndWait(AMX *amx, cell &retval, int &error);
}

#endif
