#include "Thread.h"

NAMESPACE_UPP

ThreadWrapper::ThreadWrapper() {}
ThreadWrapper::~ThreadWrapper() { Join(THREAD_WAIT_INFINITE); }

void ThreadWrapper::Run()
{
	Mutex::Lock lock(m_threadObjMutex);
	if (!m_threadObj.IsOpen()) {
		m_threadObj.Run([this]{ MainWrapper(); });
	}
}

void ThreadWrapper::SignalTermination()
{
	m_shouldTerminateSemaphore.Release();
}

bool ThreadWrapper::Join(unsigned msecTimeout)
{
	if (!IsRunning()) return true;
	
	bool terminated = false;
	if (msecTimeout == THREAD_WAIT_INFINITE) {
		m_isTerminatedSemaphore.Wait();
		terminated = true;
	} else {
		terminated = m_isTerminatedSemaphore.Wait((int)msecTimeout);
	}

	if (terminated) {
		Mutex::Lock lock(m_threadObjMutex);
		if (m_threadObj.IsOpen()) {
			m_threadObj.Wait();
		}
	}
	return terminated;
}

void ThreadWrapper::Msleep(unsigned msecs)
{
	Thread::Sleep((int)msecs);
}

void ThreadWrapper::MainWrapper()
{
	Main();
	m_isTerminatedSemaphore.Release();
}

bool ThreadWrapper::ShouldTerminate() const
{
	return m_shouldTerminateSemaphore.Wait(0);
}

bool ThreadWrapper::IsRunning() const
{
	Mutex::Lock lock(m_threadObjMutex);
	return m_threadObj.IsOpen();
}

END_UPP_NAMESPACE
