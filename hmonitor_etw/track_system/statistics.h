/*
 *  File   : track_system/statistics.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/20 14:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __TRACK_SYSTEM_STATISTICS_H__
#define __TRACK_SYSTEM_STATISTICS_H__

#include "Typelist.h"
#include "HierarchyGenerators.h"


template <typename T>
struct CountStat
{
	T total;
	T current;
	T peak;

	CountStat()
		: total(), current(), peak()
	{
	}

	void Increase(T i)
	{
		total += i;
		current += i;
		if (current > peak)
			peak = current;
	}
	void Decrease(T d)
	{
		current -= d;
	}
};


typedef CountStat<uint64_t> AllocCount;
typedef CountStat<uint64_t> AllocBytes;

// ...


typedef Loki::Tuple<LOKI_TYPELIST_2(AllocCount, AllocBytes)> StackStat;
typedef Loki::Tuple<LOKI_TYPELIST_2(AllocCount, AllocBytes)> ImageStat;
typedef Loki::Tuple<LOKI_TYPELIST_2(AllocCount, AllocBytes)> HeapStat;
typedef Loki::Tuple<LOKI_TYPELIST_2(AllocCount, AllocBytes)> ThreadStat;
typedef Loki::Tuple<LOKI_TYPELIST_2(AllocCount, AllocBytes)> ProcessStat;

enum
{
	AllocCountIdx = 0,
	AllocBytesIdx
};

template <class _SL>
class StatBase
{
public:
	template <int _I>
	typename Loki::FieldHelper<_SL, _I>::ResultType&
	StatBy()
	{
		return Loki::Field<_I>(m_sl);
	}

	template <int _I>
	typename Loki::FieldHelper<const _SL, _I>::ResultType&
	StatBy() const
	{
		return Loki::Field<_I>(m_sl);
	}

protected:
	_SL m_sl;
};


#endif /* __TRACK_SYSTEM_STATISTICS_H__ */
