// hmonitor_etw.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "etw_event.h"
#include "etw_session.h"
#include "track_system/track_system.h"
#include "analyzer/symbol.h"
#include "analyzer/analyzer.h"
#include <fstream>

#include "jtwsm/thread_base.h"
#include "utility/qpc_helper.h"
CQPCHelper g_QPCHelper;
#include "utility/lddn_helper.h"
CLDDNHelper g_lddnHelper;


// KERNEL_LOGGER_NAMEA
// KERNEL_LOGGER_NAMEW
#define HEAP_LOGGER_NAMEA   "Heap Logger"
#define HEAP_LOGGER_NAMEW  L"Heap Logger"

#define SESSION_FLUSH_TIME_MS 1000
#define SESSION_CACHE_TIME_MS 3000


class EventFilter_NewProcess : public IEventFilter
{
public:
	bool filter(PEVENT_RECORD rawEvt)
	{
		return false;
	}
};

class HeapMonitor : public jtwsm::ThreadBase<HeapMonitor>
{
	typedef jtwsm::ThreadBase<HeapMonitor> _ThreadBase;

public:
	HeapMonitor()
		: m_flusher(this)
		, m_lastTS(0)
		, m_processTimes(0)
		, m_processed(0)
		, m_spendTime(0)
	{
	}
	~HeapMonitor()
	{
	}

public:
	bool Start()
	{
		stopTrace(); // stop old
		startTrace();

		if (!m_kernelSession.Init(true, KERNEL_LOGGER_NAMEW) ||
			!m_heapSession.Init(true, HEAP_LOGGER_NAMEW))
		{
			stopTrace();
			return false;
		}

		m_kernelSession.Start();
		m_heapSession.Start();

		m_flusher.Start();

		_ThreadBase::Start();
		return true;
	}

	void Stop()
	{
		_ThreadBase::SetNeedExit();
		m_flusher.SetNeedExit();

		stopTrace();

		m_kernelSession.Stop();
		m_heapSession.Stop();

		_ThreadBase::Stop();
		m_flusher.Stop();
	}

	int64_t LastTS() const { return m_lastTS; }

	struct STATUS
	{
		ETWSession::STATUS ks;
		ETWSession::STATUS hs;
		uint64_t processTimes;
		uint64_t processed;
		uint64_t spendTime;
	};

	void Status(STATUS &status) const
	{
		m_kernelSession.Status(status.ks);
		m_heapSession.Status(status.hs);
		status.processTimes = m_processTimes;
		status.processed = m_processed;
		status.spendTime = m_spendTime;
	}

protected:
	void startTrace()
	{
		// Xperf : Start "NT Kernel Logger" "Heap Logger"
		system("xperf -start" " \""KERNEL_LOGGER_NAMEA"\" "
			"-on PROC_THREAD+LOADER -stackwalk ThreadCreate+ImageLoad -RealTime -ClockType PerfCounter "
			">> xperf.log 2>&1");
		system("xperf -start" " \""HEAP_LOGGER_NAMEA"\" "
			"-heap -Pids 0 -stackwalk HeapCreate+HeapAlloc+HeapRealloc -RealTime -ClockType PerfCounter "
			">> xperf.log 2>&1");
	}

	void stopTrace()
	{
		// Xperf : Stop "NT Kernel Logger" "Heap Logger"
		system("xperf -stop" " \""KERNEL_LOGGER_NAMEA"\" "
			">> xperf.log 2>&1");
		system("xperf -stop" " \""HEAP_LOGGER_NAMEA"\" "
			">> xperf.log 2>&1");
	}

public:
	void Flush()
	{
		m_kernelSession.Flush();
		m_heapSession.Flush();
	}

	unsigned Process(uint64_t &processed)
	{
		int64_t staleTime;
		{
			int64_t k = m_kernelSession.LastTS();
			int64_t h = m_heapSession.LastTS();
			staleTime = max(k, h);
			staleTime = g_QPCHelper.GetQPCFrom(staleTime, -SESSION_CACHE_TIME_MS);
		}

		EventQueue::BlockList blk, blh;
		m_kernelSession.PopStales(staleTime, blk);
		m_heapSession.PopStales(staleTime, blh);

		EventQueue::BlockListTraver tk(&blk), th(&blh);

		while (!tk.end() && !th.end())
		{
			Event &ek = tk.elem();
			Event &eh = th.elem();
			if (ek.createtime() <= eh.createtime())
			{
				ek.inner()->consume();
				ek.inner()->destroy();
				tk.next();
			}
			else
			{
				eh.inner()->consume();
				eh.inner()->destroy();
				th.next();
			}
			++processed;
		}
		while (!tk.end())
		{
			Event &ek = tk.elem();
			ek.inner()->consume();
			ek.inner()->destroy();
			tk.next();
			++processed;
		}
		while (!th.end())
		{
			Event &eh = th.elem();
			eh.inner()->consume();
			eh.inner()->destroy();
			th.next();
			++processed;
		}

		m_lastTS = staleTime;

		if (!m_kernelSession.IsRunning())
			return m_kernelSession.ExitCode();
		if (!m_heapSession.IsRunning())
			return m_heapSession.ExitCode();
		return 0;
	}

	unsigned ThreadWorker()
	{
		for (size_t cnt = 0; !_ThreadBase::NeedExit(); ++cnt)
		{
			DWORD begin = GetTickCount();

			uint64_t processed = 0;

			getTrackSystem()->TrackBegin();
			Process(processed);
			getTrackSystem()->TrackEnd(m_lastTS);

			DWORD spendTime = GetTickCount() - begin;

			++m_processTimes;
			m_processed += processed;
			m_spendTime += spendTime;

			Sleep(200 > spendTime ? (200 - spendTime) : 0);
		}

		return 0;
	}

private:
	ETWSession m_kernelSession;
	ETWSession m_heapSession;

	class Flusher : public jtwsm::ThreadBase<Flusher>
	{
		typedef jtwsm::ThreadBase<Flusher> _ThreadBase;
		HeapMonitor* m_host;

	public:
		Flusher(HeapMonitor* host)
			: m_host(host)
		{
		}
		unsigned ThreadWorker()
		{
			for (size_t cnt = 0; !_ThreadBase::NeedExit(); ++cnt)
			{
				m_host->Flush();
				Sleep(SESSION_FLUSH_TIME_MS);
			}
			return 0;
		}

	} m_flusher;

	int64_t m_lastTS;

	// status
	uint64_t m_processTimes;
	uint64_t m_processed;
	uint64_t m_spendTime;

} g_heapMonitor;


bool initInnerDlls()
{
	std::ifstream f;
	f.open("innerdlls.txt");
	if (!f)
	{
		return false;
	}

	while (!f.eof())
	{
		// n xxx.dll
		char line[512];
		f.getline(line, sizeof(line));

		if (strlen(line) == 0)
		{
			continue;
		}

		if (!f)
		{
			return false;
		}

		char* d = strchr(line, ' ');
		if (d == NULL)
		{
			return false;
		}

		*d = 0;

		size_t level = atoi(line);

		getTrackSystem()->SetImgLevel(d + 1, level);
	}

	return true;
}

std::string readImgName(int argc, _TCHAR* argv[])
{
	const size_t IMAGENAME_MAX = 100;
	std::string imageName;

	if (argc > 1)
	{
#ifdef _UNICODE
		char tmp[IMAGENAME_MAX + 1] = {0};
		wcstombs(tmp, argv[1], IMAGENAME_MAX);
		imageName = tmp;
#else
		imageName = argv[1];
#endif
		printf("ImageName : %s\n", imageName.c_str());
	}
	else
	{
		printf("ImageName : ");
		char tmp[IMAGENAME_MAX + 1] = { 0 };
		fgets(tmp, IMAGENAME_MAX, stdin);
		char *newline = strchr(tmp, '\n');
		if (newline) *newline = 0;
		imageName = tmp;
	}
	printf("\n");

	return imageName;
}

bool heapTraceEXE(const char* name)
{
	std::string key = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
	key += name;

	HKEY hKey = NULL;
	LONG lRet = RegCreateKeyExA(
		HKEY_LOCAL_MACHINE,
		key.c_str(),
		0,
		NULL,
		0,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		NULL
		);
	if (lRet != ERROR_SUCCESS)
		return false;

	DWORD dw = 1;
	RegSetValueExA(
		hKey,
		"TracingFlags",
		0,
		REG_DWORD,
		(const BYTE*)&dw,
		sizeof(dw)
		);

	RegCloseKey(hKey);

	return true;
}

void heapUntraceEXE(const char* name)
{
	std::string key = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
	key += name;

	RegDeleteKeyA(
		HKEY_LOCAL_MACHINE,
		key.c_str()
		);
}


// LOG

static HANDLE g_logFile = INVALID_HANDLE_VALUE;

int myPrintfToLog(const char* format, ...)
{
	if (g_logFile == INVALID_HANDLE_VALUE)
		return 1;

	va_list arglist;
	va_start(arglist, format);

	char buffer[2048];
	int len = _vsnprintf_s(buffer, 2048, _TRUNCATE, format, arglist);
	if (len <= 0)
		return 0;

	DWORD cbWritten;
	WriteFile(g_logFile, buffer, len, &cbWritten, NULL);
	return 1;
}

int myPrintf(const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	char buffer[2048];
	int len = _vsnprintf_s(buffer, 2048, _TRUNCATE, format, arglist);
	if (len <= 0)
		return 0;

	printf(buffer);

	if (g_logFile != INVALID_HANDLE_VALUE)
	{
		DWORD cbWritten;
		WriteFile(g_logFile, buffer, len, &cbWritten, NULL);
	}

	return 1;
}

static void TurnOffLogFile()
{
	if (g_logFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_logFile);
		g_logFile = INVALID_HANDLE_VALUE;
	}
}

void cmdLogFile(int argc, wchar_t** argv)
{
	TurnOffLogFile();

	const wchar_t* fileName = L"HeapMonitorTrace.log";
	if (argc >= 1)
	{
		// turn off
		if (_wcsicmp(argv[0], L"off") == 0)
		{
			wprintf(L"LogFile is turned off\n");
			return;
		}
		else
		{
			fileName = argv[0];
		}
	}

	g_logFile = CreateFileW(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_logFile == INVALID_HANDLE_VALUE) {
		wprintf(L"Cannot open LogFile %ls\n", fileName);
		return;
	}

	SetFilePointer(g_logFile, 0, NULL, FILE_END);

	wchar_t fullName[MAX_PATH] = { 0 };
	wchar_t *pszFilePart = NULL;
	GetFullPathNameW(fileName, MAX_PATH, fullName, &pszFilePart);
	wprintf(L"LogFile is turned on, File=%ls\n", fullName);
}


void cmdStatus()
{
	HeapMonitor::STATUS status;
	g_heapMonitor.Status(status);

	myPrintf("%llu : %llu : %llu : %llu\n",
		status.processTimes, status.processed, status.spendTime,
		status.processed / (status.spendTime ? status.spendTime : 1) );
	myPrintf("%llu : %llu : %llu : %llu : %llu\n",
		status.ks.total, status.ks.success, status.ks.parseFailed,
		status.ks.maxDur, status.ks.versionIgnore);
	myPrintf("%llu : %llu : %llu : %llu : %llu\n",
		status.hs.total, status.hs.success, status.hs.parseFailed,
		status.hs.maxDur, status.hs.versionIgnore);
}

void cmdSnapshot(TSSAnalyzer &analyzer)
{
	myPrintf("Capture snapshot for now. Wait ...\n");
	int64_t nowTS = g_QPCHelper.GetQPCNow();
	while(nowTS > g_heapMonitor.LastTS())
	{
		Sleep(200);
	}
	analyzer.UpdateSnapshot();
	myPrintf("OK!\n");
}

void cmdCheck(int argc, wchar_t** argv)
{
	if (argc >= 1)
	{
		if (_wcsicmp(argv[0], L"heap") == 0 &&
			argc > 1)
		{
			tst_heapid hid = (tst_heapid)
				_wcstoui64(argv[1], NULL, 16);
			getTrackSystem()->CheckHeap(hid);
			return;
		}
	}
}

void cmdUse(TSSAnalyzer &analyzer, int argc, wchar_t** argv)
{
	if (argc > 1)
	{
		tst_pid pid = _wtoi(argv[1]);
		bool fSuccess = analyzer.SetTargetProcess(pid);
		if (fSuccess)
			myPrintf("Switch to process %u\n", pid);
		else
			myPrintf("Unknown process\n");
	}
	else
	{
		myPrintf("Current process is %u\n", analyzer.TargetProcess());
	}
}

void userCmdLoop()
{
	TSSAnalyzer analyzer;
	analyzer.Init();

	bool fAutoMode = true, lastAutoMode = false;

	const size_t USERCMD_MAX = 128;
	char cmd[USERCMD_MAX + 1] = { 0 };
	char lastCmd[USERCMD_MAX + 1] = { 0 };
	do
	{
		if (lastAutoMode != fAutoMode)
		{
			myPrintf(fAutoMode ? "Auto mode ON\n" : "Auto mode OFF\n");
			lastAutoMode = fAutoMode;
		}
		myPrintf(">");
		memset(cmd, 0, sizeof(cmd));
		fgets(cmd, USERCMD_MAX, stdin);

		char *newline = strchr(cmd, '\n');
		if (newline) *newline = 0;

		myPrintfToLog("%s\n", cmd);

		if (cmd[0] == 0)
			memcpy(cmd, lastCmd, USERCMD_MAX);

		wchar_t wcmd[USERCMD_MAX + 1] = {0};
		mbstowcs(wcmd, cmd, USERCMD_MAX);

		int argc = 0;
		wchar_t **argv = NULL;
		argv = CommandLineToArgvW(wcmd, &argc);
		if (argv == NULL)
		{
			continue;
		}

		if (_wcsicmp(argv[0], L"exit") == 0)
		{
			break;
		}
		else if (_wcsicmp(argv[0], L"clear") == 0 ||
			_wcsicmp(argv[0], L"cls") == 0)
		{
			system("cls");
		}
		else if (_wcsicmp(argv[0], L"logfile") == 0)
		{
			cmdLogFile(argc - 1, argv + 1);
		}
		else if (_wcsicmp(argv[0], L"status") == 0)
		{
			cmdStatus();
		}
		else if (_wcsicmp(argv[0], L"auto") == 0)
		{
			if (argc > 1 && _wcsicmp(argv[1], L"off") == 0)
				fAutoMode = false;
			else
				fAutoMode = true;
		}
		else if (_wcsicmp(argv[0], L"snapshot") == 0 ||
			_wcsicmp(argv[0], L"ss") == 0)
		{
			cmdSnapshot(analyzer);
			fAutoMode = false;
		}
		else if (_wcsicmp(argv[0], L"check") == 0)
		{
			cmdCheck(argc - 1, argv + 1);
		}
		else if (_wcsicmp(argv[0], L"use") == 0)
		{
			cmdUse(analyzer, argc, argv);
		}
		else
		{
			if (fAutoMode)
				analyzer.UpdateSnapshot();

			bool fKnown = analyzer.Command(argc, argv);

			if (!fKnown)
				myPrintf("Unknown command\n");
		}

		myPrintf("\n");

		LocalFree(argv);
		memcpy(lastCmd, cmd, USERCMD_MAX);

	} while (true);

	analyzer.Term();
}


int _tmain(int argc, _TCHAR* argv[])
{
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	setlocale( LC_ALL, "" );
	SetConsoleTitleW(L"Heap Monitor Controller");

	initInnerDlls();

	std::string imageName = readImgName(argc, argv);
	getTrackSystem()->SetFocus(imageName.c_str());

	if (!g_heapMonitor.Start())
	{
		printf("Start heap monitor failed\n");
		return 1;
	}

	heapTraceEXE(imageName.c_str());

	printf("All ready! Start your application now ...\n\n");

	initSymbol();

	userCmdLoop();

	termSymbol();

	heapUntraceEXE(imageName.c_str());

	g_heapMonitor.Stop();

	TurnOffLogFile();

#if NDEBUG
	_exit(0);
#endif

	return 0;
}
