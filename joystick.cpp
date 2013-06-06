#include "joystick.h"
#include "tmsg.h"
#include "string.h"

//#include "windows.h"
// The following lines duplicate the code needed from windows.h
// In case of a compile problem, comment out the following and
// uncomment the above includes.
extern "C" {
struct JOYINFO {
    unsigned intwXpos;                 
    unsigned intwYpos;                 
    unsigned intwZpos;                 
    unsigned intwButtons;              
};
struct JOYINFOEX {
    unsigned long dwSize;		 
    unsigned long dwFlags;		 
    unsigned long dwXpos;                
    unsigned long dwYpos;                
    unsigned long dwZpos;                
    unsigned long dwRpos;		 
    unsigned long dwUpos;		 
    unsigned long dwVpos;		 
    unsigned long dwButtons;             
    unsigned long dwButtonNumber;        
    unsigned long dwPOV;                 
    unsigned long dwReserved1;		 
    unsigned long dwReserved2;		 
};
typedef unsigned int MMRESULT;   
__declspec(dllimport) MMRESULT __stdcall joyGetNumDevs(void);
__declspec(dllimport) MMRESULT __stdcall joyGetPos(unsigned int uJoyID, JOYINFO *pji);
__declspec(dllimport) MMRESULT __stdcall joyGetPosEx(unsigned int uJoyID, JOYINFOEX *pji);
#define JOYSTICKID1 0
#define JOYSTICKID2 1
#define JOYERR_NOERROR (0)
#define JOY_RETURNX		0x00000001l
#define JOY_RETURNY		0x00000002l
#define JOY_RETURNZ		0x00000004l
#define JOY_RETURNR		0x00000008l
#define JOY_RETURNU		0x00000010l
#define JOY_RETURNV		0x00000020l
#define JOY_RETURNPOV		0x00000040l
#define JOY_RETURNBUTTONS	0x00000080l
#define JOY_RETURNCENTERED	0x00000400l
#define JOY_RETURNALL		(JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNPOV | JOY_RETURNBUTTONS)
}
// End Windows.h duplication

// Local
int joy = 0;
int joyWhichDevice = -1;

// Public
float joyX = 0.f;
float joyY = 0.f;
float joyZ = 0.f;
float joyR = 0.f;
float joyPOV = 0.f;
int joyButtons[32] = {0,};

float joyXLast = 0.f;
float joyYLast = 0.f;
float joyZLast = 0.f;
float joyRLast = 0.f;
float joyPOVLast = 0.f;
int joyButtonsLast[32] = {0,};

int joyInit() {
	joy = 0;
	JOYINFO joyinfo; 
	int numJoysticks = joyGetNumDevs();
	if( numJoysticks == 0 ) {
		return 0; 
	}

	int joy1Attached = joyGetPos(JOYSTICKID1,&joyinfo) == JOYERR_NOERROR; 
	int joy2Attached = numJoysticks == 2 && joyGetPos(JOYSTICKID2,&joyinfo) == JOYERR_NOERROR; 

	// DECIDE which joystick to use
	if( joy1Attached || joy2Attached ) {
		joyWhichDevice = joy1Attached ? JOYSTICKID1 : JOYSTICKID2;
		joy = 1;
		return 1;
	}
	return 0; 
}

int joySample() {
	if( joyWhichDevice != -1 ) {
		JOYINFOEX joyInfoEx;
		joyInfoEx.dwSize = sizeof(joyInfoEx);
		joyInfoEx.dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED;
		MMRESULT ret = joyGetPosEx( joyWhichDevice, &joyInfoEx );
		if( ret == JOYERR_NOERROR ) {
			// SAVE the old values
			joyXLast = joyX;
			joyYLast = joyY;
			joyZLast = joyZ;
			joyRLast = joyR;
			joyPOVLast = joyPOV;
			memcpy( joyButtonsLast, joyButtons, sizeof(joyButtonsLast) );

			// NORMALIZE the new values
			joyX = (float)joyInfoEx.dwXpos / 65536.f;
			joyY = (float)joyInfoEx.dwYpos / 65536.f;
			joyZ = (float)joyInfoEx.dwZpos / 65536.f;
			joyR = (float)joyInfoEx.dwRpos / 65536.f;
			for( int i=0; i<32; i++ ) {
				joyButtons[i] = joyInfoEx.dwButtons & (1 << i);
			}
			joyPOV = (float)joyInfoEx.dwPOV / 36000.f;

			// SCAN for button changes and generate events on state changes
			for( i=0; i<32; i++ ) {
				if( joyButtonsLast[i] != joyButtons[i] ) {
					if( joyButtons[i] ) {
						tMsgQueue( "type=JoyButtonPress which=%d", i );
					}
					else {
						tMsgQueue( "type=JoyButtonRelease which=%d", i );
					}
				}
			}
		}
		return 1;
	}
	return 0;
}
 
