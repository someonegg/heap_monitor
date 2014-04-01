/*
 *  File   : fixed_queue.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/12 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __FIXED_QUEUE_H__
#define __FIXED_QUEUE_H__

#include <vector>
#include <list>
#include <exception>

namespace jtwsm {


class INVALID_INDEX: public std::exception
{
	virtual const char* what() const throw()
	{
		return "popblock_queue : invalid element index";
	}
};


template<class _MyQueue>
class _pbq_const_iterator
	: public std::iterator<
		std::random_access_iterator_tag,
		typename _MyQueue::value_type,
		typename _MyQueue::difference_type,
		typename _MyQueue::const_pointer,
		typename _MyQueue::const_reference
		>
{
public:
	typedef _pbq_const_iterator<_MyQueue> _Myiter;
	typedef std::random_access_iterator_tag iterator_category;

	typedef typename _MyQueue::value_type value_type;
	typedef typename _MyQueue::difference_type difference_type;
	typedef typename _MyQueue::const_pointer pointer;
	typedef typename _MyQueue::const_reference reference;
	typedef typename _MyQueue::size_type size_type;

public:
	_pbq_const_iterator(
		const _MyQueue* host = NULL, size_type idx = 0)
		: m_host(host)
		, m_idx(idx)
	{	// construct with null pointer
	}

	reference operator*() const
	{	// return designated object
		return m_host->elem(m_idx);
	}

	pointer operator->() const
	{	// return pointer to class object
		return (std::pointer_traits<pointer>::pointer_to(**this));
	}

	_Myiter & operator++()
	{	// preincrement
		++m_idx;
		return (*this);
	}

	_Myiter operator++(int)
	{	// postincrement
		_Myiter _Tmp = *this;
		++*this;
		return (_Tmp);
	}

	_Myiter & operator--()
	{	// predecrement
		--m_idx;
		return (*this);
	}

	_Myiter operator--(int)
	{	// postdecrement
		_Myiter _Tmp = *this;
		--*this;
		return (_Tmp);
	}

	_Myiter & operator+=(difference_type _Off)
	{	// increment by integer
		m_idx += _Off;
		return (*this);
	}

	_Myiter operator+(difference_type _Off) const
	{	// return this + integer
		_Myiter _Tmp = *this;
		return (_Tmp += _Off);
	}

	_Myiter & operator-=(difference_type _Off)
	{	// decrement by integer
		return (*this += -_Off);
	}

	_Myiter operator-(difference_type _Off) const
	{	// return this - integer
		_Myiter _Tmp = *this;
		return (_Tmp -= _Off);
	}

	difference_type operator-(const _Myiter &_Right) const
	{	// return difference of iterators
		return (this->m_idx - _Right.m_idx);
	}

	reference operator[](difference_type _Off) const
	{	// subscript
		return (*(*this + _Off));
	}

	bool operator==(const _Myiter &_Right) const
	{	// test for iterator equality
		return (this->m_idx == _Right.m_idx);
	}

	bool operator!=(const _Myiter &_Right) const
	{	// test for iterator inequality
		return (!(*this == _Right));
	}

	bool operator<(const _Myiter &_Right) const
	{	// test if this < _Right
		return (this->m_idx < _Right.m_idx);
	}

	bool operator>(const _Myiter &_Right) const
	{	// test if this > _Right
		return (_Right < *this);
	}

	bool operator<=(const _Myiter &_Right) const
	{	// test if this <= _Right
		return (!(_Right < *this));
	}

	bool operator>=(const _Myiter &_Right) const
	{	// test if this >= _Right
		return (!(*this < _Right));
	}

private:
	const _MyQueue* m_host;
	size_type m_idx;
};

template<class _MyQueue>
class _pbq_iterator
	: public _pbq_const_iterator<_MyQueue>
{
public:
	typedef _pbq_iterator<_MyQueue> _Myiter;
	typedef _pbq_const_iterator<_MyQueue> _Mybase;
	typedef std::random_access_iterator_tag iterator_category;

	typedef typename _MyQueue::value_type value_type;
	typedef typename _MyQueue::difference_type difference_type;
	typedef typename _MyQueue::pointer pointer;
	typedef typename _MyQueue::reference reference;
	typedef typename _MyQueue::size_type size_type;

public:
	_pbq_iterator(
		_MyQueue* host = NULL, size_type idx = 0)
		: _Mybase(host, idx)
	{	// construct with null pointer
	}

	reference operator*() const
	{	// return designated object
		return ((reference)**(_Mybase*)this);
	}

	pointer operator->() const
	{	// return pointer to class object
		return (std::pointer_traits<pointer>::pointer_to(**this));
	}

	_Myiter & operator++()
	{	// preincrement
		++*(_Mybase*)this;
		return (*this);
	}

	_Myiter operator++(int)
	{	// postincrement
		_Myiter _Tmp = *this;
		++*this;
		return (_Tmp);
	}

	_Myiter & operator--()
	{	// predecrement
		--*(_Mybase*)this;
		return (*this);
	}

	_Myiter operator--(int)
	{	// postdecrement
		_Myiter _Tmp = *this;
		--*this;
		return (_Tmp);
	}

	_Myiter & operator+=(difference_type _Off)
	{	// increment by integer
		*(_Mybase*)this += _Off;
		return (*this);
	}

	_Myiter operator+(difference_type _Off) const
	{	// return this + integer
		_Myiter _Tmp = *this;
		return (_Tmp += _Off);
	}

	_Myiter & operator-=(difference_type _Off)
	{	// decrement by integer
		return (*this += -_Off);
	}

	_Myiter operator-(difference_type _Off) const
	{	// return this - integer
		_Myiter _Tmp = *this;
		return (_Tmp -= _Off);
	}

	difference_type operator-(const _Mybase &_Right) const
	{	// return difference of iterators
		return (*(_Mybase *)this - _Right);
	}

	reference operator[](difference_type _Off) const
	{	// subscript
		return (*(*this + _Off));
	}
};


template<class _E,
	size_t _BlockSize = 512,
	class _Alloc = std::allocator<_E> >
class popblock_queue
{
public:
	typedef popblock_queue<_E, _BlockSize, _Alloc> _Myt;

	typedef std::vector<_E, _Alloc> Block;

	typedef typename Block::value_type value_type;
	typedef typename Block::size_type size_type;
	typedef typename Block::difference_type difference_type;
	typedef typename Block::pointer pointer;
	typedef typename Block::const_pointer const_pointer;
	typedef typename Block::reference reference;
	typedef typename Block::const_reference const_reference;

	typedef typename _pbq_iterator<popblock_queue> iterator;
	typedef typename _pbq_const_iterator<popblock_queue> const_iterator;

	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

public:
	popblock_queue()
		: m_base(0)
		, m_top(0)
	{
	}
	popblock_queue(const _Myt &r)
		: m_list(r.m_list)
		, m_base(0)
		, m_top(0)
	{
		for (BlockList::const_iterator itor = m_list.begin();
			itor != m_list.end(); ++itor)
		{
			const Block &b = *itor;
			m_id2block[m_top++] = &b;
		}
	}
	_Myt & operator=(const _Myt &r)
	{
		if (this == &r)
			return *this;

		m_list = r.m_list;
		m_base = 0;
		m_top = 0;

		for (BlockList::const_iterator itor = m_list.begin();
			itor != m_list.end(); ++itor)
		{
			const Block &b = *itor;
			m_id2block[m_top++] = &b;
		}

		return *this;
	}
	~popblock_queue()
	{
	}

	bool empty() const throw()
	{
		return m_list.empty();
	}

	size_t block_count() const throw()
	{
		return m_list.size();
	}

	size_type size() const throw()
	{
		switch (size_t s = m_list.size())
		{
		case 0:
			return 0;
		default:
			return (s - 1) * _BlockSize + m_list.back().size();
		}
	}

	const Block & front() const
	{
		return m_list.front();
	}

	const_reference elem(size_type idx) const
	{
		const size_t QUICK_POSITIONING = 4;

		size_t bidx = (size_t)idx / _BlockSize;
		size_type boff = idx % _BlockSize;

		size_t bc = m_list.size();

		if (bidx < QUICK_POSITIONING)
		{
			BlockList::const_iterator itor = m_list.begin();
			while (bidx > 0)
			{
				++itor;
				--bidx;
			}
			return (*itor)[boff];
		}
		else if (bidx + QUICK_POSITIONING >= bc)
		{
			BlockList::const_iterator itor = m_list.end();
			while (bidx < bc)
			{
				--itor;
				++bidx;
			}
			return (*itor)[boff];
		}
		else
		{
			size_t bid = bidx + m_base;
			Id2Block::const_iterator itor = m_id2block.find(bid);
			if (itor != m_id2block.end())
			{
				return (*(itor->second))[boff];
			}
			throw INVALID_INDEX();
		}
	}

	void push(const _E &e)
	{
		__try_expand(m_list);

		Block &b = m_list.back();
		b.push_back(e);
	}

	// Iterators, pointers and references are invalidated
	void pop()
	{
		m_list.pop_front();
		m_id2block.erase(m_base++);
	}

	// Iterators, pointers and references are invalidated
	void popto(Block &b)
	{
		b.clear();
		b.swap(m_list.front());
		m_list.pop_front();
		m_id2block.erase(m_base++);
	}

	const_iterator begin() const throw()
	{
		return const_iterator(this, 0);
	}
	iterator begin() throw()
	{
		return iterator(this, 0);
	}
	const_iterator end() const throw()
	{
		return const_iterator(this, size());
	}
	iterator end() throw()
	{
		return iterator(this, size());
	}
	const_reverse_iterator rbegin() const throw()
	{
		return const_reverse_iterator(end());
	}
	reverse_iterator rbegin() throw()
	{
		return reverse_iterator(end());
	}
	const_reverse_iterator rend() const throw()
	{
		return const_reverse_iterator(begin());
	}
	reverse_iterator rend() throw()
	{
		return reverse_iterator(begin());
	}

private:
	typedef typename _Alloc::template rebind<
		Block>::other _BlockAlloc;
	typedef std::list<Block, _BlockAlloc> BlockList;
	BlockList m_list;

	size_t m_base;
	size_t m_top;

	typedef typename _Alloc::template rebind<
		std::pair<const size_t, const Block*> >::other _IBPairAlloc;
	typedef std::map<size_t, const Block*,
		std::less<size_t>, _IBPairAlloc> Id2Block;
	Id2Block m_id2block;

	void __try_expand(BlockList &l)
	{
		if (l.empty())
		{
			__expand(l);
			return;
		}

		Block &b = l.back();
		if (b.size() >= _BlockSize)
		{
			__expand(l);
			return;
		}
	}

	void __expand(BlockList &l)
	{
		l.resize(l.size() + 1);
		Block &b = m_list.back();
		b.reserve(_BlockSize);
		m_id2block[m_top++] = &b;
	}
};


} // namespace jtwsm

#endif /* __FIXED_QUEUE_H__ */
