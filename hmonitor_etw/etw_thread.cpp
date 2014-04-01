#include "stdafx.h"
#include "etw_event.h"
#include "track_system/track_system.h"


DEFINE_GUID ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
    ThreadTraceGuid,
    0x3d6fa8d1,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

#define TRACETYPE_THREAD_START    0x1
#define TRACETYPE_THREAD_END      0x2

#pragma pack(push, 1)

// Version 3 : From Vista
struct ThreadStartEnd_Event_32
{
	uint32_t ProcessId;
	uint32_t ThreadId;
	uint32_t StackBase;
	uint32_t StackLimit;
	uint32_t UserStackBase;
	uint32_t UserStackLimit;
	uint32_t Affinity;
	uint32_t Win32StartAddr;
	uint32_t TebBase;
	uint32_t SubProcessTag;
	uint8_t  BasePriority;
	uint8_t  PagePriority;
	uint8_t  IoPriority;
	uint8_t  ThreadFlags;
};

// Version 3 : From Vista
struct ThreadStartEnd_Event_64
{
	uint32_t ProcessId;
	uint32_t ThreadId;
	uint64_t StackBase;
	uint64_t StackLimit;
	uint64_t UserStackBase;
	uint64_t UserStackLimit;
	uint64_t Affinity;
	uint64_t Win32StartAddr;
	uint64_t TebBase;
	uint32_t SubProcessTag;
	uint8_t  BasePriority;
	uint8_t  PagePriority;
	uint8_t  IoPriority;
	uint8_t  ThreadFlags;
};

#pragma pack(pop)


template <bool _Start, template<class _Evt> class _Impl>
class CE_Thread_StartEnd : public IEvent
{
	typedef _Impl<CE_Thread_StartEnd> _This;

	uint32_t m_processId;
	uint32_t m_threadId;
	uint64_t m_stackBase;
	uint64_t m_startAddr;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		consume(pThis, Loki::Int2Type<_Start>());
	}

private:
	void consume(_This* pThis, Loki::Int2Type<true>)
	{
		STACKINFO siStart = {0};
		pThis->stackS(siStart.stackid, siStart.depth, siStart.frames);
		getTrackSystem()->OnThreadStart(pThis->timeStampS(), m_processId, m_threadId, m_stackBase, m_startAddr, siStart);
	}
	void consume(_This* pThis, Loki::Int2Type<false>)
	{
		getTrackSystem()->OnThreadEnd(pThis->timeStampS(), m_processId, m_threadId);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			ThreadStartEnd_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_threadId = raw.ThreadId;
			m_stackBase = raw.UserStackBase;
			m_startAddr = raw.Win32StartAddr;
		}
		else
		{
			ThreadStartEnd_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_threadId = raw.ThreadId;
			m_stackBase = raw.UserStackBase;
			m_startAddr = raw.Win32StartAddr;
		}
		return CREATE_SUCCESS;
	}
};


class CEC_Thread
{
public:
	static
	int EventCreator(PEVENT_RECORD rawEvt, IEventReceiver* receiver)
	{
		if (!IsEqualGUID(rawEvt->EventHeader.ProviderId, ThreadTraceGuid))
			return CREATE_UNKNOWN_PROVIDER;

		int64_t  etTS   = rawEvt->EventHeader.TimeStamp.QuadPart;
		uint32_t etPid  = rawEvt->EventHeader.ProcessId;
		uint32_t etTid  = rawEvt->EventHeader.ThreadId;
		unsigned etType = rawEvt->EventHeader.EventDescriptor.Opcode;
		unsigned etVer  = rawEvt->EventHeader.EventDescriptor.Version;
		char*    etData = (char*)rawEvt->UserData;
		size_t   etLen  = rawEvt->UserDataLength;
		bool f64 = (rawEvt->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0;

		switch (etType)
		{
		case TRACETYPE_THREAD_START:
			if (etVer == 3)
			{
				CStackEventImpl<CE_Thread_StartEnd<true, CStackEventImpl> >* sevt =
					new CStackEventImpl<CE_Thread_StartEnd<true, CStackEventImpl> >;
				sevt->initS(etTS, etPid, etTid);

				int ret;
				if ((ret = sevt->InitBase(f64, etData, etLen)) != CREATE_SUCCESS)
				{
					sevt->destroyS();
					return ret;
				}

				receiver->newEvent(sevt);
				return CREATE_SUCCESS;
			}

			return CREATE_IGNORE_VERSION;

		case TRACETYPE_THREAD_END:
			if (etVer == 3)
			{
				CEventImpl<CE_Thread_StartEnd<false, CEventImpl> >* evt =
					new CEventImpl<CE_Thread_StartEnd<false, CEventImpl> >;
				evt->initS(etTS, etPid, etTid);

				int ret;
				if ((ret = evt->InitBase(f64, etData, etLen)) != CREATE_SUCCESS)
				{
					evt->destroyS();
					return ret;
				}

				receiver->newEvent(evt);
				return CREATE_SUCCESS;
			}

			return CREATE_IGNORE_VERSION;
		}

		return CREATE_IGNORE_EVENT;
	}
};

static CEventRegister<CEC_Thread> _Register;
