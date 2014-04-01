#include "stdafx.h"
#include "etw_event.h"
#include "track_system/track_system.h"


DEFINE_GUID ( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
    ProcessTraceGuid,
    0x3d6fa8d0,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
  );

#define TRACETYPE_PROCESS_START    0x1
#define TRACETYPE_PROCESS_END      0x2

#pragma pack(push, 1)

// Version 3 : From Vista
struct ProcessStartEnd_Event_32
{
	uint32_t UniqueProcessKey;
	uint32_t ProcessId;
	uint32_t ParentId;
	uint32_t SessionId;
	uint32_t ExitStatus;
	uint32_t DirectoryTableBase;
	//object UserSID;
	//string ImageFileName; // NullTerminated
	//string CommandLine; // NullTerminated + Wide
};

// Version 3 : From Vista
struct ProcessStartEnd_Event_64
{
	uint64_t UniqueProcessKey;
	uint32_t ProcessId;
	uint32_t ParentId;
	uint32_t SessionId;
	uint32_t ExitStatus;
	uint64_t DirectoryTableBase;
	//object UserSID;
	//string ImageFileName; // NullTerminated
	//string CommandLine; // NullTerminated + Wide
};

#pragma pack(pop)


template <bool _Start, template<class _Evt> class _Impl>
class CE_Process_StartEnd : public IEvent
{
	typedef _Impl<CE_Process_StartEnd> _This;

	uint32_t m_processId;
	uint32_t m_parentId;
	uint32_t m_exitStatus;
	char m_imageFileName[MAX_PATH];

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		consume(pThis, Loki::Int2Type<_Start>());
	}

private:
	void consume(_This* pThis, Loki::Int2Type<true>)
	{
		getTrackSystem()->OnProcessStart(pThis->timeStampS(), m_processId, m_parentId, m_imageFileName);
	}
	void consume(_This* pThis, Loki::Int2Type<false>)
	{
		getTrackSystem()->OnProcessEnd(pThis->timeStampS(), m_processId, m_exitStatus);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			ProcessStartEnd_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_parentId = raw.ParentId;
			m_exitStatus = raw.ExitStatus;
		}
		else
		{
			ProcessStartEnd_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_parentId = raw.ParentId;
			m_exitStatus = raw.ExitStatus;
		}
		return CREATE_SUCCESS;
	}

	// Print the property.
	#define SeLengthSid( Sid ) (8 + (4 * ((SID*)Sid)->SubAuthorityCount))

	int InitSID(bool f64, char* &etData, size_t &etLen)
	{
		ULONG temp = 0;
		if (etLen < sizeof(ULONG))
			return CREATE_PARSE_FAILED;
		memcpy(&temp, etData, sizeof(ULONG));
		if (temp > 0)
		{
			char buffer[SECURITY_MAX_SID_SIZE];
			size_t hLen = 2 * ET_PSIZE(f64);
			if (etLen < hLen)
				return CREATE_PARSE_FAILED;
			etData += hLen;
			etLen  -= hLen;

			if (etLen < 8)
				return CREATE_PARSE_FAILED;
			memcpy(buffer, etData,
				etLen < SECURITY_MAX_SID_SIZE ? etLen : SECURITY_MAX_SID_SIZE);
			SID* psid = (SID*)buffer;
			size_t sLen = SeLengthSid(psid);
			etData += sLen;
			etLen  -= sLen;
		}
		else
		{
			etData += sizeof(ULONG);
			etLen  -= sizeof(ULONG);
		}
		return CREATE_SUCCESS;
	}

	int InitImage(char* &etData, size_t &etLen)
	{
		size_t len = strlen(etData) + 1;
		if (etLen < len)
			return CREATE_PARSE_FAILED;
		strncpy(m_imageFileName, etData, _ARRAYSIZE(m_imageFileName) - 1);
		m_imageFileName[_ARRAYSIZE(m_imageFileName) - 1] = '\0';
		etData += len;
		etLen  -= len;
		return CREATE_SUCCESS;
	}
};


class CEC_Process
{
public:
	static
	int EventCreator(PEVENT_RECORD rawEvt, IEventReceiver* receiver)
	{
		if (!IsEqualGUID(rawEvt->EventHeader.ProviderId, ProcessTraceGuid))
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
		case TRACETYPE_PROCESS_START:
			if (etVer == 3)
			{
				CEventImpl<CE_Process_StartEnd<true, CEventImpl> >* evt =
					new CEventImpl<CE_Process_StartEnd<true, CEventImpl> >;
				evt->initS(etTS, etPid, etTid);

				int ret;
				if ((ret = evt->InitBase(f64, etData, etLen)) != CREATE_SUCCESS ||
					(ret = evt->InitSID(f64, etData, etLen)) != CREATE_SUCCESS ||
					(ret = evt->InitImage(etData, etLen)) != CREATE_SUCCESS )
				{
					evt->destroyS();
					return ret;
				}

				receiver->newEvent(evt);
				return CREATE_SUCCESS;
			}

			return CREATE_IGNORE_VERSION;

		case TRACETYPE_PROCESS_END:
			if (etVer == 3)
			{
				CEventImpl<CE_Process_StartEnd<false, CEventImpl> >* evt =
					new CEventImpl<CE_Process_StartEnd<false, CEventImpl> >;
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

static CEventRegister<CEC_Process> _Register;
