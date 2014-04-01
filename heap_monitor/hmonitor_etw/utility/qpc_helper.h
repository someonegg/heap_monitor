/*
 *  File   : qpc_helper.h
 *  Author : Jiang Wangsheng
 *  Date   : 2013/8/7 15:09:09
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __QPC_HELPER_H__
#define __QPC_HELPER_H__

#include <Windows.h>

class CQPCHelper
{
	LARGE_INTEGER m_frequency;

public:
	CQPCHelper()
	{
		QueryPerformanceFrequency(&m_frequency);
	}

	inline float QPCToMS(long long duration)
	{
		float flSeconds = (float)(duration / double(m_frequency.QuadPart));
		return flSeconds * 1000;
	}

	inline long long GetQPCNow()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return now.QuadPart;
	}

	// msFrom:
	//   before negative | after positive
	inline long long GetQPCFrom(long long origin, float msFrom)
	{
		long long ll = ((long long)(msFrom * (double)m_frequency.QuadPart)) / 1000;

		return origin + ll;
	}

	// msFromNow:
	//   before negative | after positive
	inline long long GetQPCFromNow(float msFromNow)
	{
		return GetQPCFrom(GetQPCNow(), msFromNow);
	}

	inline float GetTimeSpentMS(long long qpcBegin)
	{
		return QPCToMS(GetQPCNow() - qpcBegin);
	}
};

extern CQPCHelper g_QPCHelper;

#endif /* __QPC_HELPER_H__ */
