#include "stdafx.h"
#include "etw_event.h"
#include "track_system/track_system.h"
#include "utility/lddn_helper.h"


DEFINE_GUID ( /* 2cb15d1d-5fc1-11d2-abe1-00a0c911f518 */
    ImageTraceGuid,
    0x2cb15d1d,
    0x5fc1,
    0x11d2,
    0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18
  );

#define TRACETYPE_IMAGE_LOAD      0xA
#define TRACETYPE_IMAGE_UNLOAD    0x2

#pragma pack(push, 1)

// Version 2: From Vista
struct ImageLoadUnload_Event_32
{
	uint32_t ImageBase;
	uint32_t ImageSize;
	uint32_t ProcessId;
	uint32_t ImageCheckSum;
	uint32_t TimeDateStamp;
	uint32_t Reserved0;
	uint32_t DefaultBase;
	uint32_t Reserved1;
	uint32_t Reserved2;
	uint32_t Reserved3;
	uint32_t Reserved4;
	//string FileName; // NullTerminated + Wide
};

// Version 2: From Vista
struct ImageLoadUnload_Event_64
{
	uint64_t ImageBase;
	uint64_t ImageSize;
	uint32_t ProcessId;
	uint32_t ImageCheckSum;
	uint32_t TimeDateStamp;
	uint32_t Reserved0;
	uint64_t DefaultBase;
	uint32_t Reserved1;
	uint32_t Reserved2;
	uint32_t Reserved3;
	uint32_t Reserved4;
	//string FileName; // NullTerminated + Wide
};

#pragma pack(pop)


template <bool _Load, template<class _Evt> class _Impl>
class CE_Image_LoadUnload : public IEvent
{
	typedef _Impl<CE_Image_LoadUnload> _This;

	uint32_t m_processId;
	uint64_t m_imageBase;
	uint32_t m_imageSize;
	char m_imageFileName[MAX_PATH + 1];

public:
	void consume()
	{
		_This* pThis = static_cast<_This*>(this);
		consume(pThis, Loki::Int2Type<_Load>());
	}

private:
	void consume(_This* pThis, Loki::Int2Type<true>)
	{
		STACKINFO siLoad = {0};
		pThis->stackS(siLoad.stackid, siLoad.depth, siLoad.frames);
		getTrackSystem()->OnImageLoad(pThis->timeStampS(), m_processId, m_imageBase, m_imageSize, m_imageFileName, siLoad);
	}
	void consume(_This* pThis, Loki::Int2Type<false>)
	{
		getTrackSystem()->OnImageUnload(pThis->timeStampS(), m_processId, m_imageBase);
	}

public:
	int InitBase(bool f64, char* &etData, size_t &etLen)
	{
		if (f64)
		{
			ImageLoadUnload_Event_64 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_imageBase = raw.ImageBase;
			m_imageSize = (uint32_t)raw.ImageSize;
		}
		else
		{
			ImageLoadUnload_Event_32 raw = {0};
			if (etLen < sizeof(raw))
				return CREATE_PARSE_FAILED;
			memcpy(&raw, etData, sizeof(raw));
			etData += sizeof(raw);
			etLen  -= sizeof(raw);

			m_processId = raw.ProcessId;
			m_imageBase = raw.ImageBase;
			m_imageSize = (uint32_t)raw.ImageSize;
		}
		return CREATE_SUCCESS;
	}

	int InitImage(char* &etData, size_t &etLen)
	{
		size_t len = (wcslen((const wchar_t*)etData) + 1) * sizeof(wchar_t);
		if (etLen < len)
			return CREATE_PARSE_FAILED;

		wchar_t deviceForm[MAX_PATH];
		wcsncpy(deviceForm, (const wchar_t*)etData, _ARRAYSIZE(deviceForm) - 1);
		deviceForm[_ARRAYSIZE(deviceForm) - 1] = L'\0';
		etData += len;
		etLen  -= len;

		wchar_t dosForm[MAX_PATH];
		g_lddnHelper.DeviceForm2DosForm(deviceForm, dosForm);

		wcstombs(m_imageFileName, dosForm, MAX_PATH);
		m_imageFileName[MAX_PATH] = 0;

		return CREATE_SUCCESS;
	}
};


class CEC_Image
{
public:
	static
	int EventCreator(PEVENT_RECORD rawEvt, IEventReceiver* receiver)
	{
		if (!IsEqualGUID(rawEvt->EventHeader.ProviderId, ImageTraceGuid))
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
		case TRACETYPE_IMAGE_LOAD:
			if (etVer == 2)
			{
				CStackEventImpl<CE_Image_LoadUnload<true, CStackEventImpl> >* sevt =
					new CStackEventImpl<CE_Image_LoadUnload<true, CStackEventImpl> >;
				sevt->initS(etTS, etPid, etTid);

				int ret;
				if ((ret = sevt->InitBase(f64, etData, etLen)) != CREATE_SUCCESS ||
					(ret = sevt->InitImage(etData, etLen)) != CREATE_SUCCESS )
				{
					sevt->destroyS();
					return ret;
				}

				receiver->newEvent(sevt);
				return CREATE_SUCCESS;
			}

			return CREATE_IGNORE_VERSION;

		case TRACETYPE_IMAGE_UNLOAD:
			if (etVer == 2)
			{
				CEventImpl<CE_Image_LoadUnload<false, CEventImpl> >* evt =
					new CEventImpl<CE_Image_LoadUnload<false, CEventImpl> >;
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

static CEventRegister<CEC_Image> _Register;
