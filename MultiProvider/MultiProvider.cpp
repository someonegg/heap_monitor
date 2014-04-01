// MultiProvider.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "etwprof.h"
#include <assert.h>

// Pause for a short period of time with the CPU idle.
__declspec(noinline) void IdleDelay()
{
	ETWMark("Going to sleep for a bit...");
	Sleep(10);
}

// Pause for a short period of time with the CPU busy.
__declspec(noinline) void BusyDelay()
{
	ETWMark("Wasting CPU time for a bit...");
	DWORD startTick = GetTickCount();

	for (;;)
	{
		DWORD elapsed = GetTickCount() - startTick;
		if (elapsed > 10)
			break;
	}
}

__declspec(noinline) void TestMainProvider(int argc)
{
	IdleDelay();
	ETWMark("After an idle delay");

	IdleDelay();
	ETWMarkPrintf("argc is %d", argc);

	// Record Begin/End event pairs. Normally this is best
	// done with CETWScope.
	int64 startTick = ETWBegin("Busy work...");
	BusyDelay();
	ETWEnd("Busy work...", startTick);
}

__declspec(noinline) void TestWorkerProvider(int argc)
{
	IdleDelay();
	ETWWorkerMark("After an idle delay");

	IdleDelay();
	ETWWorkerMarkPrintf("argc is %d", argc);

	// Record Begin/End event pairs. Normally this is best
	// done with CETWScope.
	int64 startTick = ETWWorkerBegin("Busy work...");
	BusyDelay();
	ETWWorkerEnd("Busy work...", startTick);
}

__declspec(noinline) void TestHeapProvider()
{
	CETWScope timer("Lots of memory allocations");
	void* allocations[2400];

	const size_t delayInterval = 32;
	for (size_t i = 0; i < _countof(allocations); ++i)
	{
		// Allocate a random amount of memory.
		allocations[i] = malloc(1024 + rand());
		// Put in occasional delays to make for a better looking graph over time
		if ((i % delayInterval) == 0)
			IdleDelay();
	}

	// Free some of the memory, but not all. This allows demonstration
	// of AIFI (Allocated Inside Freed Inside) versus AIFO (Allocated Inside
	// Freed Outside) types in summary tables.
	for (size_t i = 0; i < _countof(allocations); ++i)
	{
		// 50% chance of freeing each block
		if (rand() < RAND_MAX / 2)
		{
			free(allocations[i]);
		}
		// Put in occasional delays to make for a better looking graph over time
		if ((i % delayInterval) == 0)
			IdleDelay();
	}


	// Put in a long enough delay so that the graph will show the allocaiton count
	// stabilizing before zeroing out at process exit.
	for (int i = 0; i < 20; ++i)
		IdleDelay();
}



LRESULT CALLBACK LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	assert(nCode == HC_ACTION);
	// wParam is WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP

	KBDLLHOOKSTRUCT* pKbdLLHook = (KBDLLHOOKSTRUCT*)lParam;

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
	{
		ETWKeyDown(pKbdLLHook->vkCode, 0, 0);
	}
	
	return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	assert(nCode == HC_ACTION);

	MSLLHOOKSTRUCT* pMouseLLHook = (MSLLHOOKSTRUCT*)lParam;

	int whichButton = -1;
	bool pressed = true;

	switch( wParam )
	{
	case WM_LBUTTONDOWN:
		whichButton = 0;
		break;
	case WM_MBUTTONDOWN:
		whichButton = 1;
		break;
	case WM_RBUTTONDOWN:
		assert(0);
		whichButton = 2;
		break;
	case WM_LBUTTONUP:
		whichButton = 0;
		pressed = false;
		break;
	case WM_MBUTTONUP:
		whichButton = 1;
		pressed = false;
		break;
	case WM_RBUTTONUP:
		whichButton = 2;
		pressed = false;
		break;
	}

	if ( whichButton >= 0 )
	{
		if ( pressed )
		{
			ETWMouseDown( whichButton, 0, pMouseLLHook->pt.x, pMouseLLHook->pt.y );
		}
		else
		{
			ETWMouseUp( whichButton, 0, pMouseLLHook->pt.x, pMouseLLHook->pt.y );
		}
	}
	else
	{
		if ( wParam == WM_MOUSEWHEEL )
		{
			int wheelDelta = GET_WHEEL_DELTA_WPARAM( pMouseLLHook->mouseData );
			ETWMouseWheel( 0, wheelDelta, pMouseLLHook->pt.x, pMouseLLHook->pt.y );
		}

		if ( wParam == WM_MOUSEMOVE )
		{
			ETWMouseMove( 0, pMouseLLHook->pt.x, pMouseLLHook->pt.y );
		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}



DWORD __stdcall InputThread(LPVOID)
{
	// Install a hook so that this thread will receive all keyboard messages. They must be
	// processed in a timely manner or else bad things will happen. Doing this on a
	// separate thread is a good idea, but even then bad things will happen to your system
	// if you halt in a debugger. Even simple things like calling printf() from the hook
	// can easily cause system deadlocks which render the mouse unable to move!
	HHOOK keyHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardHook, NULL, 0);
	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseHook, NULL, 0);

	if (!keyHook && !mouseHook)
		return 0;

	// Run a message pump -- necessary so that the hooks will be processed
	BOOL bRet;
	MSG msg;
	// Keeping pumping messages until WM_QUIT is received. If this is opened
	// in a child thread then you can terminate it by using PostThreadMessage
	// to send WM_QUIT.
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
			// handle the error and possibly exit
			break;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	}

	// Unhook and exit.
	if (keyHook)
		UnhookWindowsHookEx(keyHook);
	if (mouseHook)
		UnhookWindowsHookEx(mouseHook);
	return 0;
}

void InputLogger()
{
	printf("Logging keyboard input to ETW events - type Ctrl+C to terminate.\n");

	InputThread(NULL);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc > 1 && _wcsicmp(argv[1], L"-inputlogger") == 0)
	{
		InputLogger();
		return 0;
	}

	printf("Emitting ETW events...\n");

	// Insert named Begin/End events to mark the start and stop
	// of main().
	CETWScope globalTimer(__FUNCTION__);

	TestMainProvider(argc);
	TestWorkerProvider(argc);

	// Test the render provider.
	for (int i = 0; i < 40; ++i)
	{
		ETWRenderFrameMark();
		IdleDelay();
	}

	// Test the input provider
	// The first parameter indicates which button -- conventionally this is
	// zero for left, 1 for middle, and 2 for right.
	ETWMouseDown(2, 0, 10, 20);
	ETWMouseUp(2, 0, 10, 20);

	TestHeapProvider();

	printf("Done.\n");
	return 0;
}
