请在hmonitor_etw.exe当前目录放入
	dbghelp.dll symsrv.dll (可以从调试器里拷贝过来)
	innerdlls.txt (工程路径下)

符号路径
	当前目录下symbols子目录  +  _NT_SYMBOL_PATH


exit
	退出
clear cls
	清屏

logfile
	logfile			开启LOG输出到默认文件HeapMonitorTrace.log
	logfile filexxx.log	开启LOG输出到filexxx.log
	logfile off		关闭LOG输出

use
	查看当前进程
use pid
	切换到进程

pl ple
	查看进程列表
tl tle
	查看线程列表
	ext 线程创建堆栈
il ile
	查看模块列表
	ext 模块加载堆栈
hl hle
	查看Heap列表
	ext Heap 创建堆栈
sl sle
	查看Stack列表
	ext Stack 符号信息


l 系列命令

参数 [Condition] TopLimit SortType Order
TopLimit	显示Top N
SortType	CountTotal CountCurrent CountPeak BytesTotal BytesCurrent BytesPeak
Order		DSC or ASC
TopLimit SortType Order 三项顺序无关

Condition
格式		"c:xxx"
参数		CountTotal CountCurrent CountPeak BytesTotal BytesCurrent BytesPeak
		[sl sle] ImgId HeapId
运算符		==   ~=   <   >   <=   >=   and   or   not   (   )

输出
xxID  (CountTotal, CountCurrent, CountPeak) (BytesTotal, BytesCurrent, BytesPeak)


AUTO 模式
	开启的情况下，l 系列命令执行前都更新快照
	auto 模式切换的时候会有提示

auto
	开启auto模式
auto off
	关闭 auto 模式

ss snapshot
	截取当前的快照，并关闭auto模式
