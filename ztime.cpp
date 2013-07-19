// @ZBS {
//		*MODULE_NAME High Precision Full Scale Timer
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A portable time system that keeps track of time with high precision
//			and UTC range as a double.  Includes handy framework for real-time
//			systems and games.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES ztime.cpp ztime.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 Naming convention totally revised.
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:

#ifdef WIN32
	//	#include "windows.h"
	//	#include "mmsystem.h"
	// The following lines duplicate the code needed from windows.h
	// In case of a compile problem, comment out the following and
	// uncomment the above includes.
	extern "C" {
		__declspec(dllimport) unsigned long __stdcall SetThreadAffinityMask( void *hThread, unsigned long dwThreadAffinityMask );
		__declspec(dllimport) void *__stdcall GetCurrentThread( void );
		typedef union _LARGE_INTEGER {
			struct { unsigned long LowPart; long HighPart; };
			struct { unsigned long LowPart; long HighPart; } u;
			__int64 QuadPart;
		} LARGE_INTEGER;
		__declspec(dllimport) int __stdcall QueryPerformanceCounter( LARGE_INTEGER *lpPerformanceCount );
		__declspec(dllimport) int __stdcall QueryPerformanceFrequency( LARGE_INTEGER *lpFrequency );
		__declspec(dllimport) void __stdcall Sleep( unsigned long dwMilliseconds );
		__declspec(dllimport) int __stdcall timeGetTime();
		__declspec(dllimport) void __stdcall timeBeginPeriod(unsigned int period);
		__declspec(dllimport) void __stdcall GetLocalTime(void *);
		__declspec(dllimport) void __stdcall GetSystemTime(void *);
	}

	struct SYSTEMTIME {
		unsigned short wYear;
		unsigned short wMonth;
		unsigned short wDayOfWeek;
		unsigned short wDay;
		unsigned short wHour;
		unsigned short wMinute;
		unsigned short wSecond;
		unsigned short wMilliseconds;
	};
#else
	#include "sys/time.h"
	#include "unistd.h"
#endif

// STDLIB includes:
#include "time.h"
#include "math.h"
#include "stdlib.h"
#include "memory.h"
#include "assert.h"
#include "stdio.h"
#include "string.h"
// MODULE includes:
#include "ztime.h"
// ZBSLIB includes:

#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

// Globals
//------------------------------------------------------------------------

int zTimeFrameCount = 0;
	// Incremented each time zTimeTick() is called (once per frame)

ZTime zTime = 0.0;
	// The time during the last call to zTimeSetFrameTime(),

ZTime zTimeDT = 0.0;
	// Delta time on the last frame.

float zTimeDTF = 0.f;
	// Delta time in float on the last frame.

ZTime zTimeFPS = 0.0;
	// The last calculated frames per second.
	// Always based on local time as opposed to the master time

int zTimeAvgWindow = 16;
	// The number of frames to averge when computing avgFPS

ZTime zTimeAvgFPS = 0.0;
	// The running average of the last DTIME_FRAMES_TO_AVG 
	// Always based on local time as opposed to the master time

ZTime zTimeStart = 0.0;
	// Time that the zTime system was initialized in local time

ZTime *zTimeAvgs = 0;
	// Array of the last tick times for FPS avergaing

int zTimeAvgCount = 0;
	// Number of samples currently in the zTimeAvgs array.

int zTimeLastAvgWindow = -1;
	// The size of the last allocation done for the averging window buffer

int zTimeInited = 0;
	// Has the system been initialized

ZTime zTimeLastTick = 0.0;
	// Last tick

int zTimePaused = 0;
	// Flag indicating if time is paused.  (Maybe increment past 1)

ZTime zTimeWhenPaused = 0.0;
	// The time when it was put into pause mode.

ZTime zTimePausedAccumulator = 0.0;
	// Each time zTimeResume() is called, this accumulator is incremented

ZTime zTimeClockFrequency = 0.0;
	// The frequency of the clock.  (Only used under Win32)

ZTime zTimeSim = 0.0;
	// The sim time during the last call to zTimeTick()

ZTime zTimeSimStart = 0.0;
	// Time that the zTime system was initialized in sim time

ZTime zTimeSimDT = 0.0;
	// Delta time on the last frame, in simulation time domain

float zTimeSimDTF = 0.0f;
	// Delta time in float on the last frame, in simulation time domain

// Statics
//------------------------------------------------------------------------
static ZTime zTimeSimOldDT = 0.0;
	// This stores the old simulation DT when paused in ZT_FIXED mode.

static ZTimeMode zTimeSimMode = ZT_REAL;
	// The mode used for the simulation time domain.

static void zTimeSimInit ()
{
	switch (zTimeSimMode)
	{
	default:
	case ZT_REAL:
		zTimeSimStart = zTimeStart;
		zTimeSim = zTime;
		break;

	case ZT_FIXED:
		zTimeSimStart = 0.0;
		zTimeSim = 0.0;
		break;
	}

}

// Public Interface Functions
//------------------------------------------------------------------------

void zTimeInit() {
	zTimeInited = 1;
 	#ifdef WIN32
		unsigned long mask = 1;
		SetThreadAffinityMask( GetCurrentThread(), mask );
			// To avoid time running backwards bug on AMD, etc.

			// Random quote from online:
			//I am having exactly the same problem with my new athlon 64 x2. I've 
			//been looking for a solution and found this thread, and a couple of other 
			//threads, about the problem. The solution I just found experimenting is a 
			//bit dirty, but quite good for me (I am main developer of an 
			//online game and actually implementing multi core support for multiple game clients and stuff)
			//, so maybe can do the trick for you too.
			// force the thread to CPU 0 DWORD oldmask=SetThreadAffinityMask(GetCurrentThread(), 1); 
			//. . ... here you call QueryPerformanceFrequency(), QueryPerformanceCounter(), etc..... . . 
			// let the thread wander around cpus as usual SetThreadAffinityMask(GetCurrentThread(), oldmask); 
			//In fact this solution works amazingly well. I had my doubts about 
			//windows being so kind as to switch the thread to cpu 0 so fast, 
			//but it really does. Once you exit from the first SetThreadAffinityMask() you 
			//are guaranteed to be running on the cpu 0 of the system. 
			//It is way better than to lock a whole thread to cpu 0 permanently, 
			//as usually the timing-paranoid thread uses to be the 3d processing/rendering thread, 
			//just the thread that is more in need of real hardware multithreading. 
			//Best regards Toni Pomar www.prisonserver.co.uk 

		timeBeginPeriod(1);
			// "It happens that calling timeBeginPeriod(1) sets the resolution of Sleep
			// to 1 ms as well as its documented purpose of setting the multimedia
			// timer resolution to 1 ms.  Direct3D uses this to improve the resolution
			// of Sleep for some features."
			//  --Max McMullen, Direct3D

		__int64 frequency;
		QueryPerformanceFrequency( (LARGE_INTEGER *)&frequency );
		zTimeClockFrequency = (double)frequency;
	#endif
	zTimeStart = zTimeNow();
	zTimeLastTick = zTimeStart;
	zTimeSimInit ();
} 

ZTime zTimeNow() {
	if( !zTimeInited ) {
		zTimeInit();
	}

	#ifdef WIN32
		__int64 counter;
		QueryPerformanceCounter( (LARGE_INTEGER *)&counter );
		return (ZTime)counter / zTimeClockFrequency;
	#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday( &tv, &tz );

		int _mils = tv.tv_usec/1000;
		int _secs = tv.tv_sec;

		return (ZTime)_secs + ((ZTime)_mils/1000.0);
	#endif
}

void zTimeTick() {
	if( !zTimeInited ) {
		zTimeInit();
	}

	if( zTimePaused ) {
		zTime = zTimeWhenPaused;
		zTimeDT = 0.0;
		zTimeDTF = 0.f;
	}
	else {
 		zTime = zTimeNow() - zTimePausedAccumulator;
		zTimeDT = zTime - zTimeLastTick;
		zTimeDTF = (float)zTimeDT;
	}


#ifdef DEBUG_TIME_RUNNING_BACKWARDS
	// print some debug info
	extern void trace( char *msg, ... );
	if( zTimeFrameCount > 0 && zTimeDT <= 0.0 ) {
		trace( "zTimeFrameCount = %d\n", zTimeFrameCount );
		trace( "zTimeStart = %g\n", zTimeStart );
		trace( "zTimePausedAccumulator = %g\n", zTimePausedAccumulator );
		trace( "zTimeIsPaused = %d\n", zTimeIsPaused );
		trace( "zTimeWhenPaused = %g\n", zTimeWhenPaused );
		trace( "zTimeLastTick = %g\n", zTimeLastTick );
		trace( "zTime = %g\n", zTime );
		trace( "zTimeDT = %g\n", zTimeDT );
	}
#endif

	zTimeLastTick = zTime;

	// Set the current fps as long as we have one sample
	if( zTimeFrameCount > 0 ) {
		zTimeFPS = 0.0;
		if( zTimeDT > 0.0 ) {
			zTimeFPS = 1.0 / zTimeDT;
		}
		else if( zTimeDT < 0.0 ) {
			int timeRunningBackwards = 1;
			assert( !timeRunningBackwards );
			// ZBS added 3 July 2007 after discovering that time was running backwards
			// on my machine.  I did a windows update and the problem went away.
			// Aparently the problem is related to multi-core machines switching
			// between threads so that QueryPerformanceCounter fails
			// No, the problem didn't go away.  It's still here.

		}
	}

	// REALLOC the averaging window if necessary
	if( zTimeAvgWindow != zTimeLastAvgWindow ) {
		ZTime *newAvgs = (ZTime *)realloc( zTimeAvgs, sizeof(ZTime) * zTimeAvgWindow );
		zTimeAvgCount = min( zTimeLastAvgWindow, zTimeAvgWindow );
		if( zTimeLastAvgWindow > 0 ) {
			memcpy( newAvgs, zTimeAvgs, sizeof(ZTime) * zTimeAvgCount );
		}
		zTimeAvgs = newAvgs;
		zTimeLastAvgWindow = zTimeAvgWindow;
	}

	zTimeAvgs[ zTimeFrameCount % zTimeAvgWindow ] = zTime;
	zTimeAvgCount++;

	// SET the average as long as we have enough samples.
	if( zTimeAvgCount > zTimeAvgWindow ) {
		ZTime elapsed = zTime - zTimeAvgs[ (zTimeFrameCount+1) % zTimeAvgWindow ];
		zTimeAvgFPS = 0.0;
		if( elapsed > 0.0 ) {
			zTimeAvgFPS = (ZTime)zTimeAvgWindow / elapsed;
		}
	}

	zTimeFrameCount++;

	//
	// Simulation time domain
	//

	switch (zTimeSimMode)
	{
	default:
	case ZT_REAL:
		// Sync the sim variables to the real time variables
		zTimeSimDT = zTimeDT;
		zTimeSimDTF = zTimeDTF;
		zTimeSim = zTime;
		break;

	case ZT_FIXED:
		// If paused, clear the deltas
		if( zTimePaused ) {
			zTimeSimDT = 0.0;
			zTimeSimDTF = 0.0f;
		}
		else
		{
			// Update the current simulation time
			zTimeSim += zTimeSimDT;
		}
		break;
	}
}

int zTimeIsPaused() {
	return zTimePaused > 0;
}

void zTimePause() {
	if( zTimePaused == 0 ) {
		zTimeWhenPaused = zTimeNow();

		// Save the current delta time for later restoration
		if (zTimeSimMode == ZT_FIXED)
			zTimeSimOldDT = zTimeSimDT;  
	}
	zTimePaused++;
}

void zTimeResume() {
	if( zTimePaused ) {
		zTimePaused--;
		if( !zTimePaused ) {
			zTimePausedAccumulator += zTimeNow() - zTimeWhenPaused;

			if (zTimeSimMode == ZT_FIXED) {
				zTimeSimDT = zTimeSimOldDT;
				zTimeSimDTF = (float) zTimeSimOldDT;
			}
		}
	}
}

void zTimeResetPaused() {
	zTimePausedAccumulator = 0.0;
}

void zTimeSetSimMode (ZTimeMode mode)
{
	if (zTimeSimMode != mode)
	{
		// Reset the simulation time values and set the mode
		zTimeSimInit ();
		zTimeSimMode = mode;
	}
}

void zTimeSleepSecs( int secs ) {
	#ifdef WIN32
		Sleep( secs * 1000 );
	#else
		sleep( secs );
	#endif
}

void zTimeSleepMils( int mils ) {
	#ifdef WIN32
		Sleep( mils );
	#else
		timespec request;
		request.tv_sec = mils / 1000;
		request.tv_nsec = 1000000*(mils % 1000);
		nanosleep( &request, 0 );
	#endif
}

void zTimeGetLocalTime( int &hour, int &minute, int &second, int &millisecond ) {
	#ifdef WIN32
		SYSTEMTIME st;
		GetLocalTime( &st );
		hour = st.wHour;
		minute = st.wMinute;
		second = st.wSecond;
		millisecond = st.wMilliseconds;
	#else
		assert( 0 );
			// @TODO: port
	#endif
}

char *zTimeGetLocalTimeString( int includeMils ) {
	static char str[256];
	#ifdef WIN32
		SYSTEMTIME st;
		GetLocalTime( &st );
		char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		sprintf( str, "%s %d %s %d. %02d:%02d:%02d",
			days[st.wDayOfWeek],
			st.wDay,
			months[st.wMonth-1],
			st.wYear,
			st.wHour,
			st.wMinute,
			st.wSecond
		);
		if( includeMils ) {
			char buf[8];
			sprintf( buf, ".%03d", st.wMilliseconds );
			strcat( str, buf );
		}
	#else
		time_t t = time( 0 );
		sprintf( str, "%s", ctime( &t ) );
		
		if( includeMils ) {
			// @TODO
		}
	#endif
	return str;
}

char *zTimeGetLocalTimeStringNumeric( int includeMils ) {
	static char str[256];
	#ifdef WIN32
		SYSTEMTIME st;
		GetLocalTime( &st );
		sprintf( str, "%d%02d%02d %02d:%02d:%02d",
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond
		);
		if( includeMils ) {
			char buf[8];
			sprintf( buf, ".%03d", st.wMilliseconds );
			strcat( str, buf );
		}
	#else
		time_t t = time( 0 );
		struct tm *timeinfo = localtime( &t );
		sprintf( str, "%d%02d%02d %02d:%02d:%02d",
				timeinfo->tm_year + 1900, 
				timeinfo->tm_mon + 1,
				timeinfo->tm_mday,
				timeinfo->tm_hour,
				timeinfo->tm_min,
				timeinfo->tm_sec 
		);
		if( includeMils ) {
			// @TODO
		}
	#endif
	return str;
}

char *zTimeGetUTCTimeStringNumeric( int includeMils ) {
	static char str[256];
#ifdef WIN32
	SYSTEMTIME st;
	GetSystemTime( &st );
	sprintf( str, "%d%02d%02d %02d:%02d:%02d",
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond
			);
	if( includeMils ) {
		char buf[8];
		sprintf( buf, ".%03d", st.wMilliseconds );
		strcat( str, buf );
	}
#else
	time_t t = time( 0 );
	struct tm *timeinfo = gmtime( &t );
	sprintf( str, "%d%02d%02d %02d:%02d:%02d",
			timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1,
			timeinfo->tm_mday,
			timeinfo->tm_hour,
			timeinfo->tm_min,
			timeinfo->tm_sec
			);
	if( includeMils ) {
		// @TODO
	}
#endif
	return str;
}

ZTimeUTC zTimeUTC() {
	return (ZTimeUTC)time( 0 );
}


#ifdef SELF_TEST

#include "stdio.h"

void main() {
	ZTimer timer0( 2.0 );
	ZTimer timer1( 0.5 );
	ZTimer timer2;
	ZTimer timer3( 0.1 );

	while( !timer0.isDone() ) {
		zTimeTick();

		if( timer3.isDone() ) {
			printf( "now=%lf\n", zTime-zTimeStart );
			timer3.start( 0.1 );
		}

		zTimeSleepMils( 10 );

		if( timer1.isDone() ) {
			printf( "Pausing time\n" );
			zTimePause();
			timer1.stop();
			timer2.start( 0.5 );
		}

		if( timer2.isDone() ) {
			printf( "Resuming time\n" );
			zTimeResume();
			timer2.stop();
		}
	}
}
#endif
