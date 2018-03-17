#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include "sdk/amx/amx.h"

namespace Threads
{
	void DetachThread(AMX *amx, bool safe);
	void PauseThreads(AMX *amx);
	void ResumeThreads(AMX *amx);
	void SyncThreads();
}

#endif
