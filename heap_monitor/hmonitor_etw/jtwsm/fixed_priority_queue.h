#ifndef __FIXED_PRIORITY_QUEUE_H__
#define __FIXED_PRIORITY_QUEUE_H__

#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>

//------------------------------------------------------------------------------

template<class T, class ContainerT = std::vector<T>, class CompT = std::greater<T> >
class FixedPriorityQueue
{
	typedef typename ContainerT::value_type value_type;
	typedef typename ContainerT::size_type size_type;

	size_type m_capacity;
	ContainerT &m_cont;
	CompT m_comp;

public:
	explicit FixedPriorityQueue(size_type capacity, ContainerT &cont)
		: m_capacity(capacity), m_cont(cont), m_comp()
	{
		assert(m_capacity);
		m_cont.reserve(m_capacity);
	}

	bool empty() const
	{
		return (m_cont.empty());
	}

	size_type size() const
	{
		return (m_cont.size());
	}

	void push(const value_type& v)
	{
		if (m_cont.size() < m_capacity) {
			// 队列未满，插入到最后并调整堆即可
			m_cont.push_back(v);
			std::push_heap(m_cont.begin(), m_cont.end(), m_comp);
		} else {
			// 队列已满
			// m_cont.front()是堆的根节点，也就是当前的最小值，将新数据与它进行比较，若更小则直接丢弃
			if (!m_comp(m_cont.front(), v)) {
				// 弹出当前的最小值，注意此时容器的size会维持不变，而被弹出的值放到了容器的最后位置上
				std::pop_heap(m_cont.begin(), m_cont.end(), m_comp);
				// 插入新值，调整堆
				m_cont.back() = v;
				std::push_heap(m_cont.begin(), m_cont.end(), m_comp);
			}
		}
	}

	void sort()
	{
		// 将堆还原为排序好的数据序列
		if (m_cont.size()) {
			std::sort_heap(m_cont.begin(), m_cont.end(), m_comp);
		}
	}
};

//------------------------------------------------------------------------------

#endif // !__FIXED_PRIORITY_QUEUE_H__
