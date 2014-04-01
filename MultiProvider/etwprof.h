//============ Copyright (c) Cygnus Software, All rights reserved. ============
//
// ETW (Event Tracing for Windows) profiling helpers.
// This allows easy insertion of Generic Event markers into ETW/xperf tracing
// which then aids in analyzing the traces and finding performance problems.
// The usage patterns are to use ETWBegin and ETWEnd (typically through the
// convenience class CETWScope) to bracket time-consuming operations. In addition
// ETWFrameMark marks the beginning of each frame, and ETWMark can be used to
// mark other notable events. More event types and providers can be added as needed.
//
//===============================================================================

#ifndef ETWPROF_H
#define ETWPROF_H
#if defined( _MSC_VER )
#pragma once
#endif

typedef long long int64;

#ifdef	_WIN32
// ETW support should be compiled in for all Windows PC platforms. It isn't
// supported on Windows XP but that is determined at run-time.
#define	ETW_MARKS_ENABLED
#endif

// Flag to indicate that a mouse-down actually corresponds to a double-click.
// Add this to the button number.
const int kFlagDoubleClick = 100;

#ifdef	ETW_MARKS_ENABLED

// Could use this for __declspec(export)
#define PLATFORM_INTERFACE
#include <sal.h> // For _Printf_format_string_

// Insert a single event to mark a point in an ETW trace. The return value is a 64-bit
// time stamp.
PLATFORM_INTERFACE int64 ETWMark( const char *pMessage );
PLATFORM_INTERFACE int64 ETWWorkerMark( const char *pMessage );
// _Printf_format_string_ is used by /analyze
PLATFORM_INTERFACE int64 ETWMarkPrintf( _Printf_format_string_ const char *pMessage, ... );
PLATFORM_INTERFACE int64 ETWWorkerMarkPrintf( _Printf_format_string_ const char *pMessage, ... );

// Insert a begin event to mark the start of some work. The return value is a 64-bit
// time stamp which should be passed to the corresponding ETWEnd function.
PLATFORM_INTERFACE int64 ETWBegin( const char *pMessage );
PLATFORM_INTERFACE int64 ETWWorkerBegin( const char *pMessage );

// Insert a paired end event to mark the end of some work.
PLATFORM_INTERFACE int64 ETWEnd( const char *pMessage, int64 nStartTime );
PLATFORM_INTERFACE int64 ETWWorkerEnd( const char *pMessage, int64 nStartTime );

// Mark the start of the next render frame. bIsServerProcess must be passed
// in consistently for a particular process.
PLATFORM_INTERFACE void ETWRenderFrameMark();
// Return the frame number recorded in the ETW trace -- useful for synchronizing
// other profile information to the ETW trace.
PLATFORM_INTERFACE int ETWGetRenderFrameNumber();

// Button numbers are 0, 1, 2 for left, middle, right, with kFlagDoubleClick added
// in for double clicks.
PLATFORM_INTERFACE void ETWMouseDown( int nWhichButton, unsigned flags, int nX, int nY );
PLATFORM_INTERFACE void ETWMouseUp( int nWhichButton, unsigned flags, int nX, int nY );
PLATFORM_INTERFACE void ETWMouseMove( unsigned flags, int nX, int nY );
PLATFORM_INTERFACE void ETWMouseWheel( unsigned flags, int zDelta, int nX, int nY );
PLATFORM_INTERFACE void ETWKeyDown( unsigned nChar, unsigned nRepCnt, unsigned flags );

// This class calls the ETW Begin and End functions in order to insert a
// pair of events to bracket some work.
class CETWScope
{
public:
	CETWScope( const char *pMessage )
		: m_pMessage( pMessage )
	{
		m_nStartTime = ETWBegin( pMessage );
	}
	~CETWScope()
	{
		ETWEnd( m_pMessage, m_nStartTime );
	}
private:
	// Private and unimplemented to disable copying.
	CETWScope( const CETWScope& rhs );
	CETWScope& operator=( const CETWScope& rhs );

	const char* m_pMessage;
	int64 m_nStartTime;
};

#else

inline int64 ETWMark( const char* ) { return 0; }
inline int64 ETWMarkPrintf( const char *pMessage, ... ) { return 0; }
inline int64 ETWBegin( const char* ) { return 0; }
inline int64 ETWEnd( const char*, int64 ) { return 0; }
inline void ETWRenderFrameMark() {}
inline int ETWGetRenderFrameNumber() { return 0; }

inline void ETWMouseDown( int nWhichButton, unsigned int flags, int nX, int nY ) {}
inline void ETWMouseUp( int nWhichButton, unsigned int flags, int nX, int nY ) {}
inline void ETWMouseMove( unsigned int flags, int nX, int nY ) {}
inline void ETWMouseWheel( unsigned int flags, int zDelta, int nX, int nY ) {}
inline void ETWKeyDown( unsigned nChar, unsigned nRepCnt, unsigned flags ) {}

// This class calls the ETW Begin and End functions in order to insert a
// pair of events to bracket some work.
class CETWScope
{
public:
	CETWScope( const char* )
	{
	}
private:
	// Private and unimplemented to disable copying.
	CETWScope( const CETWScope& rhs );
	CETWScope& operator=( const CETWScope& rhs );
};

#endif

#endif // ETWPROF_H
