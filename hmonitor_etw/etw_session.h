/*
 *  File   : etw_session.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/03 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __ETW_SESSION_H__
#define __ETW_SESSION_H__

#include "etw_event.h"
#include "jtwsm/thread_base.h"
#include "jtwsm/fresh_queue.h"
#include "utility/lock_helper.h"


class StackTable
{
public:
	StackTable()
	{
	}
	~StackTable()
	{
		for (StackMap::iterator itor = m_stacks.begin();
			itor != m_stacks.end(); ++itor)
		{
			ASSERT (itor->second.frames != NULL);
			free(itor->second.frames);
		}
		m_stacks.clear();
	}

public:
	uint64_t append(size_t depth, uint32_t* frames, const uint64_t* &constFrames)
	{
		uint64_t id = genId(depth, frames);

		StackMap::iterator itor = m_stacks.find(id);
		if (itor != m_stacks.end())
		{
			constFrames = itor->second.frames;
			return id;
		}

		Stack stack;
		stack.depth = depth;
		stack.frames = (uint64_t*)malloc(depth * sizeof(uint64_t));

		for (size_t i = 0; i < depth; ++i)
			stack.frames[i] = frames[i];

		m_stacks[id] = stack;

		constFrames = stack.frames;
		return id;
	}
	uint64_t append(size_t depth, uint64_t* frames, const uint64_t* &constFrames)
	{
		uint64_t id = genId(depth, frames);

		StackMap::iterator itor = m_stacks.find(id);
		if (itor != m_stacks.end())
		{
			constFrames = itor->second.frames;
			return id;
		}

		Stack stack;
		stack.depth = depth;
		stack.frames = (uint64_t*)malloc(depth * sizeof(uint64_t));

		memcpy(stack.frames, frames, stack.depth * sizeof(uint64_t));

		m_stacks[id] = stack;

		constFrames = stack.frames;
		return id;
	}

	bool query(uint64_t id, size_t &depth, uint64_t* &frames)
	{
		StackMap::iterator itor = m_stacks.find(id);
		if (itor != m_stacks.end())
		{
			depth = itor->second.depth;
			frames = itor->second.frames;
			return true;
		}
		return false;
	}

private:
	struct Stack
	{
		size_t depth;
		uint64_t* frames;
	};
	typedef std::map<uint64_t, Stack> StackMap;
	StackMap m_stacks;

	union StackId
	{
		// little endian && x64/x86 address space layout
		struct {
		uint64_t sum : 56;
		uint64_t cnt : 8;
		};
		uint64_t u64;
	};

	template<class T>
	uint64_t genId(size_t depth, T* frames)
	{
		StackId id; id.u64 = 0;
		for (size_t i = 0; i < depth; ++i)
			id.sum += frames[i];
		id.cnt = depth;
		return id.u64;
	}
};


template <class _IEvt>
class EventWrapper
{
	typedef EventWrapper<_IEvt> _MyT;

public:
	EventWrapper()
	{
		m_evt = NULL;
		m_ts  = 0;
	}
	EventWrapper(_IEvt* evt)
	{
		m_evt = evt;
		m_ts  = m_evt->timeStamp();
	}
	EventWrapper& operator=(_IEvt* evt)
	{
		m_evt = evt;
		m_ts  = m_evt->timeStamp();
		return *this;
	}
	~EventWrapper()
	{
		m_evt = NULL;
		m_ts  = 0;
	}

	int64_t createtime() const
	{
		return m_ts;
	}

	_IEvt* inner() const
	{
		return m_evt;
	}

	bool operator<(int64_t ts) const
	{
		return createtime() < ts;
	}

	bool operator<(const _MyT &r) const
	{
		return (*this) < r.createtime();
	}

private:
	_IEvt* m_evt;
	int64_t m_ts;
};

typedef EventWrapper<IEvent> Event;
typedef jtwsm::fresh_queue<Event> EventQueue;


class ETWSession
	: public jtwsm::ThreadBase<ETWSession>
	, public IEventReceiver
{
public:
	ETWSession();
	~ETWSession();

public:
	bool Init(
		bool fRealtime,
		LPCWSTR sessionName,
		IEventFilter* filter = NULL
		);

	bool Flush();

	void PopStales(int64_t staleTime, EventQueue::BlockList &bl);

	int64_t LastTS() const { return m_lastTS; }

	int64_t MaxDur() const { return m_maxDur; }

	struct STATUS
	{
		uint64_t total;
		uint64_t success;
		uint64_t parseFailed;
		uint64_t filtered;
		uint64_t unknownProvider;
		uint64_t eventIgnore;
		uint64_t versionIgnore;

		int64_t lastTS;
		int64_t maxDur;
	};

	void Status(STATUS &status) const
	{
		status.total = m_total;
		status.success = m_success;
		status.parseFailed = m_parseFailed;
		status.filtered = m_filtered;
		status.unknownProvider = m_unknownProvider;
		status.eventIgnore = m_eventIgnore;
		status.versionIgnore = m_versionIgnore;
		status.lastTS = m_lastTS;
		status.maxDur = m_maxDur;
	}

public:
	void newEvent(IEvent* evt);

	void newStack(
		int64_t ts,
		uint32_t pid, uint32_t tid,
		size_t depth, uint32_t* frames
		);

	void newStack(
		int64_t ts,
		uint32_t pid, uint32_t tid,
		size_t depth, uint64_t* frames
		);

	void newStack(
		int64_t ts,
		uint32_t pid, uint32_t tid,
		uint64_t id, size_t depth, const uint64_t* frames
		);

public:
	unsigned ThreadWorker();

private:
	static void __stdcall ProcessEvent(PEVENT_RECORD pEvent);
	void processEvent(PEVENT_RECORD pEvent);

private:
	KCriticalSection m_lock;
	typedef KAutoLock<KCriticalSection> AutoLock;

	bool m_fRealtime;
	std::wstring m_sessionName;
	IEventFilter* m_filter;
	EVENT_TRACE_PROPERTIES* m_etwProperties;

	StackTable m_stackTable;
	EventQueue m_evtQueue;

	int64_t m_lastTS;
	int64_t m_maxDur;

	uint64_t m_wildStack;

	// status
	uint64_t m_total;
	uint64_t m_success;
	uint64_t m_parseFailed;
	uint64_t m_filtered;
	uint64_t m_unknownProvider;
	uint64_t m_eventIgnore;
	uint64_t m_versionIgnore;
};


#endif /* __ETW_SESSION_H__ */
