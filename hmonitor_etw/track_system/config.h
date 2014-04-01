/*
 *  File   : track_system/config.h
 *  Author : Jiang Wangsheng
 *  Date   : 2014/03/21 14:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __TRACK_SYSTEM_CONFIG_H__
#define __TRACK_SYSTEM_CONFIG_H__


typedef uint64_t tst_time;
typedef uint64_t tst_pointer;
typedef uint64_t tst_ptdiffer;

typedef uint32_t tst_pid;
typedef uint32_t tst_tid;

typedef tst_pointer tst_imgid;
typedef uint32_t tst_imgsz;

typedef uint64_t tst_heapid;
typedef uint64_t tst_stackid;

typedef uint32_t tst_exit;

// Image Relative Address
union IRA
{
	// valid ? ImageRelativeAddress : NormalAddress
	//
	// little endian && x64/x86 address space layout
	//
	struct {
	uint32_t offset;
	uint32_t index : 31;
	uint32_t valid : 1;
	};
	uint64_t u64;
};


#endif /* __TRACK_SYSTEM_CONFIG_H__ */
