/*
 *  File   : track_system/track_system.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/02/25 14:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __TRACK_SYSTEM_TRACK_SYSTEM_H__
#define __TRACK_SYSTEM_TRACK_SYSTEM_H__

#include "config.h"
#include "snapshot.h"


struct STACKINFO
{
	tst_stackid stackid;
	size_t depth;
	const tst_pointer* frames;
};

struct ITrackSystem
{
	virtual void SetFocus(
		const char* name
		) = 0;

	virtual void SetInnerDll(
		const char* dll
		) = 0;

	virtual void TrackBegin(
		) = 0;

	virtual void OnProcessStart(
		tst_time tsStart,
		tst_pid pid,
		tst_pid ppid,
		const char* name
		) = 0;
	virtual void OnProcessEnd(
		tst_time tsEnd,
		tst_pid pid,
		tst_exit code
		) = 0;

	virtual void OnImageLoad(
		tst_time tsLoad,
		tst_pid pid,
		tst_pointer base,
		tst_imgsz size,
		const char* name,
		STACKINFO siLoad
		) = 0;
	virtual void OnImageUnload(
		tst_time tsUnload,
		tst_pid pid,
		tst_pointer base
		) = 0;

	virtual void OnHeapCreate(
		tst_time tsCreate,
		tst_pid pid,
		tst_heapid heapid,
		STACKINFO siCreate
		) = 0;
	virtual void OnHeapDestroy(
		tst_time tsDestroy,
		tst_pid pid,
		tst_heapid heapid
		) = 0;

	virtual void OnThreadStart(
		tst_time tsStart,
		tst_pid pid,
		tst_tid tid,
		tst_pointer userStack,
		tst_pointer entry,
		STACKINFO siStart
		) = 0;
	virtual void OnThreadEnd(
		tst_time tsEnd,
		tst_pid pid,
		tst_tid tid
		) = 0;

	virtual void OnHeapAlloc(
		tst_time tsAlloc,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer addr,
		tst_ptdiffer size,
		tst_tid tid,
		STACKINFO siAlloc
		) = 0;
	virtual void OnHeapRealloc(
		tst_time tsRealloc,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer newAddr,
		tst_ptdiffer newSize,
		tst_pointer oldAddr,
		tst_ptdiffer oldSize,
		tst_tid tid,
		STACKINFO siRealloc
		) = 0;
	virtual void OnHeapFree(
		tst_time tsFree,
		tst_pid pid,
		tst_heapid heapid,
		tst_pointer addr
		) = 0;

	virtual void TrackEnd(
		tst_time tsLast
		) = 0;

	virtual void Snapshot(
		TSS_System &sys
		) = 0;
};


ITrackSystem* getTrackSystem();

#endif /* __TRACK_SYSTEM_TRACK_SYSTEM_H__ */
