// ZBSDISCLAIM

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "profile.h"
#ifdef PROFILE_REPORT_REALTIME
	#include "fulltime.h"
#endif

ProfileData  profileData [PROFILE_MAX+PROFILE_EXTRA];
ProfileData *profileDataLast = &profileData[PROFILE_MAX+PROFILE_EXTRA-1];
int          profileStack [PROFILE_STACK_SIZE];
int         *profileStackTop = &profileStack[0];
__int64      profileStartTime = 0;
__int64      profileStopTime  = 0;
int          profileStartFlag = 0;
int         *profileStackTLimit = &profileStack[PROFILE_STACK_SIZE-1];
int          profileStartFrame = 0;
int          profileAdvanceState = 0;
int          profileState = 0;
#ifdef PROFILE_REPORT_REALTIME
	FullTime     profileStartRealTime;
#endif

void profileStartBlockFunc() {
#ifdef ZPROFILE
	// Make eax myIP
	__asm mov eax, [ebp+4]

	// Make ecx parentIP
	__asm mov ebx, DWORD PTR [profileStackTop]
	__asm mov ecx, [ebx]			

	// Push myIP on the stack
	__asm add ebx, 4				
	__asm mov DWORD PTR [ebx], eax
	__asm mov DWORD PTR [profileStackTop], ebx

	#ifdef PROFILE_SAFTY_ASSERT
	// Check for stack overflow
	__asm mov edx, DWORD PTR [profileStackTLimit]
	__asm cmp edx, ebx
	__asm jg STACK_OK
	__asm int 3
	#endif

STACK_OK:
	// Hash value is parentIP + myIP & MASK
	__asm mov ebx, ecx
	__asm add ebx, eax
	__asm and ebx, PROFILE_MASK
	__asm shl ebx, 5

CHECK_BUCKET:
	// Make edx p->myIP
	__asm mov edx, DWORD PTR [profileData + ebx + 20]

	// If p->myIP == 0 then found a bucket
	__asm cmp edx, 0
	__asm jz FOUND_BUCKET

	// If p->myIP != myIP then not found a bucket
	__asm cmp edx, eax
	__asm jnz NOT_FOUND_BUCKET

	// Or if p->parentIP != parentIP then not found a bucket
	__asm cmp DWORD PTR [profileData + ebx + 24], ecx
	__asm jnz NOT_FOUND_BUCKET

FOUND_BUCKET:
	__asm mov DWORD PTR [profileData + ebx + 20], eax
	__asm mov DWORD PTR [profileData + ebx + 24], ecx
	__asm inc DWORD PTR [profileData + ebx + 28]
	__asm rdtsc
	__asm mov [profileData + ebx + 8], eax
	__asm mov [profileData + ebx + 12], edx

	return;

NOT_FOUND_BUCKET:
	// Increment the bucket pointer
	__asm add ebx, 32
	__asm and ebx, (PROFILE_MASK<<5)
	__asm jmp CHECK_BUCKET

/* C Code equivilent
	int myIP;
	__asm mov eax, [ebp+4]
	__asm mov DWORD PTR [myIP], eax;
	int parentIP = *profileStackTop;

	profileStackTop++;
	if( profileStackTop > profileStackTLimit ) {
		__asm int 3;
	}
	*profileStackTop = myIP;

	int node = (myIP + parentIP) & PROFILE_MASK;
	ProfileData *p = &profileData[node];
	while(1) {
		if(
			p->myIP == 0 ||
			(p->myIP == myIP && p->parentIP == parentIP)
		) {
			p->myIP = myIP;
			p->parentIP = parentIP;
			p->count++;
			__int64 startTime;
			__asm {
				rdtsc
				mov [DWORD PTR startTime], eax
				mov [DWORD PTR startTime+4], edx
			}
			p->startTime = startTime;
			break;
		}
		p++;
		if( p > profileDataLast ) {
			__asm int 3;
		}
	}
*/
	#endif
}

void profileStopBlockFunc() {
#ifdef ZPROFILE
	// Read time stamp counter first
	__asm rdtsc

	// Pop myIP from profile stack into edi and ebx
	__asm mov ecx, [profileStackTop]
	__asm mov edi, [ecx]
	__asm mov ebx, edi
	__asm sub ecx, 4
	__asm mov DWORD PTR [profileStackTop], ecx

	// Move parentIP into esi
	__asm mov esi, [ecx]
	
	// Hash value is parentIP + myIP & MASK
	__asm add ebx, esi
	__asm and ebx, PROFILE_MASK
	__asm shl ebx, 5

CHECK_BUCKET:
	__asm cmp DWORD PTR [profileData + ebx + 20], edi
	__asm jne NOT_FOUND_BUCKET

	__asm cmp DWORD PTR [profileData + ebx + 24], esi
	__asm jne NOT_FOUND_BUCKET

	// Calc elapsed time
	__asm sub eax, [profileData + ebx + 8]
	__asm sbb edx, [profileData + ebx + 12]
	
	// Add to time accumulator
	__asm add DWORD PTR [profileData + ebx], eax
	__asm adc DWORD PTR [profileData + ebx + 4], edx

	return;

NOT_FOUND_BUCKET:
	__asm add ebx, 32
	__asm and ebx, (PROFILE_MASK<<5)
	__asm jmp CHECK_BUCKET

/* C Code equivilent
	__int64 stopTime;
	__asm {
		rdtsc
		mov [DWORD PTR stopTime], eax
		mov [DWORD PTR stopTime+4], edx
	}

	int myIP = *profileStackTop;
	profileStackTop--;
	int parentIP = *profileStackTop;

	int node = (myIP + parentIP) & PROFILE_MASK;
	ProfileData *p = &profileData[node];

	while( p->myIP != myIP || p->parentIP != parentIP ) {
		p++;
		if( p > profileDataLast ) {
			__asm int 3;
		}
	}

	p->accumTime += stopTime - p->startTime;
*/
#endif
}

void profileStartBlockNoParentFunc() {
#ifdef ZPROFILE
	// Make eax is myIP
	__asm mov eax, [ebp+4]

	// Push myIP on the stack
	__asm mov ebx, DWORD PTR [profileStackTop]
	__asm add ebx, 4				
	__asm mov DWORD PTR [ebx], eax
	__asm mov DWORD PTR [profileStackTop], ebx

	#ifdef PROFILE_SAFTY_ASSERT
	// Check for stack overflow
	__asm mov edx, DWORD PTR [profileStackTLimit]
	__asm cmp edx, ebx
	__asm jg STACK_OK
	__asm int 3
	#endif

STACK_OK:
	// Hash value is myIP & MASK
	__asm mov ebx, eax
	__asm and ebx, PROFILE_MASK
	__asm shl ebx, 5

CHECK_BUCKET:
	// Make edx p->myIP
	__asm mov edx, DWORD PTR [profileData + ebx + 20]

	// If p->myIP == 0 then found a bucket
	__asm cmp edx, 0
	__asm jz FOUND_BUCKET

	// If p->myIP != myIP then not found a bucket
	__asm cmp edx, eax
	__asm jnz NOT_FOUND_BUCKET

	// Or if p->parentIP then not found a bucket
	__asm cmp DWORD PTR [profileData + ebx + 24], 0
	__asm jnz NOT_FOUND_BUCKET

FOUND_BUCKET:
	__asm mov DWORD PTR [profileData + ebx + 20], eax
	__asm mov DWORD PTR [profileData + ebx + 24], 0
	__asm inc DWORD PTR [profileData + ebx + 28]
	__asm rdtsc
	__asm mov [profileData + ebx + 8], eax
	__asm mov [profileData + ebx + 12], edx

	return;

NOT_FOUND_BUCKET:
	// Increment the bucket pointer
	__asm add ebx, 32
	__asm and ebx, (PROFILE_MASK<<5)
	__asm jmp CHECK_BUCKET

/* C Code equivilent
	int myIP;
	__asm mov eax, [ebp+4]
	__asm mov DWORD PTR [myIP], eax;

	profileStackTop++;
	if( profileStackTop > profileStackTLimit ) {
		__asm int 3;
	}
	*profileStackTop = myIP;

	int node = myIP & PROFILE_MASK;
	ProfileData *p = &profileData[node];
	while(1) {
		if(
			p->myIP == 0 ||
			(p->myIP == myIP && p->parentIP == 0)
		) {
			p->myIP = myIP;
			p->parentIP = 0;
			p->count++;
			__int64 startTime;
			__asm {
				rdtsc
				mov [DWORD PTR startTime], eax
				mov [DWORD PTR startTime+4], edx
			}
			p->startTime = startTime;
			break;
		}
		p++;
		if( p > profileDataLast ) {
			__asm int 3;
		}
	}
*/
#endif
}

void profileStopBlockNoParentFunc() {
#ifdef ZPROFILE
	// Read time stamp counter first
	__asm rdtsc

	// Pop myIP from profile stack into edi and ebx
	__asm mov ecx, [profileStackTop]
	__asm mov edi, [ecx]
	__asm mov ebx, edi
	__asm sub ecx, 4
	__asm mov DWORD PTR [profileStackTop], ecx

	// Hash value is myIP & MASK
	__asm and ebx, PROFILE_MASK
	__asm shl ebx, 5

CHECK_BUCKET:
	__asm cmp DWORD PTR [profileData + ebx + 20], edi
	__asm jne NOT_FOUND_BUCKET

	__asm cmp DWORD PTR [profileData + ebx + 24], 0
	__asm jne NOT_FOUND_BUCKET

	// Calc elapsed time
	__asm sub eax, [profileData + ebx + 8]
	__asm sbb edx, [profileData + ebx + 12]
	
	// Add to time accumulator
	__asm add DWORD PTR [profileData + ebx], eax
	__asm adc DWORD PTR [profileData + ebx + 4], edx

	return;

NOT_FOUND_BUCKET:
	__asm add ebx, 32
	__asm and ebx, (PROFILE_MASK<<5)
	__asm jmp CHECK_BUCKET

/* C Code equivilent
	__int64 stopTime;
	__asm {
		rdtsc
		mov [DWORD PTR stopTime], eax
		mov [DWORD PTR stopTime+4], edx
	}

	int myIP = *profileStackTop;
	profileStackTop--;

	int node = (myIP) & PROFILE_MASK;
	ProfileData *p = &profileData[node];

	while( p->myIP != myIP || p->parentIP != parentIP ) {
		p++;
		if( p > profileDataLast ) {
			__asm int 3;
		}
	}

	p->accumTime += stopTime - p->startTime;
*/
#endif
}

void profileStart() {
#ifdef ZPROFILE
	memset( profileData, 0, sizeof(ProfileData) * PROFILE_MAX+PROFILE_EXTRA );
	memset( profileStack, 0, sizeof(int) * PROFILE_STACK_SIZE );
	profileStackTop = &profileStack[0];
	profileStackTLimit = &profileStack[PROFILE_STACK_SIZE-1];

	for( ProfileRange *c = ProfileRange::head; c; c=c->next ) {
		if( c->data ) {
			memset( c->data, 0, sizeof(ProfileData) * c->count );
		}
	}

	profileStopTime = 0;
	__asm {
		rdtsc
		mov [DWORD PTR profileStartTime], eax
		mov [DWORD PTR profileStartTime+4], edx
	}
	#ifdef PROFILE_REPORT_REALTIME
		extern int globalFrameNumber;
		profileStartFrame = globalFrameNumber;
		profileStartRealTime = frameTime;
	#endif
#endif
}

void profileDump() {
#ifdef ZPROFILE
	__asm {
		rdtsc
		mov [DWORD PTR profileStopTime], eax
		mov [DWORD PTR profileStopTime+4], edx
	}

	char *filename = "profile.txt";

	FILE *profOut = fopen( filename, "w+b" );
	if( !profOut ) {
		return;
	}

	fprintf( profOut, "0 0 1 %20.0f\n", (double)(profileStopTime - profileStartTime) );

	for( int i=0; i<PROFILE_MAX+PROFILE_EXTRA; i++ ) {
		if( profileData[i].startTime ) {
			fprintf( profOut,
				"%X %X %d %20.0f\n",
				profileData[i].myIP
				#ifdef PROFILE_FUNC
				-5
				#endif
				,
				profileData[i].parentIP
				#ifdef PROFILE_FUNC
				 - (profileData[i].parentIP?5:0)
				#endif
				,
				profileData[i].count,
				(double)profileData[i].accumTime
			);
		}
	}

	for( ProfileRange *c = ProfileRange::head; c; c=c->next ) {
		fprintf( profOut, "-------------------------------------\n%s\n", c->desc );

		// Sum up all times in this range
		double global = (double)(profileStopTime - profileStartTime);
		double total = 0.0;
		for( int i=0; i<c->count; i++ ) {
			if( c->data[i].count ) {
				total += (double)c->data[i].accumTime;
			}
		}

		for( i=0; i<c->count; i++ ) {
			if( c->data[i].count ) {
				fprintf(
					profOut, "%d %d %20.0f group=%3.1f%% global=%3.1f%% %s\n",
					i,
					c->data[i].count,
					(double)c->data[i].accumTime, 
					((double)c->data[i].accumTime / total) * 100.0,
					((double)c->data[i].accumTime / global) * 100.0,
					c->strings ? c->strings[i] : ""
				);
			}
		}
	}

	fprintf( profOut, "=========================================\n" );

	#ifdef PROFILE_REPORT_REALTIME
		fprintf( profOut, "FPS = %5.2f\n",
			(float)(((float)globalFrameNumber-(float)profileStartFrame) / (frameTime.time - profileStartRealTime.time))
		);
	#endif

	fclose (profOut);
#endif
}


// ProfileRange
//----------------------------------------------------------------------

#pragma optimize ("awg", off)
ProfileRange *ProfileRange::head = (ProfileRange *)0;

ProfileRange::ProfileRange( int _count, char *_desc, char **_strings ) {
#ifdef ZPROFILE
	count = _count;
	desc = _desc;
	strings = _strings;
	data = (ProfileData *)malloc( count * sizeof(ProfileData) );
	memset( data, 0, count * sizeof(ProfileData) );
	next = ProfileRange::head;
	ProfileRange::head = this;
#endif
}

ProfileRange::~ProfileRange() {
#ifdef ZPROFILE
	if( data ) {
		free( data );
		data = (ProfileData *)0;
	}
#endif
}

void ProfileRange::start( int i ) {
	#ifdef ZPROFILE
		#ifdef PROFILE_SAFTY_ASSERT
			if( 0 > i || i >= count ) {
				__asm int 03;
			}
		#endif
		__int64 temp;
		__asm {
			rdtsc
			mov [DWORD PTR temp], eax
			mov [DWORD PTR temp+4], edx
		}
		data[i].startTime = temp;
		data[i].count++;
	#endif
}

void ProfileRange::stop( int i ) {
	#ifdef ZPROFILE
		__int64 temp;
		__asm {
			rdtsc
			mov [DWORD PTR temp], eax
			mov [DWORD PTR temp+4], edx
		}
		data[i].accumTime += temp - data[i].startTime;
	#endif
}
#pragma optimize ("", on)
