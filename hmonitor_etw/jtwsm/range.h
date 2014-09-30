/*
 *  File   : range.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/09/27 14:21:23
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __RANGE_H__
#define __RANGE_H__

#include <utility>
#include <list>

#ifndef ASSERT
#define ASSERT(_E)		((void)0)
#endif

namespace jtwsm {


// Range will be automatically merged

template <typename T>
class RangeManager
{
public:
	// [first,last)
	typedef std::pair<T, T> Range;
	typedef std::list<Range> Ranges;

	Ranges ranges;

	void add(T first, T last)
	{
		ASSERT(first < last);

		Range rn(first, last);
		__add(rn);
	}

	void del(T first, T last)
	{
		ASSERT(first < last);

		Range rn(first, last);
		__del(rn);
	}

	bool isin(T t)
	{
		typename Ranges::const_iterator itor = ranges.begin();
		for ( ; itor != ranges.end(); ++itor)
		{
			if (t >= itor->first && t < itor->second)
			{
				return true;
			}
		}
		return false;
	}

private:
	void __add(Range &rn)
	{
		typename Ranges::iterator itor = ranges.begin();
		for ( ; itor != ranges.end(); )
		{
			if ( rn.second < itor->first )
			{
				ranges.insert(itor, rn);
				return;
			}

			if ( rn.first < itor->first /* &&
				rn.second >= itor->first */ )
			{
				if ( rn.second <= itor->second )
				{
					itor->first = rn.first;
					return;
				}

				itor = ranges.erase(itor);
				continue;
			}

			if ( /* rn.first >= itor->first && */
				rn.first <= itor->second )
			{
				if ( rn.second <= itor->second )
					return;

				rn.first = itor->first;
				itor = ranges.erase(itor);
				continue;
			}

			++itor;
		}

		ASSERT(itor == ranges.end());
		ranges.insert(itor, rn);
		return;
	}

	void __del(Range &rn)
	{
		typename Ranges::iterator itor = ranges.begin();
		for ( ; itor != ranges.end(); )
		{
			if ( rn.second <= itor->first )
			{
				return;
			}

			if ( rn.first <= itor->first /* &&
				rn.second > itor->first */ )
			{
				if ( rn.second < itor->second )
				{
					itor->first = rn.second;
					return;
				}
				if (rn.second == itor->second)
				{
					ranges.erase(itor);
					return;
				}

				rn.first = itor->second;
				itor = ranges.erase(itor);
				continue;
			}

			if ( /* rn.first > itor->first && */
				rn.first < itor->second )
			{
				if ( rn.second < itor->second )
				{
					Range rnn(rn.second, itor->second);
					itor->second = rn.first;
					ranges.insert(++itor, rnn);
					return;
				}
				if (rn.second == itor->second)
				{
					itor->second = rn.first;
					return;
				}

				rn.first = itor->second;
				++itor;
				continue;
			}

			++itor;
		}

		return;
	}
};


} // namespace jtwsm

#endif // !__RANGE_H__
