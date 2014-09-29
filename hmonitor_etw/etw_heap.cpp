#include "stdafx.h"
#include "etw_event.h"
#include "track_system/track_system.h"


DEFINE_GUID ( /* 222962ab-6180-4b88-a825-346b75f2a24a */
    HeapTraceGuid,
    0x222962ab,
    0x6180,
    0x4b88,
    0xa8, 0x25, 0x34, 0x6b, 0x75, 0xf2, 0xa2, 0x4a
  );

#define TRACETYPE_HEAP_CREATE     0x20
#define TRACETYPE_HEAP_DESTROY    0x23
#define TRACETYPE_HEAP_EXPAND     0x25
#define TRACETYPE_HEAP_CONTRACT   0x2a
#define TRACETYPE_HEAP_ALLOC      0x21
#define TRACETYPE_HEAP_REALLOC    0x22
#define TRACETYPE_HEAP_FREE       0x24

#pragma pack(push, 1)

// Version 3 : From Win7 ?
struct HeapCreate_Event_32
{
	uint32_t HeapHandle;
	uint32_t HeapFlags;
	uint32_t ReservedSpace;
	uint32_t CommittedSpace;
	uint32_t AllocatedSpace;
};

// Version 3 : From Win7 ?
struct HeapCreate_Event_64
{
	uint64_t HeapHandle;
	uint32_t HeapFlags;
	uint64_t ReservedSpace;
	uint64_t CommittedSpace;
	uint64_t AllocatedSpace;
};

// Version 2 : From Win7 ?
struct HeapDestroy_Event_32
{
	uint32_t HeapHandle;
};

// Version 2 : From Win7 ?
struct HeapDestroy_Event_64
{
	uint64_t HeapHandle;
};

// Version 3 : From Win7 ?
struct HeapExpandContract_Event_32
{
	uint32_t HeapHandle;
	uint32_t Size;    // CommittedSize or DeCommittedSize
	uint32_t Address; // CommitAddress or DeCommitAddress
	uint32_t FreeSpace;
	uint32_t CommittedSpace;
	uint32_t ReservedSpace;
	uint32_t NoOfUCRs;
	uint32_t AllocatedSpace;
};

// Version 3 : From Win7 ?
struct HeapExpandContract_Event_64
{
	uint64_t HeapHandle;
	uint64_t Size;    // CommittedSize or DeCommittedSize
	uint64_t Address; // CommitAddress or DeCommitAddress
	uint64_t FreeSpace;
	uint64_t CommittedSpace;
	uint64_t ReservedSpace;
	uint32_t NoOfUCRs;
	uint64_t AllocatedSpace;
};

// Version 2 : From Win7 ?
struct HeapAlloc_Event_32
{
	uint32_t HeapHandle;
	uint32_t AllocSize;
	uint32_t AllocAddress;
	uint32_t SourceId;
};

// Version 2 : From Win7 ?
struct HeapAlloc_Event_64
{
	uint64_t HeapHandle;
	uint64_t AllocSize;
	uint64_t AllocAddress;
	uint32_t SourceId;
};

// Version 2 : From Win7 ?
struct HeapRealloc_Event_32
{
	uint32_t HeapHandle;
	uint32_t NewAllocAddress;
	uint32_t OldAllocAddress;
	uint32_t NewAllocSize;
	uint32_t OldAllocSize;
	uint32_t SourceId;
};

// Version 2 : From Win7 ?
struct HeapRealloc_Event_64
{
	uint64_t HeapHandle;
	uint64_t NewAllocAddress;
	uint64_t OldAllocAddress;
	uint64_t NewAllocSize;
	uint64_t OldAllocSize;
	uint32_t SourceId;
};

// Version 2 : From Win7 ?
struct HeapFree_Event_32
{
	uint32_t HeapHandle;
	uint32_t FreeAddress;
	uint32_t SourceId;
};

// Version 2 : From Win7 ?
struct HeapFree_Event_64
{
	uint64_t HeapHandle;
	uint64_t FreeAddress;
	uint32_t SourceId;
};

#pragma pack(pop)

template <template<class _Evt> class _Impl>
class CE_Heap_Create : public IEvent
{
	typedef _Impl<CE_Heap_Create> _This;

	uint64_t m_heapHandle;
	uint32_t m_heapFlags;
	uint64_t m_reservedSpace;
	uint64_t m_committedSpace;
	uint64_t m_allocatedSpace;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();

		STACKINFO siCreate = {0};
		pThis->stackS(siCreate.stackid, siCreate.depth, siCreate.frames);
		getTrackSystem()->OnHeapCreate(pThis->timeStampS(), pid, m_heapHandle, siCreate);
		getTrackSystem()->OnHeapSpaceChange(pid, m_heapHandle,
			true, m_committedSpace, 0);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			HeapCreate_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_heapFlags = raw.HeapFlags;
			m_reservedSpace = raw.ReservedSpace;
			m_committedSpace = raw.CommittedSpace;
			m_allocatedSpace = raw.AllocatedSpace;
		}
		else
		{
			HeapCreate_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_heapFlags = raw.HeapFlags;
			m_reservedSpace = raw.ReservedSpace;
			m_committedSpace = raw.CommittedSpace;
			m_allocatedSpace = raw.AllocatedSpace;
		}
		return CREATE_SUCCESS;
	}
};

template <template<class _Evt> class _Impl>
class CE_Heap_Destroy : public IEvent
{
	typedef _Impl<CE_Heap_Destroy> _This;

	uint64_t m_heapHandle;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();

		getTrackSystem()->OnHeapDestroy(pThis->timeStampS(), pid, m_heapHandle);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			HeapDestroy_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
		}
		else
		{
			HeapDestroy_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
		}
		return CREATE_SUCCESS;
	}
};

template <template<class _Evt> class _Impl>
class CE_Heap_ExpandContract : public IEvent
{
	typedef _Impl<CE_Heap_ExpandContract> _This;

	uint64_t m_heapHandle;
	bool m_fExpand;
	uint64_t m_changeSize;
	uint32_t m_noOfUCRs;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();

		getTrackSystem()->OnHeapSpaceChange(pid, m_heapHandle,
			m_fExpand, m_changeSize, m_noOfUCRs);
	}

public:
	int InitBase(bool f64, bool fExpand, char* &etData, size_t &etLen)
	{
		m_fExpand = fExpand;
		if (f64)
		{
			HeapExpandContract_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_changeSize = raw.Size;
			m_noOfUCRs = raw.NoOfUCRs;
		}
		else
		{
			HeapExpandContract_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_changeSize = raw.Size;
			m_noOfUCRs = raw.NoOfUCRs;
		}
		return CREATE_SUCCESS;
	}
};

template <template<class _Evt> class _Impl>
class CE_Heap_Alloc : public IEvent
{
	typedef _Impl<CE_Heap_Alloc> _This;

	uint64_t m_heapHandle;
	uint64_t m_allocAddress;
	uint64_t m_allocSize;
	uint32_t m_sourceId;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();
		uint32_t tid = pThis->tidS();

		// Emit
		STACKINFO siAlloc = {0};
		pThis->stackS(siAlloc.stackid, siAlloc.depth, siAlloc.frames);
		getTrackSystem()->OnHeapAlloc(pThis->timeStampS(), pid,
			m_heapHandle, m_allocAddress, m_allocSize,
			tid, siAlloc);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			HeapAlloc_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_allocAddress = raw.AllocAddress;
			m_allocSize = raw.AllocSize;
			m_sourceId = raw.SourceId;
		}
		else
		{
			HeapAlloc_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_allocAddress = raw.AllocAddress;
			m_allocSize = raw.AllocSize;
			m_sourceId = raw.SourceId;
		}
		return CREATE_SUCCESS;
	}
};

template <template<class _Evt> class _Impl>
class CE_Heap_Realloc : public IEvent
{
	typedef _Impl<CE_Heap_Realloc> _This;

	uint64_t m_heapHandle;
	uint64_t m_newAllocAddress;
	uint64_t m_newAllocSize;
	uint64_t m_oldAllocAddress;
	uint64_t m_oldAllocSize;
	uint32_t m_sourceId;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();
		uint32_t tid = pThis->tidS();

		// Emit
		STACKINFO siRealloc = {0};
		pThis->stackS(siRealloc.stackid, siRealloc.depth, siRealloc.frames);
		getTrackSystem()->OnHeapRealloc(pThis->timeStampS(), pid,
			m_heapHandle, m_newAllocAddress, m_newAllocSize, m_oldAllocAddress, m_oldAllocSize,
			tid, siRealloc);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			HeapRealloc_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_newAllocAddress = raw.NewAllocAddress;
			m_newAllocSize = raw.NewAllocSize;
			m_oldAllocAddress = raw.OldAllocAddress;
			m_oldAllocSize = raw.OldAllocSize;
			m_sourceId = raw.SourceId;
		}
		else
		{
			HeapRealloc_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_newAllocAddress = raw.NewAllocAddress;
			m_newAllocSize = raw.NewAllocSize;
			m_oldAllocAddress = raw.OldAllocAddress;
			m_oldAllocSize = raw.OldAllocSize;
			m_sourceId = raw.SourceId;
		}
		if (m_newAllocAddress != m_oldAllocAddress)
		{
			// have received: HeapAlloc + HeapFree
			return CREATE_IGNORE_EVENT;
		}
		return CREATE_SUCCESS;
	}
};

template <template<class _Evt> class _Impl>
class CE_Heap_Free : public IEvent
{
	typedef _Impl<CE_Heap_Free> _This;

	uint64_t m_heapHandle;
	uint64_t m_freeAddress;
	uint32_t m_sourceId;

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		uint32_t pid = pThis->pidS();

		// Emit
		getTrackSystem()->OnHeapFree(pThis->timeStampS(), pid,
			m_heapHandle, m_freeAddress);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			HeapFree_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_freeAddress = raw.FreeAddress;
			m_sourceId = raw.SourceId;
		}
		else
		{
			HeapFree_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_heapHandle = raw.HeapHandle;
			m_freeAddress = raw.FreeAddress;
			m_sourceId = raw.SourceId;
		}
		return CREATE_SUCCESS;
	}
};


class CEC_Heap
{
public:
	static
	int EventCreator(PEVENT_RECORD rawEvt, IEventReceiver* receiver)
	{
		if (!IsEqualGUID(rawEvt->EventHeader.ProviderId, HeapTraceGuid))
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
		case TRACETYPE_HEAP_CREATE:
			if (etVer == 3)
			{
				CStackEventImpl<CE_Heap_Create<CStackEventImpl> >* sevt =
					new CStackEventImpl<CE_Heap_Create<CStackEventImpl> >;
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

		case TRACETYPE_HEAP_DESTROY:
			if (etVer == 2)
			{
				CEventImpl<CE_Heap_Destroy<CEventImpl> >* evt =
					new CEventImpl<CE_Heap_Destroy<CEventImpl> >;
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

		case TRACETYPE_HEAP_EXPAND:
		case TRACETYPE_HEAP_CONTRACT:
			if (etVer == 3)
			{
				CEventImpl<CE_Heap_ExpandContract<CEventImpl> >* evt =
					new CEventImpl<CE_Heap_ExpandContract<CEventImpl> >;
				evt->initS(etTS, etPid, etTid);

				int ret;
				bool fExpand = (etType == TRACETYPE_HEAP_EXPAND);
				if ((ret = evt->InitBase(f64, fExpand, etData, etLen)) != CREATE_SUCCESS)
				{
					evt->destroyS();
					return ret;
				}

				receiver->newEvent(evt);
				return CREATE_SUCCESS;
			}

			return CREATE_IGNORE_VERSION;

		case TRACETYPE_HEAP_ALLOC:
			if (etVer == 2)
			{
				CStackEventImpl<CE_Heap_Alloc<CStackEventImpl> >* sevt =
					new CStackEventImpl<CE_Heap_Alloc<CStackEventImpl> >;
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

		case TRACETYPE_HEAP_REALLOC:
			if (etVer == 2)
			{
				CStackEventImpl<CE_Heap_Realloc<CStackEventImpl> >* sevt =
					new CStackEventImpl<CE_Heap_Realloc<CStackEventImpl> >;
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

		case TRACETYPE_HEAP_FREE:
			if (etVer == 2)
			{
				CEventImpl<CE_Heap_Free<CEventImpl> >* evt =
					new CEventImpl<CE_Heap_Free<CEventImpl> >;
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

static CEventRegister<CEC_Heap> _Register;
