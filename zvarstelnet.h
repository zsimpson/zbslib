// @ZBS {
//		*MODULE_OWNER_NAME zvarstelnet
// }

#ifndef ZVARSTELNET_H
#define ZVARSTELNET_H

void zVarsTelnetCommand( char *cmd );
int zVarsTelnetHandler( char *line, class ZTelnetServerSession *s );

#endif