#error( "Deprecated" )

// @ZBS {
//		*MODULE_OWNER_NAME ztrackball
// }

#ifndef ZTRACKBALL_H
#define ZTRACKBALL_H

#include "zmsg.h"
#include "zvec.h"

extern FMat4 zTrackballMat;

void zTrackballSetupView();
void zTrackballHandleMsg( ZMsg *msg );
void zTrackballReset();

int zTrackballSave( char *filename );
int zTrackballLoad( char *filename );
	// You can also send a ZTrackball_save and Trackball_load with filename in the message queue

#endif
