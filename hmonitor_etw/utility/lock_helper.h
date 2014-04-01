/* -------------------------------------------------------------------------
//	文件名		：	lock_helper.h
//	创建者		：	Jiangwangsheng
//	创建时间	：	2008-3-10 15:59:44
//	功能描述	：
//
//	$Id: $
// -----------------------------------------------------------------------*/
#ifndef __LOCK_HELPER_H__
#define __LOCK_HELPER_H__

#include <Windows.h>

// -------------------------------------------------------------------------
// KCriticalSection

class KCriticalSection
{
	CRITICAL_SECTION m_cs;

public:
	KCriticalSection() {
		::InitializeCriticalSection( &m_cs );
	}
	~KCriticalSection() {
		::DeleteCriticalSection( &m_cs );
	}

	void lock() {
		::EnterCriticalSection( &m_cs );
	}
	void unlock() {
		::LeaveCriticalSection( &m_cs );
	}
	bool tryLock() {
		BOOL fRet = ::TryEnterCriticalSection(&m_cs);
		return fRet ? true : false;
	}

private:
	KCriticalSection(const KCriticalSection &);
	KCriticalSection& operator=(const KCriticalSection &);
};

// -------------------------------------------------------------------------
// KAutoLock

template<class LockableObjectT>
class KAutoLock
{
	LockableObjectT *m_pObject;
	bool m_bLocked;

public:
	KAutoLock( LockableObjectT *pObject, bool bLock = true )
	: m_pObject(pObject), m_bLocked(false)
	{
		//ASSERT( m_pObject );
		if ( bLock )
			lock();
	}

	~KAutoLock()
	{
		if ( m_bLocked )
			unlock();
	}

	void lock()
	{
		//ASSERT( !m_bLocked );
		//ASSERT( m_pObject );
		m_pObject->lock();
		m_bLocked = true;
	}

	void unlock()
	{
		//ASSERT( m_bLocked );
		//ASSERT( m_pObject );
		m_pObject->unlock();
		m_bLocked = false;
	}

private:
	KAutoLock( const KAutoLock & );				// Not implement
	KAutoLock& operator=( const KAutoLock & );	// Not implement
};

// -------------------------------------------------------------------------
//	$Log: $

#endif /* __LOCK_HELPER_H__ */
