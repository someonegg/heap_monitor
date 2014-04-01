/*
 *  File   : thread_base.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/05 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __THREAD_BASE_H__
#define __THREAD_BASE_H__

#include <process.h>

namespace jtwsm {


// T : ThreadWorker() public
template <class T>
class ThreadBase
{
protected:
	ThreadBase()
	{
		m_hThread = NULL;
		m_idThread = 0;
		m_needExit = false;
		m_exitCode = 0;
	}

	~ThreadBase()
	{
		ASSERT (m_hThread == NULL);
	}

	bool NeedExit()
	{
		return m_needExit;
	}

public:
	unsigned ThreadId()
	{
		return m_idThread;
	}

	bool IsRunning()
	{
		if (m_hThread == NULL)
			return false;
		return ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	unsigned ExitCode()
	{
		return m_exitCode;
	}

public:
	bool Start()
	{
		ASSERT (m_hThread == NULL);

		m_needExit = false;
		m_exitCode = 0;
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, &m_idThread);
		return m_hThread != NULL;
	}

	void Stop(unsigned msWait = INFINITE)
	{
		ASSERT (m_hThread != NULL);

		SetNeedExit();

		if (!WaitForExit(msWait))
		{
			::TerminateThread(m_hThread, 0);
		}

		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	void SetNeedExit()
	{
		m_needExit = true;
	}

	bool WaitForExit(unsigned ms = INFINITE)
	{
		ASSERT (m_hThread != NULL);

		return ::WaitForSingleObject(m_hThread, ms) == WAIT_OBJECT_0;
	}

private:
	static unsigned __stdcall threadFunc(void* args)
	{
		ThreadBase* base = (ThreadBase*)args;
		T* pThis = static_cast<T*>(base);
		base->m_exitCode = pThis->ThreadWorker();
		return base->m_exitCode;
	}

private:
	HANDLE m_hThread;
	unsigned m_idThread;
	bool m_needExit;
	unsigned m_exitCode;
};


} // namespace jtwsm

#endif // __THREAD_BASE_H__
