// @ZBS {
//		*MODULE_OWNER_NAME ztime
// }

#ifndef ZTIME_H
#define ZTIME_H

//
// This is a fully portable time class that stores time
// in seconds which as much accurarcy possible, measured in seconds.
// The time is in seconds since 1 Jan 1970, a.k.a. Universal Coordinated Time (UTC)
//
// There are two time domains: real-time and simulation time. Real time cannot be paused and is synchronized
// to the system clock. Simulation time is synchronized to real time by default, but can be paused and can be
// configured to be decoupled from real time.
//
// Choosing the correct time domain affords you the ability to pause and to use slow or fast motion without
// additional work.
//

typedef double ZTime;


// All of the following global variables are configured
// during each call to zTimeTick()

extern int zTimeFrameCount;
	// Incremented each time zTimeTick() is called (once per frame)

extern ZTime zTime;
	// The real time during the last call to zTimeTick(),

extern ZTime zTimeDT;
	// Delta time on the last frame, in real time

extern float zTimeDTF;
	// Delta time in float on the last frame, in real time

extern ZTime zTimeFPS;
	// The last calculated frames per second.
	// Always based on real time (as opposed to simulation time)

extern int zTimeAvgWindow;
	// The number of frames to averge when computing avgFPS

extern ZTime zTimeAvgFPS;
	// The running average of the last DTIME_FRAMES_TO_AVG 
	// Always based on real time (as opposed to simulation time)

extern ZTime zTimeStart;
	// Time that the zTime system was initialized in real time

extern ZTime zTimeSim;
	// The sim time during the last call to zTimeTick()

extern ZTime zTimeSimStart;
	// Time that the zTime system was initialized in sim time

extern ZTime zTimeSimDT;
	// Delta time on the last frame, in simulation time domain
	// NOTE: if simulation time mode is ZT_REAL, this is WRITTEN in zTimeTick().
	// If simulation time mode is ZT_FIXED, this value is READ in zTimeTick().
	// Thus, to use fixed delta time, set the mode to ZT_FIXED and set this value to whatever you
	// desire.

extern float zTimeSimDTF;
	// Delta time in float on the last frame, in simulation time domain
	// NOTE: if simulation time mode is ZT_REAL, this is WRITTEN in zTimeTick().
	// If simulation time mode is ZT_FIXED, this value is COPIED from zTimeSimDT in zTimeTick().

// Interface Functions
//------------------------------------------------------------------------
enum ZTimeMode
{
	ZT_REAL,  // simulation time synched to real time
	ZT_FIXED  // simulation time uses fixed delta
};

void zTimeSetSimMode (ZTimeMode mode);
	// Sets the simulation time mode.
	// NOTE: Calling this will reset the simulation time values.

ZTime zTimeNow();
	// The real time at this instant measured in seconds (arbitrary zero)

void zTimeTick();
	// Call this once a frame to set the above globals

void zTimeSleepSecs( int secs );
	// Platform independent sleep secs

void zTimeSleepMils( int mils );
	// Platform independent sleep milliseconds

// *** Should these be simulation time instead of real time?
void zTimePause();
	// Stops time and uses an accumulator to collect
	// the "stopped time".  Useful for non-synchronized modes.
	// This only modifies the "master" time calls such as
	// zTimeGetMasterTime() and dTimeMasterFrameTime

void zTimeResume();
	// Resumes time

int  zTimeIsPaused();
	// Query time status of paused time

void zTimeResetPaused();
	// Clears the paused accumulator

void zTimeGetLocalTime( int &hour, int &minute, int &second, int &millisecond );
	// Fetches local time, hour in 24, 0=midnight

char *zTimeGetLocalTimeString( int includeMils=1 );
	// Returns a complete time string, useful for logging
char *zTimeGetLocalTimeStringNumeric( int includeMils=1 );
	// Same, but numeric, more compact.


typedef unsigned int ZTimeUTC;
ZTimeUTC zTimeUTC();
	// Returns the UTC in seconds

// ZTimer Class
//------------------------------------------------------------------------

struct ZTimer {
	// The timer class constructs the timeDone variable to 0
	// A timer with timeDone of zero is considered NOT RUNNING and NOT DONE
	// Any other value of timeDone indicates that is RUNNING.  If
	// the current time is >= timeDone then this timer is considered
	// DONE (and still RUNNING)

	ZTime timeDone;

	void start(ZTime    countDown) { timeDone = zTimeNow() + countDown; }
	void start(float    countDown) { timeDone = zTimeNow() + (double)countDown; }
	void start(int      countDown) { timeDone = zTimeNow() + (double)countDown; }

	ZTimer() { stop(); }
	ZTimer(ZTime  countDown) { start(countDown); }
	ZTimer(float  countDown) { start(countDown); }
	ZTimer(int    countDown) { start(countDown); }

	int isDone() { return timeDone > 0.0 && zTimeNow() >= timeDone; }
	int isRunning() { return timeDone != 0.0; }
	void stop() { timeDone = 0.0; }
	ZTime remains() { if( timeDone == 0.0 ) return 0.0; else return( timeDone - zTimeNow() ); }
	float remainsF() { if( timeDone == 0.0 ) return 0.f; else return float( timeDone - zTimeNow() ); }
};

#endif
