/*
 *  File   : n_d_traver.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/14 09:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __N_D_TRAVER_H__
#define __N_D_TRAVER_H__

namespace jtwsm {


//
// _Ci = [_E, ...]
// _Co = [_Ci, ...]
//
template <class _E, class _Ci, class _Co>
class two_d_ctraver
{
public:
	two_d_ctraver(const _Co* c = NULL)
		: m_c(c)
	{
		if (m_c != NULL)
		{
			m_iCo = m_c->begin();
			if (m_iCo != m_c->end())
				m_iCi = m_iCo->begin();
			ajust();
		}
	}

	bool end()
	{
		return m_iCo == m_c->end();
	}

	const _E & elem()
	{
		return *m_iCi;
	}

	void next()
	{
		++m_iCi;
		ajust();
	}

private:
	void ajust()
	{
		while (m_iCo != m_c->end())
		{
			if (m_iCi != m_iCo->end())
				break;

			++m_iCo;

			if (m_iCo != m_c->end())
				m_iCi = m_iCo->begin();
		}
	}

	const _Co* m_c;
	typename _Ci::const_iterator m_iCi;
	typename _Co::const_iterator m_iCo;
};

template <class _E, class _Ci, class _Co>
class two_d_traver : public two_d_ctraver<_E, _Ci, _Co>
{
	typedef two_d_ctraver<_E, _Ci, _Co> _Mybase;

public:
	two_d_traver(_Co* c = NULL)
		: _Mybase(c)
	{
	}

	_E & elem()
	{
		return (_E &)((_Mybase*)this)->elem();
	}
};


} // namespace jtwsm

#endif /* __N_D_TRAVER_H__ */
