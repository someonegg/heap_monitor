/*
 *  File   : track_system/snapshot.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/22 14:02:03
 *  Brief  : TSS_* class must be copyable
 *
 *  $Id: $
 */
#ifndef __TRACK_SYSTEM_SNAPSHOT_H__
#define __TRACK_SYSTEM_SNAPSHOT_H__

#include "config.h"
#include "statistics.h"
#include <vector>
#include <string>
#include "jtwsm/nameidx_map.h"


#define TSS_OPERATOR_LESS \
	bool operator<(ID id) const \
	{ \
		return this->id < id; \
	} \
	bool operator<(const MyT &r) const \
	{ \
		return this->id < r.id; \
	}
#define TSS_OPERATOR_EQUAL \
	bool operator==(ID id) const \
	{ \
		return this->id == id; \
	} \
	bool operator==(const MyT &r) const \
	{ \
		return this->id == r.id; \
	}


struct IRA_Sym
{
	IRA ira;
	mutable std::string sym;
};

typedef std::vector<IRA_Sym> IRA_SymS;

class TSS_Stack : public StatBase<StackStat>
{
public:
	typedef TSS_Stack MyT;
	typedef tst_stackid ID;

	TSS_Stack()
		: id()
		, ira_syms()
	{
	}

	TSS_OPERATOR_LESS
	TSS_OPERATOR_EQUAL

	ID id;
	const IRA_SymS* ira_syms;
};

class TSS_Image : public StatBase<ImageStat>
{
public:
	typedef TSS_Image MyT;
	typedef tst_imgid ID;

	TSS_Image()
		: tsLoad()
		, tsUnload()
		, id()
		, size()
		, index()
		, stackLoad()
	{
	}

	TSS_OPERATOR_LESS
	TSS_OPERATOR_EQUAL

	tst_time tsLoad;
	tst_time tsUnload;
	ID id;
	tst_imgsz size;
	size_t index;
	const IRA_SymS* stackLoad;
};

class TSS_Heap : public StatBase<HeapStat>
{
public:
	typedef TSS_Heap MyT;
	typedef tst_heapid ID;

	TSS_Heap()
		: tsCreate()
		, tsDestroy()
		, id()
		, stackCreate()
	{
	}

	TSS_OPERATOR_LESS
	TSS_OPERATOR_EQUAL

	tst_time tsCreate;
	tst_time tsDestroy;
	ID id;
	const IRA_SymS* stackCreate;
};

class TSS_Thread : public StatBase<ThreadStat>
{
public:
	typedef TSS_Thread MyT;
	typedef tst_tid ID;

	TSS_Thread()
		: tsStart()
		, tsEnd()
		, id()
		, userStack()
		, entry()
		, stackStart()
	{
	}

	TSS_OPERATOR_LESS
	TSS_OPERATOR_EQUAL

	tst_time tsStart;
	tst_time tsEnd;
	ID id;
	tst_pointer userStack;
	IRA_Sym entry;
	const IRA_SymS* stackStart;
};

class TSS_Process : public StatBase<ProcessStat>
{
public:
	typedef TSS_Process MyT;
	typedef tst_pid ID;

	TSS_Process()
		: tsStart()
		, tsEnd()
		, id()
		, ppid()
		, code()
	{
	}

	TSS_OPERATOR_LESS
	TSS_OPERATOR_EQUAL

	tst_time tsStart;
	tst_time tsEnd;
	ID id;
	tst_pid ppid;
	tst_exit code;
	std::string name;

	// sort by stackid, ascending order
	typedef std::vector<TSS_Stack> StackS;
	StackS stacks;

	// sort by base, ascending order
	typedef std::vector<TSS_Image> ImageS;
	ImageS images;

	// sort by heapid, ascending order
	typedef std::vector<TSS_Heap> HeapS;
	HeapS heaps;

	// sort by tid, ascending order
	typedef std::vector<TSS_Thread> ThreadS;
	ThreadS threads;

public:
	IRA addr2IRA(tst_pointer addr) const
	{
		ImageS::const_iterator itor =
			std::lower_bound(images.begin(), images.end(), addr);

		// Equal
		if (itor != images.end())
		{
			const TSS_Image &image = *itor;
			if (image == addr)
			{
				IRA ira;
				ira.index = (uint32_t)image.index;
				ira.offset = 0;
				ira.valid = 1;
				return ira;
			}
		}

		if (itor != images.begin())
		{
			--itor;
			const TSS_Image &image = *itor;
			ASSERT (image.id < addr);
			if (image.id + image.size > addr)
			{
				IRA ira;
				ira.index = (uint32_t)image.index;
				ira.offset = (uint32_t)(addr - image.id);
				ira.valid = 1;
				return ira;
			}
		}

		IRA ira;
		ira.u64 = addr;
		ira.valid = 0;
		return ira;
	}
};

class TSS_System
{
public:
	TSS_System()
		: tsLast()
	{
	}

	// sort by pid, ascending order
	typedef std::vector<TSS_Process> ProcessS;
	ProcessS processes;

	// all image list
	// image index is based on this list
	jtwsm::nameidx_map<char> nimImg;

	int64_t tsLast;
};


// helper

template <class _S, typename _K>
typename _S::const_iterator
binary_find(const _S &s, const _K &k)
{
	typename _S::const_iterator itor =
		std::lower_bound(s.begin(), s.end(), k);

	// Equal
	if (itor != s.end() && *itor == k)
	{
		return itor;
	}

	return s.end();
}


#endif /* __TRACK_SYSTEM_SNAPSHOT_H__ */
