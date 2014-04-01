/*
 *  File   : nameidx_map.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/02/26 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __NAMEIDX_MAP_H__
#define __NAMEIDX_MAP_H__

#include <map>
#include <xstring>

namespace jtwsm {


template <class _C>
int str_cmp (const _C* l, const _C* r)
{
	int ret = 0 ;

	while (!(ret = (int)((wchar_t)(*l) - (wchar_t)(*r))) && *r)
		++l, ++r;

	if (ret < 0)
		ret = -1;
	else if (ret > 0)
		ret = 1;

	return ret;
}

template <class _C>
class const_string
{
public:
	const_string()
		: m_str(NULL)
	{
	}
	const_string(const _C* str)
		: m_str(str)
	{
	}
	// default copy constructor
	~const_string()
	{
		m_str = NULL;
	}

	const_string& operator=(const _C* str)
	{
		m_str = str;
		return *this;
	}
	// default assignment function

	bool operator<(const const_string& r) const
	{
		return str_cmp<_C>(m_str, r.m_str) < 0;
	}

	const _C* c_str() const
	{
		return m_str;
	}

private:
	const _C* m_str;
};

template <class _C>
class nameidx_map
{
public:
	typedef nameidx_map<_C> _Myt;

	nameidx_map()
		: m_nextIdx(0)
	{
	}
	nameidx_map(const _Myt &r)
		: m_nextIdx(0)
	{
		for (size_t i = 0; i < r.end(); ++i)
		{
			append(r.name(i));
		}
	}
	_Myt & operator=(const _Myt &r)
	{
		if (this == &r)
			return *this;

		m_nextIdx = 0;
		m_idx2Name.clear();
		m_name2Idx.clear();

		for (size_t i = 0; i < r.end(); ++i)
		{
			append(r.name(i));
		}

		return *this;
	}
	~nameidx_map()
	{
	}

public:
	enum
	{
		INVALID_INDEX = -1
	};

public:
	size_t begin() const
	{
		return 0;
	}
	size_t end() const
	{
		return m_nextIdx;
	}

	size_t append(const _C* str)
	{
		_cs s(str);

		size_t idx;

		Name2IdxMap::const_iterator itor = m_name2Idx.find(s);
		if (itor != m_name2Idx.end())
		{
			idx = itor->second;
		}
		else
		{
			idx = m_nextIdx++;
			m_idx2Name[idx] = str;
			s = m_idx2Name[idx].c_str();
			m_name2Idx[s] = idx;
		}

		return idx;
	}

	size_t index(const _C* str) const
	{
		_cs s(str);

		Name2IdxMap::const_iterator itor = m_name2Idx.find(s);
		if (itor != m_name2Idx.end())
		{
			return itor->second;
		}

		return INVALID_INDEX;
	}

	const _C* name(size_t idx) const
	{
		Idx2NameMap::const_iterator itor = m_idx2Name.find(idx);
		if (itor != m_idx2Name.end())
		{
			return itor->second.c_str();
		}

		return NULL;
	}

private:
	size_t m_nextIdx;

	typedef std::basic_string<_C, std::char_traits<_C>, std::allocator<_C> > _string;
	typedef std::map<size_t, _string> Idx2NameMap;
	Idx2NameMap m_idx2Name;

	typedef const_string<_C> _cs;
	typedef std::map<_cs, size_t> Name2IdxMap;
	Name2IdxMap m_name2Idx;
};


} // namespace jtwsm

#endif /* __NAMEIDX_MAP_H__ */
