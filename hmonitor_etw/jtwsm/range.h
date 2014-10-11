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

#include <map>

#ifndef ASSERT
#define ASSERT(_E)		((void)0)
#endif

namespace jtwsm {


// Range will be automatically merged
// range : [beg, end)

template <typename T>
class ranges
{
public:
	ranges(T min, T max)
		: m_min(min)
		, m_max(max)
	{
		ASSERT(m_min < m_max);
		m_idxs[m_min].is_beg = false;
		m_idxs[m_max].is_beg = true;
	}

	void add(T b, T e)
	{
		__CHECK(b, e);

		m_idxs[b].is_beg = true;
		m_idxs[e].is_beg = false;

		idxmap::iterator it, eb, ee;

		it = m_idxs.find(b);
		{
			idxmap::iterator aft = it; ++aft;
			idxmap::iterator bef = it; --bef;
			if (bef->second.is_beg)
				eb = it;
			else
				eb = aft;
		}

		it = m_idxs.find(e);
		{
			idxmap::iterator aft = it; ++aft;
			if (aft->second.is_beg)
				ee = it;
			else
				ee = aft;
		}

		// remove [eb, ee)
		m_idxs.erase(eb, ee);
	}

	void del(T b, T e)
	{
		__CHECK(b, e);

		m_idxs[b].is_beg = false;
		m_idxs[e].is_beg = true;

		idxmap::iterator it, eb, ee;

		it = m_idxs.find(b);
		{
			idxmap::iterator aft = it; ++aft;
			idxmap::iterator bef = it; --bef;
			if (bef->second.is_beg)
				eb = aft;
			else
				eb = it;
		}

		it = m_idxs.find(e);
		{
			idxmap::iterator aft = it; ++aft;
			if (aft->second.is_beg)
				ee = aft;
			else
				ee = it;
		}

		// remove [eb, ee)
		m_idxs.erase(eb, ee);
	}

	bool isin(T t) const
	{
		idxmap::const_iterator it = m_idxs.lower_bound(t);
		if (t == it->first)
			return it->second.is_beg;
		return !(it->second.is_beg);
	}

public:
	size_t size() const
	{
		size_t s = m_idxs.size();
		ASSERT(s > = 2 && s % 2 == 0);
		return s / 2 - 1;
	}

	template <class _C>
	void dump(_C &c) const
	{
		size_t s = m_idxs.size();
		ASSERT(s > = 2 && s % 2 == 0);
		idxmap::const_iterator it = m_idxs.begin(); ++it;
		idxmap::const_iterator end = m_idxs.end(); --end;
		for ( ; it != end; ++it)
		{
			idxmap::const_iterator aft = it; ++aft;
			c.push_back(it->first, aft->first);
			it = aft;
		}
	}

private:
	void __CHECK(T b, T e)
	{
		ASSERT(b < e);
		ASSERT(b > m_min);
		ASSERT(e < m_max);
	}

private:
	struct attr {
		bool is_beg;
	};
	typedef std::map<T, attr> idxmap;
	idxmap m_idxs;
	// all T must in (min, max)
	T m_min;
	T m_max;
};


} // namespace jtwsm

#endif // !__RANGE_H__
