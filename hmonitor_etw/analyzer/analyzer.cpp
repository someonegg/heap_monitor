#include "stdafx.h"
#include "analyzer.h"
#include "symbol.h"
#include "track_system/track_system.h"

#include "jtwsm/fixed_priority_queue.h"
#include "utility/qpc_helper.h"
#include <math.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


const char* TMPL_NORMALCONDITION =
	"function NormalCondition(CountTotal, CountCurrent, CountPeak, BytesTotal, BytesCurrent, BytesPeak)\n"
	"\treturn (%s)\n"
	"end\n";

const char* TMPL_STACKCONDITION =
	"function StackCondition(CountTotal, CountCurrent, CountPeak, BytesTotal, BytesCurrent, BytesPeak, ImgId, HeapId)\n"
	"\treturn (%s)\n"
	"end\n";

int myPrintf(const char* format, ...);


TSSAnalyzer::TSSAnalyzer()
	: m_limitTop(5)
	, m_sortType(ST_BytesCurrent)
	, m_hasCondition(false)
	, m_wthPid()
	, m_wthPname()
	, m_wthAllocStat()
	, m_pid()
	, m_process(NULL)
	, m_wthStackid()
	, m_wthImgid()
	, m_wthHeapid()
	, m_wthTid()
	, m_L()
{
}

TSSAnalyzer::~TSSAnalyzer()
{
}


bool TSSAnalyzer::Init()
{
	m_L = luaL_newstate();
	if (m_L == NULL)
	{
		return false;
	}

	luaL_openlibs(m_L);

	return true;
}

void TSSAnalyzer::Term()
{
	if (m_L != NULL)
	{
		lua_close(m_L);
		m_L = NULL;
	}
}

void TSSAnalyzer::UpdateSnapshot()
{
	getTrackSystem()->Snapshot(m_sys);
	calcIDWidth<10>(m_sys.processes, m_wthPid);
	calcPnameWidth(m_sys.processes, m_wthPname);
	calcAllocStatWidth(m_sys.processes, m_wthAllocStat);

	tst_pid pid = m_pid;
	m_pid = 0;
	m_process = NULL;

	// default
	if (pid == 0 && !m_sys.processes.empty())
		pid = m_sys.processes[0].id;

	if (pid != 0)
		SetTargetProcess(pid);
}

bool TSSAnalyzer::SetTargetProcess(tst_pid pid)
{
	TSS_System::ProcessS::const_iterator itor =
		binary_find(m_sys.processes, pid);
	if (itor != m_sys.processes.end())
	{
		m_pid = pid;
		m_process = &(*itor);

		m_wthStackid = 16;
		calcIDWidth<16>(m_process->images, m_wthImgid);
		calcIDWidth<16>(m_process->heaps, m_wthHeapid);
		calcIDWidth<10>(m_process->threads, m_wthTid);
		return true;
	}
	return false;
}

tst_pid TSSAnalyzer::TargetProcess() const
{
	return m_pid;
}

bool TSSAnalyzer::Command(int argc, wchar_t** argv)
{
	if (_wcsicmp(argv[0], L"processlist") == 0 ||
		_wcsicmp(argv[0], L"pl") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showProcessList(m_sys.processes);
		return true;
	}
	if (_wcsicmp(argv[0], L"processlistext") == 0 ||
		_wcsicmp(argv[0], L"ple") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showProcessList(m_sys.processes, true);
		return true;
	}

	if (m_process == NULL)
	{
		myPrintf("Unknown process\n");
		return true;
	}

	if (_wcsicmp(argv[0], L"stacklist") == 0 ||
		_wcsicmp(argv[0], L"sl") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_STACKCONDITION);
		showStackList(m_process->stacks);
		return true;
	}
	if (_wcsicmp(argv[0], L"stacklistext") == 0 ||
		_wcsicmp(argv[0], L"sle") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_STACKCONDITION);
		showStackList(m_process->stacks, true);
		return true;
	}
	if (_wcsicmp(argv[0], L"imagelist") == 0 ||
		_wcsicmp(argv[0], L"il") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showImageList(m_process->images);
		return true;
	}
	if (_wcsicmp(argv[0], L"imagelistext") == 0 ||
		_wcsicmp(argv[0], L"ile") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showImageList(m_process->images, true);
		return true;
	}
	if (_wcsicmp(argv[0], L"heaplist") == 0 ||
		_wcsicmp(argv[0], L"hl") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showHeapList(m_process->heaps);
		return true;
	}
	if (_wcsicmp(argv[0], L"heaplistext") == 0 ||
		_wcsicmp(argv[0], L"hle") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showHeapList(m_process->heaps, true);
		return true;
	}
	if (_wcsicmp(argv[0], L"threadlist") == 0 ||
		_wcsicmp(argv[0], L"tl") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showThreadList(m_process->threads);
		return true;
	}
	if (_wcsicmp(argv[0], L"threadlistext") == 0 ||
		_wcsicmp(argv[0], L"tle") == 0)
	{
		parseSortArgs(argc - 1, argv + 1,
			m_limitTop, m_sortType, TMPL_NORMALCONDITION);
		showThreadList(m_process->threads, true);
		return true;
	}

	return false;
}


void TSSAnalyzer::showSnapshotInfo()
{
	myPrintf("[Snapshot, %d seconds ago]\n",
		(int)g_QPCHelper.GetTimeSpentMS(m_sys.tsLast) / 1000);
}

void TSSAnalyzer::showSortInfo(size_t total)
{
	const char* szST[] =
	{
		"CountTotal", "CountCurrent", "CountPeak",
		"BytesTotal", "BytesCurrent", "BytesPeak"
	};
	myPrintf("[");
	myPrintf("Show top %u, sort by %s", total, szST[m_sortType]);
	if (m_hasCondition)
		myPrintf(", use condition");
	myPrintf("]\n");
}

void TSSAnalyzer::showProcessList(
	const TSS_System::ProcessS &pl, bool fExt)
{
	showSnapshotInfo();

	if (pl.empty())
		return;

	std::vector<const TSS_Process*> ss;
	sortSetByLimit(pl, ss, m_limitTop, m_sortType);

	showSortInfo(ss.size());
	myPrintf("\n");

	for (size_t i = 0; i < ss.size(); ++i)
	{
		const TSS_Process &p = *(ss[i]);

		myPrintf("%-*u  %*s  ",
			m_wthPid, p.id, m_wthPname, p.name.c_str());
		printAllocStat(p, m_wthAllocStat, "  ");
		myPrintf("\n");
		myPrintf("\n");
	}
}

void TSSAnalyzer::showStackList(
	const TSS_Process::StackS &sl, bool fExt)
{
	showSnapshotInfo();

	if (sl.empty())
		return;

	std::vector<const TSS_Stack*> ss;
	sortSetByLimit(sl, ss, m_limitTop, m_sortType);

	showSortInfo(ss.size());
	myPrintf("\n");

	for (size_t i = 0; i < ss.size(); ++i)
	{
		const TSS_Stack &s = *(ss[i]);

		myPrintf("0x%-*llx  ", m_wthStackid, s.id);
		printAllocStat(s, m_wthAllocStat, "  ");
		myPrintf("\n");

		if (fExt)
		{
			myPrintf("Stack info:\n");
			printIRAS(*(s.ira_syms));
		}

		myPrintf("\n");
	}
}

void TSSAnalyzer::showImageList(
	const TSS_Process::ImageS &il, bool fExt)
{
	showSnapshotInfo();

	if (il.empty())
		return;

	std::vector<const TSS_Image*> ss;
	sortSetByLimit(il, ss, m_limitTop, m_sortType);

	showSortInfo(ss.size());
	myPrintf("\n");

	for (size_t i = 0; i < ss.size(); ++i)
	{
		const TSS_Image &img = *(ss[i]);

		myPrintf("%s\n", m_sys.nimImg.name(img.index));

		myPrintf("0x%-*llx  ", m_wthImgid, img.id);
		printAllocStat(img, m_wthAllocStat, "  ");
		myPrintf("\n");

		if (fExt)
		{
			myPrintf("ImageLoad Stack info:\n");
			printIRAS(*(img.stackLoad));
		}

		myPrintf("\n");
	}
}

void TSSAnalyzer::showHeapList(
	const TSS_Process::HeapS &hl, bool fExt)
{
	showSnapshotInfo();

	if (hl.empty())
		return;

	std::vector<const TSS_Heap*> ss;
	sortSetByLimit(hl, ss, m_limitTop, m_sortType);

	showSortInfo(ss.size());
	myPrintf("\n");

	for (size_t i = 0; i < ss.size(); ++i)
	{
		const TSS_Heap &h = *(ss[i]);

		myPrintf("0x%-*llx  ", m_wthHeapid, h.id);
		printAllocStat(h, m_wthAllocStat, "  ");
		myPrintf("\n");

		{
			myPrintf("Heap Space info:\n");

			tst_ptdiffer current = h.StatBy<AllocBytesIdx>().current;
			tst_ptdiffer committed = h.committed; // max(h.committed, h.ss_committed);
			double percent = (double)current / (double)committed;
			myPrintf("  [%.3f, %*llu, %*llu] [%u]",
				percent,
				m_wthAllocStat[4] + 0, current,
				m_wthAllocStat[4] + 1, committed,
				(unsigned)h.ranges.size());
			myPrintf("\n");
		}

		if (fExt)
		{
			myPrintf("HeapCreate Stack info:\n");
			printIRAS(*(h.stackCreate));
		}

		myPrintf("\n");
	}
}

void TSSAnalyzer::showThreadList(
	const TSS_Process::ThreadS &tl, bool fExt)
{
	showSnapshotInfo();

	if (tl.empty())
		return;

	std::vector<const TSS_Thread*> ss;
	sortSetByLimit(tl, ss, m_limitTop, m_sortType);

	showSortInfo(ss.size());
	myPrintf("\n");

	for (size_t i = 0; i < ss.size(); ++i)
	{
		const TSS_Thread &t = *(ss[i]);

		myPrintf("%-*u  ", m_wthTid, t.id);
		printAllocStat(t, m_wthAllocStat, "  ");
		myPrintf("\n");

		printIRA(t.entry);
		myPrintf("\n");

		if (fExt)
		{
			myPrintf("CreateThread Stack info:\n");
			printIRAS(*(t.stackStart));
		}

		myPrintf("\n");
	}
}


void TSSAnalyzer::parseSortArgs(
	int argc, wchar_t** argv,
	size_t &top, int &st, const char* scriptTmpl)
{
	m_hasCondition = false;
	if (argc >= 1)
	{
		if(_wcsnicmp(argv[0], L"C:", 2) == 0)
		{
			char param[512];
			size_t n = wcstombs(param, argv[0] + 2, sizeof(param));
			if ((n > 0 && n < sizeof(param)) &&
				renderScript(scriptTmpl, param))
			{
				m_hasCondition = true;
			}
			--argc;
			++argv;
		}
	}

	if (argc >= 1)
	{
		int tmp = _wtoi(argv[0]);
		if (tmp > 0)
			top = tmp;
	}
	if (argc >= 2)
	{
		if (_wcsicmp(argv[1], L"CountTotal") == 0)
		{
			st = ST_CountTotal;
		}
		else if(_wcsicmp(argv[1], L"CountCurrent") == 0)
		{
			st = ST_CountCurrent;
		}
		else if(_wcsicmp(argv[1], L"CountPeak") == 0)
		{
			st = ST_CountPeak;
		}
		else if(_wcsicmp(argv[1], L"BytesTotal") == 0)
		{
			st = ST_BytesTotal;
		}
		else if(_wcsicmp(argv[1], L"BytesCurrent") == 0)
		{
			st = ST_BytesCurrent;
		}
		else if(_wcsicmp(argv[1], L"BytesPeak") == 0)
		{
			st = ST_BytesPeak;
		}
	}
}

template <class S, class SQ>
void TSSAnalyzer::sortUseQueue(const S &s, SQ &sq)
{
	for (S::const_iterator itor = s.begin(); itor != s.end(); ++itor)
	{
		if (m_hasCondition)
		{
			if (fitCondition(*itor))
			{
				sq.push(&(*itor));
			}
			continue;
		}
		sq.push(&(*itor));
	}
	sq.sort();
}

template <class S, class CPS>
void TSSAnalyzer::sortSetByLimit(const S &s, CPS &out, size_t top, int st)
{
	top = min(top, s.size());

	typedef typename CPS::value_type CP;
	switch (st)
	{
	case ST_CountTotal:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_CountTotal> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	case ST_CountCurrent:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_CountCurrent> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	case ST_CountPeak:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_CountPeak> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	case ST_BytesTotal:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_BytesTotal> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	case ST_BytesCurrent:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_BytesCurrent> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	case ST_BytesPeak:
	{
		FixedPriorityQueue<CP, CPS, CompT<ST_BytesPeak> > topQueue(top, out);
		sortUseQueue(s, topQueue);
		break;
	}
	default:
		ASSERT (0);
	}
}

template <size_t Base, class S>
void TSSAnalyzer::calcIDWidth(const S &s, size_t &wthID)
{
	typedef typename S::value_type::ID ID;
	ID idMax = ID();
	for (S::const_iterator itor = s.begin();
		itor != s.end(); ++itor)
	{
		idMax = max(idMax, itor->id);
	}
	wthID = (size_t)(log(idMax) / log(Base)) + 1;
}

void TSSAnalyzer::calcPnameWidth(
	const TSS_System::ProcessS &pl, size_t &wthPname)
{
	wthPname = 0;
	for (TSS_System::ProcessS::const_iterator itor = pl.begin();
		itor != pl.end(); ++itor)
	{
		wthPname = max(wthPname, itor->name.length());
	}
}

template <class S>
void TSSAnalyzer::calcAllocStatWidth(const S &s, size_t wthAllocStat[6])
{
	AllocCount acMax;
	AllocBytes abMax;
	for (S::const_iterator itor = s.begin();
		itor != s.end(); ++itor)
	{
		const AllocCount &ac = (*itor).StatBy<AllocCountIdx>();
		acMax.total = max(acMax.total, ac.total);
		acMax.current = max(acMax.current, ac.current);
		acMax.peak = max(acMax.peak, ac.peak);
		const AllocBytes &ab = (*itor).StatBy<AllocBytesIdx>();
		abMax.total = max(abMax.total, ab.total);
		abMax.current = max(abMax.current, ab.current);
		abMax.peak = max(abMax.peak, ab.peak);
	}
	wthAllocStat[0] = (size_t)log10(acMax.total) + 1;
	wthAllocStat[1] = (size_t)log10(acMax.current) + 1;
	wthAllocStat[2] = (size_t)log10(acMax.peak) + 1;
	wthAllocStat[3] = (size_t)log10(abMax.total) + 1;
	wthAllocStat[4] = (size_t)log10(abMax.current) + 1;
	wthAllocStat[5] = (size_t)log10(abMax.peak) + 1;
}

template <class T>
void TSSAnalyzer::printAllocStat(
	const T &t, size_t wthAllocStat[6], const char* delimiter)
{
	const AllocCount &ac = t.StatBy<AllocCountIdx>();
	const AllocBytes &ab = t.StatBy<AllocBytesIdx>();
	myPrintf(
		"(%*llu, %*llu, %*llu)%s(%*llu, %*llu, %*llu)",
		wthAllocStat[0], ac.total,
		wthAllocStat[1], ac.current,
		wthAllocStat[2], ac.peak,
		delimiter,
		wthAllocStat[3], ab.total,
		wthAllocStat[4], ab.current,
		wthAllocStat[5], ab.peak
		);
}

void TSSAnalyzer::printIRA(const IRA_Sym &ira_sym)
{
	if (!ira_sym.sym.empty())
	{
		myPrintf("%s", ira_sym.sym.c_str());
		return;
	}

	IRA ira = ira_sym.ira;
	if (!ira.valid && m_process != NULL)
	{
		ira = m_process->addr2IRA(ira.u64);
	}

	if (ira.valid)
	{
		const char* name = m_sys.nimImg.name(ira.index);
		symLoadModule(name);
		char funInfo[512];
		getFuncInfo(name, ira.offset, funInfo);
		ira_sym.sym = funInfo;
	}
	else
	{
		char funInfo[512];
		sprintf(funInfo, "0x%llx", ira.u64);
		ira_sym.sym = funInfo;
	}

	myPrintf("%s", ira_sym.sym.c_str());
	return;
}

void TSSAnalyzer::printIRAS(
	const IRA_SymS &ira_syms,
	const char* prefix,
	const char* postfix
	)
{
	for (IRA_SymS::const_iterator itor = ira_syms.begin();
		itor != ira_syms.end(); ++itor)
	{
		myPrintf("%s", prefix);
		printIRA(*itor);
		myPrintf("%s", postfix);
	}
}


bool TSSAnalyzer::renderScript(const char* scriptTmpl, const char* param)
{
	char script[1024];
	size_t n = _snprintf(script, sizeof(script), scriptTmpl, param);
	if (n > 0 && n < sizeof(script))
	{
		return (luaL_dostring(m_L, script) == 0);
	}

	return false;
}

class LuaStackBalancer
{
	lua_State* m_L;
	int m_top;
public:
	LuaStackBalancer(lua_State* L)
	{
		m_L = L;
		m_top = lua_gettop(m_L);
	}
	~LuaStackBalancer()
	{
		lua_settop(m_L, m_top);
	}
};

template <class T>
bool TSSAnalyzer::fitCondition(const T &t)
{
	const AllocCount &ac = t.StatBy<AllocCountIdx>();
	const AllocBytes &ab = t.StatBy<AllocBytesIdx>();

	LuaStackBalancer _balancer(m_L);

	lua_getglobal(m_L, "NormalCondition");
	if (!lua_isfunction(m_L, -1))
	{
		return true;
	}

	lua_pushnumber(m_L, (lua_Number)ac.total);
	lua_pushnumber(m_L, (lua_Number)ac.current);
	lua_pushnumber(m_L, (lua_Number)ac.peak);
	lua_pushnumber(m_L, (lua_Number)ab.total);
	lua_pushnumber(m_L, (lua_Number)ab.current);
	lua_pushnumber(m_L, (lua_Number)ab.peak);

	if (lua_pcall(m_L, 6, 1, 0) != 0)
	{
		return true;
	}

	if (lua_isboolean(m_L, -1))
	{
		return (lua_toboolean(m_L, -1) != 0);
	}

	return true;
}

bool TSSAnalyzer::fitCondition(const TSS_Stack &t)
{
	const AllocCount &ac = t.StatBy<AllocCountIdx>();
	const AllocBytes &ab = t.StatBy<AllocBytesIdx>();

	LuaStackBalancer _balancer(m_L);

	lua_getglobal(m_L, "StackCondition");
	if (!lua_isfunction(m_L, -1))
	{
		return true;
	}

	lua_pushnumber(m_L, (lua_Number)ac.total);
	lua_pushnumber(m_L, (lua_Number)ac.current);
	lua_pushnumber(m_L, (lua_Number)ac.peak);
	lua_pushnumber(m_L, (lua_Number)ab.total);
	lua_pushnumber(m_L, (lua_Number)ab.current);
	lua_pushnumber(m_L, (lua_Number)ab.peak);
	lua_pushnumber(m_L, (lua_Number)t.imgid);
	lua_pushnumber(m_L, (lua_Number)t.heapid);

	if (lua_pcall(m_L, 8, 1, 0) != 0)
	{
		return true;
	}

	if (lua_isboolean(m_L, -1))
	{
		return (lua_toboolean(m_L, -1) != 0);
	}

	return true;
}
