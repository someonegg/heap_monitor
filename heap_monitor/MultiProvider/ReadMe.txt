For more information on this project, for tutorials on xperf trace analysis, and
other related content, or to post comments go to:
http://randomascii.wordpress.com/category/xperf/

This project demonstrates several things related to ETW (Event Tracing for Windows)
and xperf:

1) How to create a manifest file describing multiple ETW providers.
2) How to use these providers in a cross-platform way, so that they silently
compile away on non-Windows platforms, and so that they don't prevent
the program from loading on Windows XP (which does not support ETW providers
of this type).
3) How to record xperf traces robustly, including normal traces with defined
start and stop points, heap traces (also with defined start and stop points)
and circular buffer tracing (for retroactive profiling) which is always
started and left running whenever one of the other types of tracing stops.
The included batch files are designed to easily adjusted for alternate
environments with different providers in different DLLs or EXEs.
Also included are batch files for changing the speed of the sampling profiler.
4) How to use the xperf processing commands to identify which symcache files
are used by a trace so that a trace and its symbols can easily be packaged
up for easy sharing.

Getting started:
1) Install xperf. Xperf comes with the Platform SDK which can currently be
found at http://msdn.microsoft.com/en-us/windows/bb980924.aspx. The component
you are looking for is Windows Performance Toolkit, currently located under
Common Utilities, and by default it will be installed to:
    "C:\Program Files\Microsoft Windows Performance Toolkit"
I recommend installing all of the Platform SDK because many of the tools are
extremely useful. However you should probably *not* install the Visual C++
Compilers since (as of June 2011) this is broken and will cause the install
to fail.

2) Load this project into Visual Studio 2010 and build it (debug or
release configuration).

3) From an elevated (administrator) command prompt with the current directory
set to the project directory run etwregister.bat. This registers the
ETW providers. If this step fails you have to figure out why before
continuing. The other batch files will continue to work without registering
the providers but they will spew error messages and will not be as functional.
Registering the providers is only necessary on those machines where you want
to be able to record rich ETW traces with custom information embedded.

4) From an elevated (administrator) command prompt run:
    "etwrecord.bat tracefile.etl"
This starts ETW tracing. Now do something that you want to profile such as
run this program. When this program has finished, hit enter in the elevated
command prompt to stop tracing. Xperfview will launch and a stacked series of
graphs will be displayed showing data such as:
    Sampling profiler events
    Context switch events
    CPU frequency
    File I/O
    Disk I/O
    Generic Events
The Generic Events graph is where the user provider events are stored.
They can be correlated with the other graphs to aid in analysis.

5) When "etwrecord.bat" finishes it starts up circular-buffer tracing by
invoking "etwcirc.bat". This allocates a few hundred MB of memory to use
for constantly recording ETW data. At any point you can, from an elevated
command prompt, run:
    "etwcirc.bat circtracefile.etl"
This records the current contents of the circular buffers to disk, thus
effectively giving you post-mortem profiling. Typically this will record
the last one or two minutes of activity, although the exact elapsed time
recorded will depend on the data rate and on the buffer sizes. Note that
two separate sets of circular buffers are used so that the Generic Events
graph will typically cover a much longer time than the other graphs. When
the trace has been recorded then xperfview will launch.

6) ETW/xperf can also be used for heap profiling -- recording every memory
allocation made by a process. To do this go to an elevated command prompt
and run:
    etwheap.bat allocations.etl MultiProvider.exe"
This starts ETW tracing and also turns on the heap provider for future launches
of MultiProvider.exe. Now you have two choices:
1) If you want to do heap tracing of application startup then hit Enter to start
tracing and then run MultiProvider.exe.
2) If you want to do heap tracing from the middle of the application run then
run MultiProvider.exe, get to the scenario you want to trace, then hit Enter to
start tracing.
The crucial thing is that you must run the batch file before running
the specified program, or else heap tracing will not work.
When your scenario is finished, hit enter in the elevated command prompt to stop
tracing. Xperfview will launch and the usual graphs will be displayed, with
additional graphs for displaying heap allocations. The important new graphs are
called Heap Outstanding Allocation Size, and Heap Oustanding Allocation Count.
These graphs record all heap allocations (with call stacks on allocations)
from the time that tracing started. As usual the summary tables allow deep
analysis, including exploring allocations by type (freed or not) and
call stack.
    ETW heap profiling is a superset of regular ETW profiling. The reason
you shouldn't use it all the time is because the data rate is typically *much*
higher, and it means that profiling must be set up before the process is
launched.

7) If you have Python installed and you want to package up all of the .symcache files
(stripped versions of the symbols, typically stored in c:\symcache) to make it
easier to share a trace then you can run ETWPackSymbols.py

8) If you are going to use this code as the basis for your own ETW providers then
be sure to change the GUIDs and provider names in ETWProvider.man. When you change
the provider names you will need to modify ETWCommonSettings.bat (to request
recording those new provider names) and etwprof.cpp (to adjust the run-time
registering of the providers and the emitting of events). You should use GUID
Generator to create new GUIDs.
If you use these ETW providers in your own code then you will need to
adjust the executable names (or DLLs -- providers can be associated with
DLLs) in ETWProvider.man and ETWRegister.bat.
