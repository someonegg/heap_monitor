/*
 *  File   : etw_event.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/02/22 11:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __ETW_EVENT_H__
#define __ETW_EVENT_H__

#include <evntrace.h>


class IEvent
{
public:
	virtual int64_t timeStamp() = 0;
	virtual uint32_t pid() = 0;
	virtual uint32_t tid() = 0;

	virtual void consume() = 0;

	virtual void stack(
		uint64_t &id,
		size_t &depth,
		const uint64_t* &frames
		) = 0;
	virtual void setStack(
		uint64_t id,
		size_t depth,
		const uint64_t* frames
		) = 0;

	virtual void destroy() = 0;
};


template <class _Evt>
class CEventImpl : public _Evt
{
public:
	CEventImpl()
		: m_ts(0)
		, m_pid(0)
		, m_tid(0)
	{
	}

public:
	int64_t timeStamp()
	{
		return timeStampS();
	}
	uint32_t pid()
	{
		return pidS();
	}
	uint32_t tid()
	{
		return tidS();
	}
	void stack(
		uint64_t &id,
		size_t &depth,
		const uint64_t* &frames
		)
	{
		id = 0;
		depth = 0;
		frames = NULL;
	}
	void setStack(
		uint64_t id,
		size_t depth,
		const uint64_t* frames
		)
	{
	}
	void destroy()
	{
		destroyS();
	}

public:
	void initS(
		int64_t ts,
		uint32_t pid,
		uint32_t tid
		)
	{
		m_ts = ts;
		m_pid = pid;
		m_tid = tid;
	}
	int64_t timeStampS()
	{
		return m_ts;
	}
	uint32_t pidS()
	{
		return m_pid;
	}
	uint32_t tidS()
	{
		return m_tid;
	}
	void destroyS()
	{
		delete this;
	}

private:
	int64_t m_ts;
	uint32_t m_pid;
	uint32_t m_tid;
};


template <class _Evt>
class CStackEventImpl : public CEventImpl<_Evt>
{
public:
	CStackEventImpl()
		: m_id(0)
		, m_depth(0)
		, m_frames(NULL)
	{
	}
	~CStackEventImpl()
	{
	}

public:
	void stack(
		uint64_t &id,
		size_t &depth,
		const uint64_t* &frames
		)
	{
		stackS(id, depth, frames);
	}
	void setStack(
		uint64_t id,
		size_t depth,
		const uint64_t* frames
		)
	{
		setStackS(id, depth, frames);
	}
	void destroy()
	{
		destroyS();
	}

public:
	void stackS(
		uint64_t &id,
		size_t &depth,
		const uint64_t* &frames
		)
	{
		id = m_id;
		depth = m_depth;
		frames = m_frames;
	}
	void setStackS(
		uint64_t id,
		size_t depth,
		const uint64_t* frames
		)
	{
		m_id = id;
		m_depth = depth;
		m_frames = frames;
	}
	void destroyS()
	{
		delete this;
	}

private:
	uint64_t m_id;
	size_t m_depth;
	const uint64_t* m_frames;
};


// Create Event Errors
#define CREATE_SUCCESS           0
#define CREATE_PARSE_FAILED      1
#define CREATE_FILTERED          2
#define CREATE_UNKNOWN_PROVIDER  3
#define CREATE_IGNORE_EVENT      4
#define CREATE_IGNORE_VERSION    5


class IEventReceiver
{
public:
	virtual void newEvent(IEvent* evt) = 0;

	virtual void newStack(
		int64_t ts,
		uint32_t pid, uint32_t tid,
		size_t depth, uint32_t* frames
		) = 0;

	virtual void newStack(
		int64_t ts,
		uint32_t pid, uint32_t tid,
		size_t depth, uint64_t* frames
		) = 0;
};

class IEventFilter
{
public:
	// true : drop
	virtual bool filter(PEVENT_RECORD rawEvt) = 0;
};


typedef int (*FnEventCreator)(
	PEVENT_RECORD rawEvt,
	IEventReceiver* receiver
	);

void RegEventCreator(
	FnEventCreator fnEventCreator
	);

int EventCreate(
	PEVENT_RECORD rawEvt,
	IEventReceiver* receiver,
	IEventFilter* filter = NULL
	);


template<class _EC>
class CEventRegister
{
public:
	CEventRegister()
	{
		::RegEventCreator(_EC::EventCreator);
	}
};


#define ET_PSIZE(f64) ((f64) ? 8 : 4)


#endif /* __ETW_EVENT_H__ */
