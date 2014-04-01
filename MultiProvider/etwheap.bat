@setlocal

@call etwcommonsettings.bat

@if "%1" == "" goto quit
@if "%2" == "" goto quit

@rem     Kernel provider settings:
@set KernelFlags=latency+dispatcher+virt_alloc
@set KWalkFlags=VirtualAlloc
@set KBuffers=-buffersize 1024 -minbuffers 200

@rem     Heap provider settings:
@set HeapSessionName=HeapSession
@set HWalkFlags=HeapCreate+HeapDestroy
@rem Don't stack trace on HeapFree because we normally don't need that information.
@rem +HeapFree
@rem Heap tracing creates a lot of data so we need lots of buffers to avoid losing events.
@set HBuffers=-buffersize 1024 -minbuffers 512

@rem     User provider settings:
@set SessionName=gamesession

@rem Stop the circular tracing if it is enabled.
@call etwcirc stop

@rem Enable heap tracing for your process as describe in:
@rem http://msdn.microsoft.com/en-us/library/ff190925(VS.85).aspx
@rem Note that this assumes that you pass the file name of your EXE.
reg add "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\%2" /v TracingFlags /t REG_DWORD /d 1 /f
@if not %errorlevel% == 0 goto failure

@echo Heap tracing is now enabled for future invocations of process %2. If profiling
@echo startup allocations, hit a key now and then start your application. If profiling
@echo allocations later in the application then start your application now and hit a
@echo key when you are ready to start tracing of allocations.
@pause

@rem Start kernel tracing
xperf -on %KernelFlags% -stackwalk %KWalkFlags% %KBuffers%
@if not %errorlevel% equ 0 goto failure

@rem Start heap tracing
xperf -start %HeapSessionName% -heap -Pids 0 -stackwalk %HWalkFlags% %HBuffers% -f \tmpHeap.etl
@if not %errorlevel% equ 0 goto failure

@rem Start user-provider tracing
xperf -start %SessionName% -on %UserProviders%+%CustomProviders% -f \tempUser.etl
@if not %errorlevel% equ -2147023892 goto NotInvalidFlags
@echo Trying again without the custom providers. Run ETWRegister.bat to register them.
xperf -start %SessionName% -on %UserProviders% -f \tempUser.etl
:NotInvalidFlags
@if not %errorlevel% equ 0 goto failure
@rem Record information about the loggers at the start of the trace.
xperf -loggers >preloggers.txt

@echo Hit a key when you are ready to stop heap tracing.
@pause

@rem Record information about the loggers (dropped events, for instance)
xperf -loggers >loggers.txt
xperf -stop %SessionName% -stop %HeapSessionName% -stop "NT Kernel Logger" -d %1

@rem Delete heap tracing registry key
reg delete "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\%2" /v TracingFlags /f
@echo Trace is in %1
@rem Delete the temporary ETL files
@del \*.etl
@echo Trace data is in %1 -- load it with xperfview or gpuview. Logger details are in preloggers.txt and loggers.txt
xperf %1
@rem Restart circular tracing.
@call etwcirc StartSilent
@exit /b

:quit
@echo Specify an ETL file name and process name (not full path)
@echo Example: etwheap.bat allocdata.etl notepad.exe
@exit /b

:failure
@rem Check for Access Denied
@if %errorlevel% == %ACCESSISDENIED% goto NotAdmin
@echo Failed to start tracing. Make sure the custom providers are registered
@echo (using etwregister.bat) or remove the line that adds them to UserProviders.
@echo Make sure you are running from an elevated command prompt.
@echo Forcibly stopping the kernel and user session to correct possible
@echo "file already exists" errors.
xperf -stop %SessionName%
xperf -stop %HeapSessionName%
xperf -stop "NT Kernel Logger"
@del \*.etl
@exit /b

:NotAdmin
@echo You must run this batch file as administrator.
@exit /b
