// @ZBS {
//		*MODULE_OWNER_NAME zglmatlight
// }

#ifndef ZGLMATLIGHT_H
#define ZGLMATLIGHT_H

#include "zhashtable.h"

struct ZGLLight {
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float pos[4];
	float dir[4];
		// This one is really an extra copy which is stuffed into
		// the glLight( GL_POSITION ) when it is a directional light
	float spotDir[3];
	float spotExponent;
	float spotCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	int active;
	int which;
		// Set during setGL.  It can change between
		// calls to zglLightReset()

	void resetToDefaults() {
		ambient[0] = 0.f;
		ambient[1] = 0.f;
		ambient[2] = 0.f;
		ambient[3] = 1.f;

		diffuse[0] = 0.f;
		diffuse[1] = 0.f;
		diffuse[2] = 0.f;
		diffuse[3] = 1.f;

		specular[0] = 0.f;
		specular[1] = 0.f;
		specular[2] = 0.f;
		specular[3] = 1.f;

		pos[0] = 0.f;
		pos[1] = 0.f;
		pos[2] = 1.f;
		pos[3] = 0.f;

		dir[0] = 0.f;
		dir[1] = 0.f;
		dir[2] = 1.f;
		dir[3] = 0.f;

		spotDir[0] = 0.f;
		spotDir[1] = 0.f;
		spotDir[2] = -1.f;

		spotExponent = 0;
		spotCutoff = 180;
		constantAttenuation = 1.f;
		linearAttenuation = 0.f;
		quadraticAttenuation = 0.f;
	}

	void setGL();
	void setLightNumber( int i );

	ZGLLight() {
		resetToDefaults();
		active = 0;
		setLightNumber( 0 );
	}

	int isPositional() {
		return pos[3] != 0.0 && spotCutoff == 180.f;
	}
	int isDirectional() {
		return pos[3] == 0.0 && spotCutoff == 180.f;
	}
	int isSpot() {
		return spotCutoff != 180.f;
	}
	void makePositional() {
		pos[3] = 1.0;
		spotCutoff = 180.f;
	}
	void makeDirectional() {
		pos[3] = 0.0;
		spotCutoff = 180.f;
	}
	void makeSpot() {
		pos[3] = 1.0;
		spotCutoff = 90.f;
	}
};


struct ZGLMaterial {
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float emission[4];
	float shininess;
	int side;
	int active;

	void resetToDefaults() {
		ambient[0] = 0.2f;
		ambient[1] = 0.2f;
		ambient[2] = 0.2f;
		ambient[3] = 1.f;

		diffuse[0] = 0.8f;
		diffuse[1] = 0.8f;
		diffuse[2] = 0.8f;
		diffuse[3] = 1.f;

		specular[0] = 0.f;
		specular[1] = 0.f;
		specular[2] = 0.f;
		specular[3] = 1.f;

		emission[0] = 0.f;
		emission[1] = 0.f;
		emission[2] = 0.f;
		emission[3] = 1.f;

		shininess = 0.f;
	}

	void setGL();

	ZGLMaterial() {
		resetToDefaults();
		side = GL_FRONT_AND_BACK;
		active = 0;
	}
};

// Interface
//============================================================================

void zglLightReset();
	// Call this before rending any lights each frame.  
	// It resets the counters used to determining lights counts

ZGLLight *zglLightSetup( char *name );
	// Turns on the given light.  Gives it the next light number.
	// If you need to do more sophisticated things like turning
	// a single light on and off during a frame, use the 
	// zglLightDeclare interface to give it a pointer to a light
	// that you hold a copy of so you can do whatever you want
	// Returns the light that it found

void zglLightDeclare( char *name, ZGLLight *light );
	// Declare a pointer to a light.  Use this when you
	// need access to the light data directly.  For example,
	// if you wanted to computer the position dynamically.
	// Otherwise, for static lights you just have to
	// add them to the light file which you load.

ZGLLight *zglLightFetch( char *name );
	// Fetches a pointer to the light

void zglLightSaveAll( char *filename );
	// Save all the lights into the file

void zglLightLoad( char *filename );
	// Load all the lights from the file

extern ZHashTable zglLightHash;
	// The hash table of the lights

// All the material function work just like the light one described above:
ZGLMaterial *zglMaterialFetch( char *name );
void zglMaterialReset();
void zglMaterialSetup( char *name );
void zglMaterialDeclare( char *name, ZGLMaterial *material );
void zglMaterialSaveAll( char *filename );
void zglMaterialLoad( char *filename );
extern ZHashTable zglMaterialHash;

#endif


