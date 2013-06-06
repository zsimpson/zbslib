// @ZBS {
//		*MODULE_OWNER_NAME plot
// }

#ifndef PLOT_H
#define PLOT_H

void plotUsingViewpointGetBoundFullscreen( float &l, float &t, float &r, float &b );
void plotUsingViewpointGetPixelDXDY( float &dx, float &dy );
void plotUsingViewpointGridlinesFullscreen();
void plotGridlines( float x0, float y0, float x1, float y1, float xd, float yd );
void plotGridlinesLinLog( double x0, double y0, double x1, double y1, double xd, double yd, int logx, int logy, int skipMinor, int skipXLabels, int skipYLabels );
void plotGridlines3d( float x0, float y0, float z0, float x1, float y1, float z1, float xd, float yd, float zd );

void plotGridMajorColor3f( float r, float g, float b );
void plotGridMajorColor3ub( unsigned char r, unsigned char g, unsigned char b );
void plotGridMinorColor3f( float r, float g, float b );
void plotGridMinorColor3ub( unsigned char r, unsigned char g, unsigned char b );
void plotGridAxisColor3f( float r, float g, float b );
void plotGridAxisColor3ub( unsigned char r, unsigned char g, unsigned char b );
void plotGridLegendColor3ub( unsigned char r, unsigned char g, unsigned char b );


#endif


