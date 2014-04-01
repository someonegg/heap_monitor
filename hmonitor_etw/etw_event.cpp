#include "stdafx.h"
#include "etw_event.h"


typedef std::vector<FnEventCreator> EventFactory;
static EventFactory gEventFactory;

void RegEventCreator(
	FnEventCreator fnEventCreator
	)
{
	gEventFactory.push_back(fnEventCreator);
}

int EventCreate(
	PEVENT_RECORD rawEvt,
	IEventReceiver* receiver,
	IEventFilter* filter /* = NULL */
	)
{
	if (filter != NULL && filter->filter(rawEvt))
		return CREATE_FILTERED;

	for (EventFactory::iterator itor = gEventFactory.begin();
		itor != gEventFactory.end(); ++itor)
	{
		int ret = (*itor)(rawEvt, receiver);
		if (ret == CREATE_UNKNOWN_PROVIDER)
			continue;

		return ret;
	}

	return CREATE_UNKNOWN_PROVIDER;
}
