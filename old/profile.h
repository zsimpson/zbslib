// (c) 2000 by Zachary Booth Simpson
// This code may be freely used, modified, and distributed without any license
// as long as the original author is noted in the source file and all
// changes are made clear disclaimer using a "MODIFIED VERSION" disclaimer.
// There is NO warranty and the original author may not be held liable for any damages.
// http://www.totempole.net
//
// Things to consider when using the profiler...
// 1) If you use some kind of UI to activate / dump the
//    profiler, be sure that you start/sop in a well known
//    place in the main loop.  For example, in the event
//    handler, set a flag "startProf" and then at the top of the
//    main loop, add a "if(startProf) { profileStart(); startProf=0; }"
//    Do the same for the dumpProfiler() and you will ensure that all
//    tests are always inluding an entire main loop interation and
//    no partials.
// 2) The compiler optimizers may messup the profiler.  If the
//    optimizer thinks that it can skip over a Q call, for example,
//    or in some other way optimizes out part of a P/Q combination,
//    then *all* results will be screwed.  If this happens, either
//    turn off the optimizer for that block of code (#pragma), or reduce
//    the number of P&Qs to get it stable.
//    You can use the macros PROFILE_OPTIMIZE_BEGIN and PROFILE_OPTIMIZE_END
//    around code to turn off the appropriate optimizations
//    Another symptom of optimizers is that they convert some functions
//    to inline and thus the perl script debug sysmbols that the perl
//    script uses can not resolve the addresses.
// 3) Be sure that the program being profiled is the primary application.
//    Either up the process priority to high and/or kill all other apps.
// 4) If the macro ZPROFILE is not defined, all profiling code is NULL
// 5) The perl script which generates human reable summaries
//    operates by parsing debug information from a COFF executable.
//    Thus, you must enable COFF symbols in the Link options.  You
//    May select "both" MS and COFF symbols so that you can
//    continue debugging with profiling on.  However, you will not be
//    able to incrementally link.  (Besides, incremental linking has a 
//    performance hit, so its better to have it turned off while
//    profiling anyway.
// 6) The profiler uses the Pentium rdtsc instruction which is not
//    implemented on some Pentium clones such as the Cyrix.  It
//    wil generate an exception on such machines.
// 7) The profiler uses a stack to track state.  If this stack is corrupted
//    (for example you have mismatched P's and Q's) then all data will
//    be wrong.  You can check the stack with the PROFILE_CHECK_STACK
//    macro.  You can reset the stack at a known good point with PROFILE_CHECK_RESET


#ifndef PROFILE_H
#define PROFILE_H

	#define PROFILE_SAFTY_ASSERT
		// This macro tells it to do extra stack overflow checking
		// On by default
	#define PROFILE_FUNC
		// This macro tells it to use function calls as opposed to inline code
		// On by default
	//#define PROFILE_REPORT_REALTIME
		// This macro tells it to report realtime summary in report.
		// This can only be used in conjunction with the FillTime class
		// and its associated globalVariables.
		// Off by default

	struct ProfileData {
		__int64 accumTime;
		__int64 startTime;
		int alignment;
		int myIP;
		int parentIP;
		int count;
	};

	#define cpuid __asm _emit 0fh __asm _emit 0a2h
	#define rdtsc __asm _emit 0fh __asm _emit 031h
	#define push_eip __asm _emit 0e8h __asm _emit 00h __asm _emit 00h __asm _emit 00h __asm _emit 00h

	#define PROFILE_MAX (0x200)
	#define PROFILE_EXTRA (0x10)
	#define PROFILE_MASK (PROFILE_MAX-1)
	#define PROFILE_STACK_SIZE (1000)

	#ifdef ZPROFILE
		#define PROFILE_OPTIMIZE_BEGIN \
			#ifdef ZPROFILE \
			#pragma optimize ("awg", off) \
			#endif
		#define PROFILE_OPTIMIZE_END \
			#ifdef ZPROFILE \
			#pragma optimize ("", on) \
			#endif

		extern ProfileData profileData [PROFILE_MAX+PROFILE_EXTRA];
		extern ProfileData *profileDataLast;
		extern int profileStack [PROFILE_STACK_SIZE];
		extern int *profileStackTop;
		extern int *profileStackTLimit;

		#define PROFILE_CHECK_STACK \
			static int *_profileStackTop = (int *)0; \
			static int  _profileStackTopValue = 0; \
			if( _profileStackTop == (int *)0 ) { \
				_profileStackTop = profileStackTop; \
				_profileStackTopValue = *profileStackTop; \
			} \
			else if( \
				_profileStackTop != profileStackTop || \
				_profileStackTopValue != *profileStackTop \
			) { __asm int 3 } 

		#define PROFILE_CHECK_STACK_RESET \
			_profileStackTop = (int *)0; \
			_profileStackTopValue = 0;


		#ifdef PROFILE_SAFTY_ASSERT
			// Slow mode with the safty assert

			#define profileStartBlock \
				__asm push eax \
				__asm push ebx \
				__asm push ecx \
				__asm push edx \
				__asm pushf \
				\
				__asm push_eip                                  /* ebx = eip */ \
				__asm pop ebx \
				__asm mov ecx, ebx                              /* ecx = myIP */ \
				__asm mov eax, [profileStackTop]                /* hashBucket#ebx = eip + *profileStackTop */ \
				__asm add ebx, [eax] \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				/* Assert that the parentIP and myIP are either zero */ \
				/* or the same as what we're putting into it */ \
				/* 1st, check if profData->myIp == 0, if so all is OK */ \
				__asm mov edx, [profileData + ebx + 20]         /* edx = (profileData + hashOffset#ebx)->myIp */ \
				__asm cmp edx, 0 \
				__asm jz 24 /* jump if zero to OK LABEL */ \
				\
				/* 2nd, compare profData->myIP to myIP#ecx */ \
				/* edx has profData->myIP, compare it to myIP#ecx */ \
				/* if all is ok now, edx will be 0 */ \
				__asm cmp edx, ecx \
				__asm jz 1 /* if OK, skip the exception */ \
				__asm int 03 \
				\
				/* 3rd, compare profData->parentIP to parentIP#[eax] */ \
				__asm mov edx, [profileData + ebx + 24] /* edx = (profileData + hashOffset#ebx)->parentIp */ \
				__asm cmp edx, [eax] \
				__asm jz 1 /* if OK, skip the exception */ \
				__asm int 03 \
				\
				/* OK LABEL: */ \
				__asm mov edx, [eax] \
				__asm mov [profileData + ebx + 24], edx \
				__asm add eax, 4                                /* profileStackTop ++ */ \
				__asm mov edx, DWORD PTR [profileStackTLimit]   /* check for stack overflow */ \
				__asm cmp edx, eax \
				__asm jns 1 \
				__asm int 3                                     /* stack overflow */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				__asm mov [eax], ecx                            /* Top of stack now new ip */ \
				__asm mov [profileData + ebx + 20], ecx         /* (profileData + hashOffset#ebx)->myIP = new ip */ \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp() */ \
				\
				__asm mov [profileData + ebx + 8], eax          /* (profileData + hashOffset#ebx)->startTime = edx:eax */ \
				__asm mov [profileData + ebx + 12], edx \
				\
				__asm inc DWORD PTR [profileData + ebx + 28]    /* (profileData + hashOffset#ebx)->count ++ */ \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ecx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStartBlockNoParent \
				__asm push eax \
				__asm push ebx \
				__asm push ecx \
				__asm push edx \
				__asm pushf \
				\
				__asm push_eip                                  /* ebx = eip */ \
				__asm pop ebx \
				__asm mov ecx, ebx                              /* ecx = myIP */ \
				__asm mov eax, [profileStackTop]                /* hashBucket#ebx = eip + *profileStackTop */ \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				/* Assert that the parentIP and myIP are either zero */ \
				/* or the same as what we're putting into it */ \
				/* 1st, check if profData->myIp == 0, if so all is OK */ \
				__asm mov edx, [profileData + ebx + 20]         /* edx = (profileData + hashOffset#ebx)->myIp */ \
				__asm cmp edx, 0 \
				__asm jz 25 /* jump if zero to OK LABEL */ \
				\
				/* 2nd, compare profData->myIP to myIP#ecx */ \
				/* edx has profData->myIP, compare it to myIP#ecx */ \
				/* if all is ok now, edx will be 0 */ \
				__asm cmp edx, ecx \
				__asm jz 1 /* if OK, skip the exception */ \
				__asm int 03 \
				\
				/* 3rd, compare profData->parentIP to parentIP#[eax] */ \
				__asm mov edx, [profileData + ebx + 24] /* edx = (profileData + hashOffset#ebx)->parentIp */ \
				__asm cmp edx, 0 \
				__asm jz 1 /* if OK, skip the exception */ \
				__asm int 03 \
				\
				/* OK LABEL: */ \
				__asm mov edx, 0 \
				__asm mov [profileData + ebx + 24], edx \
				__asm add eax, 4                                /* profileStackTop ++ */ \
				__asm mov edx, DWORD PTR [profileStackTLimit]   /* check for stack overflow */ \
				__asm cmp edx, eax \
				__asm jns 1 \
				__asm int 3                                     /* stack overflow */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				__asm mov [eax], ecx                            /* Top of stack now new ip */ \
				__asm mov [profileData + ebx + 20], ecx         /* (profileData + hashOffset#ebx)->myIP = new ip */ \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp() */ \
				\
				__asm mov [profileData + ebx + 8], eax          /* (profileData + hashOffset#ebx)->startTime = edx:eax */ \
				__asm mov [profileData + ebx + 12], edx \
				\
				__asm inc DWORD PTR [profileData + ebx + 28]    /* (profileData + hashOffset#ebx)->count ++ */ \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ecx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStopBlock \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm mov eax, [profileStackTop]                /* currentEIP#ebx = *profileStackTop */ \
				__asm mov ebx, [eax] \
				__asm sub eax, 4  								/* profileStackTop -- */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				\
				__asm mov edx, [eax]                            /* prevParent#edx = *(profileStackTop-1) */ \
				__asm add ebx, edx                              /* hashBucket#ebx = thisEIP + prevParent#edx */ \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp(); */ \
				\
				__asm sub eax, [profileData + ebx + 8]          /* edx:eax -= (profileData + hashOffset#ebx)->startTime; */ \
				__asm sbb edx, [profileData + ebx + 12] \
				__asm add DWORD PTR [profileData + ebx], eax    /* (profileData + hashOffset#ebx)->accumTime += edx:eax */ \
				__asm adc DWORD PTR [profileData + ebx + 4], edx \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStopBlockNoParent \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm mov eax, [profileStackTop]                /* currentEIP#ebx = *profileStackTop */ \
				__asm mov ebx, [eax] \
				__asm sub eax, 4  								/* profileStackTop -- */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp(); */ \
				\
				__asm sub eax, [profileData + ebx + 8]          /* edx:eax -= (profileData + hashOffset#ebx)->startTime; */ \
				__asm sbb edx, [profileData + ebx + 12] \
				__asm add DWORD PTR [profileData + ebx], eax    /* (profileData + hashOffset#ebx)->accumTime += edx:eax */ \
				__asm adc DWORD PTR [profileData + ebx + 4], edx \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

		#else
			// Fast mode without the safty assert

			#define profileStartBlock \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm push_eip                                  /* ebx = eip */ \
				__asm pop ebx \
				__asm push ebx                                  /* store ebx for later retrieval */ \
				__asm mov eax, [profileStackTop]                /* hashBucket#ebx = eip + *profileStackTop */ \
				__asm add ebx, [eax] \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm mov edx, [eax]                            /* (profileData + hashOffset#ebx)->parentIP = *profileStackTop via edx */ \
				__asm mov [profileData + ebx + 24], edx \
				__asm add eax, 4                                /* profileStackTop ++ */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				__asm pop edx                                   /* Retrieve eip into edx */ \
				__asm mov [eax], edx                            /* Top of stack now new ip */ \
				__asm mov [profileData + ebx + 20], edx         /* (profileData + hashOffset#ebx)->myIP = new ip */ \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp() */ \
				\
				__asm mov [profileData + ebx + 8], eax          /* (profileData + hashOffset#ebx)->startTime = edx:eax */ \
				__asm mov [profileData + ebx + 12], edx \
				\
				__asm inc DWORD PTR [profileData + ebx + 28]    /* (profileData + hashOffset#ebx)->count ++ */ \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStartBlockNoParent \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm push_eip                                  /* ebx = eip */ \
				__asm pop ebx \
				__asm push ebx                                  /* store ebx for later retrieval */ \
				__asm mov eax, [profileStackTop]                /* hashBucket#ebx = eip + *profileStackTop */ \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm mov edx, 0                                /* (profileData + hashOffset#ebx)->parentIP = *profileStackTop via edx */ \
				__asm mov [profileData + ebx + 24], edx \
				__asm add eax, 4                                /* profileStackTop ++ */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				__asm pop edx                                   /* Retrieve eip into edx */ \
				__asm mov [eax], edx                            /* Top of stack now new ip */ \
				__asm mov [profileData + ebx + 20], edx         /* (profileData + hashOffset#ebx)->myIP = new ip */ \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp() */ \
				\
				__asm mov [profileData + ebx + 8], eax          /* (profileData + hashOffset#ebx)->startTime = edx:eax */ \
				__asm mov [profileData + ebx + 12], edx \
				\
				__asm inc DWORD PTR [profileData + ebx + 28]    /* (profileData + hashOffset#ebx)->count ++ */ \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStopBlock \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm mov eax, [profileStackTop]                /* currentEIP#ebx = *profileStackTop */ \
				__asm mov ebx, [eax] \
				__asm sub eax, 4  								/* profileStackTop -- */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				\
				__asm mov edx, [eax]                            /* prevParent#edx = *(profileStackTop-1) */ \
				__asm add ebx, edx                              /* hashBucket#ebx = thisEIP + prevParent#edx */ \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp(); */ \
				\
				__asm sub eax, [profileData + ebx + 8]          /* edx:eax -= (profileData + hashOffset#ebx)->startTime; */ \
				__asm sbb edx, [profileData + ebx + 12] \
				__asm add DWORD PTR [profileData + ebx], eax    /* (profileData + hashOffset#ebx)->accumTime += edx:eax */ \
				__asm adc DWORD PTR [profileData + ebx + 4], edx \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

			#define profileStopBlockNoParent \
				__asm push eax \
				__asm push ebx \
				__asm push edx \
				__asm pushf \
				\
				__asm mov eax, [profileStackTop]                /* currentEIP#ebx = *profileStackTop */ \
				__asm mov ebx, [eax] \
				__asm sub eax, 4  								/* profileStackTop -- */ \
				__asm mov DWORD PTR [profileStackTop], eax \
				\
				__asm and ebx, PROFILE_MASK                     /* hashOffset#ebx = (ebx & MASK) << 5 */ \
				__asm shl ebx, 5 \
				\
				__asm rdtsc                                     /* edx:eax = timeStamp(); */ \
				\
				__asm sub eax, [profileData + ebx + 8]          /* edx:eax -= (profileData + hashOffset#ebx)->startTime; */ \
				__asm sbb edx, [profileData + ebx + 12] \
				__asm add DWORD PTR [profileData + ebx], eax    /* (profileData + hashOffset#ebx)->accumTime += edx:eax */ \
				__asm adc DWORD PTR [profileData + ebx + 4], edx \
				\
				__asm popf \
				__asm pop edx \
				__asm pop ebx \
				__asm pop eax 

		#endif

		#pragma message ("Profile ON")
		#ifdef PROFILE_FUNC
			extern void profileStartBlockFunc();
			extern void profileStopBlockFunc();
			extern void profileStartBlockNoParentFunc();
			extern void profileStopBlockNoParentFunc();
			#define P profileStartBlockFunc();
			#define Q profileStopBlockFunc();
			#define P0 profileStartBlockNoParentFunc();
			#define Q0 profileStopBlockNoParentFunc();
		#else
			#define P profileStartBlock;
			#define Q profileStopBlock;
			#define P0 profileStartBlockNoParent;
			#define Q0 profileStopBlockNoParent;
		#endif

	#else
		//#pragma message ("Profile OFF")
		#define P
		#define Q
		#define P0
		#define Q0
		#define PROFILE_CHECK_STACK
		#define PROFILE_CHECK_STACK_RESET
		#define PROFILE_OPTIMIZE_BEGIN
		#define PROFILE_OPTIMIZE_END
	#endif

	void profileStart();
	void profileDump();
	extern int profileAdvanceState;
	extern int profileState;

	struct ProfileRange {
		static ProfileRange *head;

		ProfileRange *next;
		ProfileData *data;
		int count;
		char *desc;
		char **strings;

		ProfileRange( int _count, char *_desc=(char *)0, char **_strings=(char **)0 );
		~ProfileRange();

		void start( int i );
		void stop( int i );
	};

#endif
