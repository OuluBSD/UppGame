#ifndef _CardEngine_Thread_h_
#define _CardEngine_Thread_h_

#include <Core/Core.h>

NAMESPACE_UPP

#define THREAD_WAIT_INFINITE	0xFFFFFFFF

class ThreadWrapper
{
public:
	ThreadWrapper();
	virtual ~ThreadWrapper();

	void Run();
	virtual void SignalTermination();
	bool Join(unsigned msecTimeout);
	bool IsRunning() const;
	static void Msleep(unsigned msecs);

protected:
	void MainWrapper();
	virtual void Main() = 0;
	bool ShouldTerminate() const;

private:
	mutable Semaphore m_isTerminatedSemaphore;
	mutable Semaphore m_shouldTerminateSemaphore;
	Thread m_threadObj;
	mutable Mutex m_threadObjMutex;
};

END_UPP_NAMESPACE

#endif
