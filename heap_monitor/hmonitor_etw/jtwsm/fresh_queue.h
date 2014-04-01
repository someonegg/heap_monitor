/*
 *  File   : fresh_queue.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/12 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __FRESH_QUEUE_H__
#define __FRESH_QUEUE_H__

#include <stdint.h>
#include "fixed_queue.h"
#include "n_d_traver.h"

namespace jtwsm {


// _E : createtime() public
template<class _E,
	class _Time = int64_t,
	size_t _BlockSize = 512,
	class _Container = popblock_queue<_E, _BlockSize> >
class fresh_queue
{
public:
	typedef _Container Container;
	typedef typename Container::Block Block;
	typedef std::list<Block> BlockList;

	typedef two_d_traver<_E, Block, BlockList> BlockListTraver;

public:
	fresh_queue()
	{
		m_staletime = 0;
		m_skip = 0;

		m_opt = 0;
		m_nor = 0;
	}
	~fresh_queue()
	{
	}

	_Time staletime() const
	{
		return m_staletime;
	}

	// staletime must increase progressively
	bool set_staletime(_Time t)
	{
		if (t < m_staletime)
			return false;
		m_staletime = t;
		return true;
	}

	const Container & container() const
	{
		return m_queue;
	}

	// 1. e's createtime must be greater than staletime
	// 2. the push input is ordered
	bool push(const _E &e)
	{
		if (e.createtime() <= m_staletime)
			return false;

		m_queue.push(e);

		return true;
	}

	// append
	void pop_all(BlockList &r)
	{
		while (!m_queue.empty())
		{
			r.resize(r.size() + 1);
			Block &rb = r.back();

			m_queue.popto(rb);
			if (m_skip != 0)
			{
				rb.erase(rb.begin(), rb.begin() + m_skip);
				m_skip = 0;
			}
		}
	}

	// append
	void pop_stales(BlockList &r)
	{
		while (!m_queue.empty())
		{
			const Block &b = m_queue.front();

			if ((b.begin() + m_skip)->createtime() > m_staletime)
			{
				break;
			}

			r.resize(r.size() + 1);
			Block &rb = r.back();

			if (b.rbegin()->createtime() <= m_staletime)
			{
				++m_opt;

				m_queue.popto(rb);
				if (m_skip != 0)
				{
					rb.erase(rb.begin(), rb.begin() + m_skip);
					m_skip = 0;
				}
				continue;
			}

			++m_nor;

			rb.reserve(_BlockSize - m_skip);

			for (Block::const_iterator itorb = b.begin() + m_skip;
				itorb != b.end(); ++itorb)
			{
				if (itorb->createtime() > m_staletime)
				{
					break;
				}

				rb.push_back(*itorb);
				++m_skip;
			}

			break;
		}
	}

private:
	_Time m_staletime;
	Container m_queue;
	typename Container::size_type m_skip;

	size_t m_opt;
	size_t m_nor;
};


} // namespace jtwsm

#endif /* __FRESH_QUEUE_H__ */
