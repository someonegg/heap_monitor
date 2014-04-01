/*
 *  File   : analyzer/analyzer.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/22 14:02:03
 *  Brief  : TSS_* class must be copyable
 *
 *  $Id: $
 */
#ifndef __ANALYZER_ANALYZER_H__
#define __ANALYZER_ANALYZER_H__

#include "track_system/snapshot.h"


class TSSAnalyzer
{
public:
	TSSAnalyzer();
	~TSSAnalyzer();

	void UpdateSnapshot();

	bool SetTargetProcess(tst_pid pid);
	tst_pid TargetProcess() const;

	bool Command(int argc, wchar_t** argv);

protected:
	void showSnapshotInfo();
	void showSortInfo();

	void showProcessList(
		const TSS_System::ProcessS &pl, bool fExt = false);
	void showStackList(
		const TSS_Process::StackS &sl, bool fExt = false);
	void showImageList(
		const TSS_Process::ImageS &il, bool fExt = false);
	void showHeapList(
		const TSS_Process::HeapS &hl, bool fExt = false);
	void showThreadList(
		const TSS_Process::ThreadS &tl, bool fExt = false);

protected:
	enum
	{
		ST_CountTotal = 0, ST_CountCurrent, ST_CountPeak,
		ST_BytesTotal, ST_BytesCurrent, ST_BytesPeak
	};

	template <int st>
	struct CompT {};

	template <>
	struct CompT<ST_CountTotal>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &acl = l->StatBy<AllocCountIdx>();
			const AllocCount &acr = r->StatBy<AllocCountIdx>();
			return acl.total > acr.total;
		}
	};
	template <>
	struct CompT<ST_CountCurrent>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &acl = l->StatBy<AllocCountIdx>();
			const AllocCount &acr = r->StatBy<AllocCountIdx>();
			return acl.current > acr.current;
		}
	};
	template <>
	struct CompT<ST_CountPeak>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &acl = l->StatBy<AllocCountIdx>();
			const AllocCount &acr = r->StatBy<AllocCountIdx>();
			return acl.peak > acr.peak;
		}
	};
	template <>
	struct CompT<ST_BytesTotal>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &abl = l->StatBy<AllocBytesIdx>();
			const AllocCount &abr = r->StatBy<AllocBytesIdx>();
			return abl.total > abr.total;
		}
	};
	template <>
	struct CompT<ST_BytesCurrent>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &abl = l->StatBy<AllocBytesIdx>();
			const AllocCount &abr = r->StatBy<AllocBytesIdx>();
			return abl.current > abr.current;
		}
	};
	template <>
	struct CompT<ST_BytesPeak>
	{
		template <class T>
		bool operator() (const T* l, const T* r) const
		{
			const AllocCount &abl = l->StatBy<AllocBytesIdx>();
			const AllocCount &abr = r->StatBy<AllocBytesIdx>();
			return abl.peak > abr.peak;
		}
	};

	void parseSortArgs(
		int argc, wchar_t** argv, size_t &top, int &st);

	template <class S, class CPS>
	void sortSetByLimit(const S &s, CPS &out, size_t top, int st);

	template <size_t Base, class S>
	void calcIDWidth(const S &s, size_t &wthID);

	void calcPnameWidth(
		const TSS_System::ProcessS &pl, size_t &wthPname);

	template <class S>
	void calcAllocStatWidth(const S &s, size_t wthAllocStat[6]);
	template <class T>
	void printAllocStat(
		const T &t, size_t wthAllocStat[6], const char* delimiter);

	void printIRA(const IRA_Sym &ira_sym);
	void printIRAS(
		const IRA_SymS &ira_syms,
		const char* prefix = "  ",
		const char* postfix = "\n"
		);

protected:
	TSS_System m_sys;

	size_t m_limitTop;
	int m_sortType;

	// format
	size_t m_wthPid;
	size_t m_wthPname;
	size_t m_wthAllocStat[6];

	tst_pid m_pid;
	const TSS_Process* m_process;

	// format
	size_t m_wthStackid;
	size_t m_wthImgid;
	size_t m_wthHeapid;
	size_t m_wthTid;
};


#endif /* __ANALYZER_ANALYZER_H__ */
