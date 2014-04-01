#include "stdafx.h"
#include "etw_event.h"
#include <time.h>


DEFINE_GUID ( /* def2fe46-7bd6-4b80-bd94-f57fe20d0ce3 */
    StackTraceGuid,
    0xdef2fe46,
    0x7bd6,
    0x4b80,
    0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3
  );

#define TRACETYPE_STACK_RECORD    0x20

#pragma pack(push, 1)

// Version 2 : From Win7
struct StackWalk_Event_32
{
	int64_t  EventTimeStamp;
	uint32_t StackProcess;
	uint32_t StackThread;
	uint32_t Stack[192];
};

// Version 2 : From Win7
struct StackWalk_Event_64
{
	int64_t  EventTimeStamp;
	uint32_t StackProcess;
	uint32_t StackThread;
	uint64_t Stack[192];
};

#pragma pack(pop)


class CEC_Stack
{
public:
	static
	int ParseStackRecord(
		bool f64, char* &etData, size_t &etLen, IEventReceiver* receiver)
	{
		if (f64)
		{
			size_t minLen = sizeof(StackWalk_Event_64) - 192 * sizeof(uint64_t);
			if (etLen < minLen)
				return CREATE_PARSE_FAILED;
			size_t stackDepth = (etLen - minLen) / sizeof(uint64_t);

			StackWalk_Event_64 raw = {0};
			memcpy(&raw, etData, etLen);
			etData += etLen;
			etLen  -= etLen;

			size_t kernelSkip = 0;
			for (; kernelSkip < stackDepth; ++kernelSkip)
			{
				if (raw.Stack[kernelSkip] > 0x7ffffffffff)
					continue;
				break;
			}

			receiver->newStack(raw.EventTimeStamp, raw.StackProcess, raw.StackThread,
				stackDepth - kernelSkip, raw.Stack + kernelSkip);
		}
		else
		{
			size_t minLen = sizeof(StackWalk_Event_32) - 192 * sizeof(uint32_t);
			if (etLen < minLen)
				return CREATE_PARSE_FAILED;
			size_t stackDepth = (etLen - minLen) / sizeof(uint32_t);

			StackWalk_Event_32 raw = {0};
			memcpy(&raw, etData, etLen);
			etData += etLen;
			etLen  -= etLen;

			size_t kernelSkip = 0;
			for (; kernelSkip < stackDepth; ++kernelSkip)
			{
				if (raw.Stack[kernelSkip] > 0x7fffffff)
					continue;
				break;
			}

			receiver->newStack(raw.EventTimeStamp, raw.StackProcess, raw.StackThread,
				stackDepth - kernelSkip, raw.Stack + kernelSkip);
		}

		return CREATE_SUCCESS;
	}

	static
	int EventCreator(PEVENT_RECORD rawEvt, IEventReceiver* receiver)
	{
		if (!IsEqualGUID(rawEvt->EventHeader.ProviderId, StackTraceGuid))
			return CREATE_UNKNOWN_PROVIDER;

		unsigned etType = rawEvt->EventHeader.EventDescriptor.Opcode;
		unsigned etVer  = rawEvt->EventHeader.EventDescriptor.Version;
		char*    etData = (char*)rawEvt->UserData;
		size_t   etLen  = rawEvt->UserDataLength;
		bool f64 = (rawEvt->EventHeader.Flags & EVENT_HEADER_FLAG_64_BIT_HEADER) != 0;

		switch (etType)
		{
		case TRACETYPE_STACK_RECORD:
			if (etVer == 2)
			{
				return ParseStackRecord(f64, etData, etLen, receiver);
			}

			return CREATE_IGNORE_VERSION;
		}

		return CREATE_IGNORE_EVENT;
	}
};

static CEventRegister<CEC_Stack> _Register;
