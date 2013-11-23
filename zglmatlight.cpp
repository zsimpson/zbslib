// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for tracking the state of lights and materials in a very simplistic fashion
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zglmatlight.cpp zglmatlight.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "assert.h"
// MODULE includes:
#include "zglmatlight.h"
// ZBSLIB includes:
#include "zhashtable.h"
#include "zregexp.h"

// ZGLLight
////////////////////////////////////////////////////////////////////

void ZGLLight::setGL() {
	if( active ) {
		if( isDirectional() ) {
			glLightfv( which, GL_POSITION, dir );
		}
		else {
			glLightfv( which, GL_POSITION, pos );
		}
		glLightfv( which, GL_AMBIENT, ambient );
		glLightfv( which, GL_DIFFUSE, diffuse );
		glLightfv( which, GL_SPECULAR, specular );
		glLightfv( which, GL_CONSTANT_ATTENUATION, &constantAttenuation );
		glLightfv( which, GL_LINEAR_ATTENUATION, &linearAttenuation );
		glLightfv( which, GL_QUADRATIC_ATTENUATION, &quadraticAttenuation );
		glLightfv( which, GL_SPOT_DIRECTION, spotDir );
		glLightfv( which, GL_SPOT_EXPONENT, &spotExponent );
		glLightfv( which, GL_SPOT_CUTOFF, &spotCutoff );
	}
}

void ZGLLight::setLightNumber( int i ) {
	which = i + GL_LIGHT0;
}


// Light Interface
////////////////////////////////////////////////////////////////////

ZHashTable zglLightHash;
int zglLightCount = 0;

void zglLightReset() {
	zglLightCount = 0;
	glDisable( GL_LIGHT0 );
	glDisable( GL_LIGHT1 );
	glDisable( GL_LIGHT2 );
	glDisable( GL_LIGHT3 );
	glDisable( GL_LIGHT4 );
	glDisable( GL_LIGHT5 );
	glDisable( GL_LIGHT6 );
	glDisable( GL_LIGHT7 );
}

void zglLightDeclare( char *name, ZGLLight *light ) {
	zglLightHash.putP( name, light );
}

ZGLLight *zglLightFetch( char *name ) {
	return (ZGLLight *)zglLightHash.getp( name );
}

ZGLLight *zglLightSetup( char *name ) {
	// Find the light structure in the database
	ZGLLight *light = zglLightFetch( name );
	if( light ) {
		if( light->active ) {
			glEnable( GL_LIGHTING );
			light->which = GL_LIGHT0 + zglLightCount;
			zglLightCount++;
			glEnable( light->which );
			light->setGL();
		}
	}
	return light;
}

static int zglSort( const void *a, const void *b ) {
	#ifdef __USE_GNU
	#define stricmp strcasecmp
	#endif	
	return stricmp( (char *)a, (char *)b );
}

void zglLightSaveAll( char *filename ) {
	FILE *f = fopen( filename, "wt" );
	assert( f );

	char sorted[128][32];
	int count = 0, i;
	for( i=0; i<zglLightHash.size(); i++ ) {
		char *k = zglLightHash.getKey(i);
		if( k ) {
			strcpy( sorted[count++], k );
		}
	}
	qsort( sorted, count, 32, zglSort );

	for( i=0; i<count; i++ ) {
		char *k = sorted[i];
		ZGLLight *l = zglLightFetch( k );
		fprintf( f, ":%s\n", k );
		fprintf( f, "ambient: %f %f %f %f\n", l->ambient[0], l->ambient[1], l->ambient[2], l->ambient[3] );
		fprintf( f, "diffuse: %f %f %f %f\n", l->diffuse[0], l->diffuse[1], l->diffuse[2], l->diffuse[3] );
		fprintf( f, "specular: %f %f %f %f\n", l->specular[0], l->specular[1], l->specular[2], l->specular[3] );
		fprintf( f, "pos: %f %f %f %f\n", l->pos[0], l->pos[1], l->pos[2], l->pos[3] );
		fprintf( f, "dir: %f %f %f %f\n", l->dir[0], l->dir[1], l->dir[2], l->dir[3] );
		fprintf( f, "spotDir: %f %f %f\n", l->spotDir[0], l->spotDir[1], l->spotDir[2] );
		fprintf( f, "spotCutoff: %f\n", l->spotCutoff );
		fprintf( f, "spotExponent: %f\n", l->spotExponent );
		fprintf( f, "constantAttenuation: %f\n", l->constantAttenuation );
		fprintf( f, "linearAttenuation: %f\n", l->linearAttenuation );
		fprintf( f, "quadraticAttenuation: %f\n", l->quadraticAttenuation );
		fprintf( f, "active: %d\n", l->active );
		fprintf( f, "\n" );
	}
	fclose( f );
}

void zglLightLoad( char *filename ) {
	FILE *f = fopen( filename, "rt" );
	if( !f ) return;
	char line[256];
	ZGLLight *light = 0;
	while( fgets( line, 256, f ) ) {
		// LOOK for comments
		int comment = 0;
		for( int i=0; i<256; i++ ) {
			if( i=='#' ) {
				comment = 1;
				break;
			}
			else if( i=='/' ) {
				comment = 1;
				break;
			}
			if( i==0 ) {
				break;
			}
		}
		if( comment ) {
			continue;
		}

		// SPLIT on spaces
		char *fields[10] = {0,};
		fields[0] = line;
		int fieldCount = 1;
		for( char *c = line; *c; c++ ) {
			if( *c == ' ' || *c == '\r' || *c == '\n' || *c == 0 ) {
				*c = 0;
				fields[fieldCount++] = c+1;
			}
		}

		if( fields[0][0] == ':' ) {
			light = zglLightFetch( &fields[0][1] );
			if( !light ) {
				light = new ZGLLight();
				zglLightDeclare( &fields[0][1], light );
			}
		}
		if( !stricmp( fields[0], "ambient:" ) ) {
			light->ambient[0] = (float)atof( fields[1] );
			light->ambient[1] = (float)atof( fields[2] );
			light->ambient[2] = (float)atof( fields[3] );
			light->ambient[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "diffuse:" ) ) {
			light->diffuse[0] = (float)atof( fields[1] );
			light->diffuse[1] = (float)atof( fields[2] );
			light->diffuse[2] = (float)atof( fields[3] );
			light->diffuse[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "specular:" ) ) {
			light->specular[0] = (float)atof( fields[1] );
			light->specular[1] = (float)atof( fields[2] );
			light->specular[2] = (float)atof( fields[3] );
			light->specular[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "pos:" ) ) {
			light->pos[0] = (float)atof( fields[1] );
			light->pos[1] = (float)atof( fields[2] );
			light->pos[2] = (float)atof( fields[3] );
			light->pos[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "dir:" ) ) {
			light->dir[0] = (float)atof( fields[1] );
			light->dir[1] = (float)atof( fields[2] );
			light->dir[2] = (float)atof( fields[3] );
			light->dir[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "spotDir:" ) ) {
			light->spotDir[0] = (float)atof( fields[1] );
			light->spotDir[1] = (float)atof( fields[2] );
			light->spotDir[2] = (float)atof( fields[3] );
		}
		else if( !stricmp( fields[0], "spotCutoff:" ) ) {
			light->spotCutoff = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "spotExponent:" ) ) {
			light->spotExponent = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "constantAttenuation:" ) ) {
			light->constantAttenuation = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "linearAttenuation:" ) ) {
			light->linearAttenuation = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "quadraticAttenuation:" ) ) {
			light->quadraticAttenuation = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "active:" ) ) {
			light->active = atoi( fields[1] );
		}
	}
	fclose( f );
}

// zglMaterial interface
////////////////////////////////////////////////////////////////////

ZHashTable zglMaterialHash;

void ZGLMaterial::setGL() {
	if( active ) {
		glMaterialfv( side, GL_AMBIENT, ambient );
		glMaterialfv( side, GL_DIFFUSE, diffuse );
		glMaterialfv( side, GL_SPECULAR, specular );
		glMaterialfv( side, GL_EMISSION, emission );
		glMaterialfv( side, GL_SHININESS, &shininess );
	}
}


void zglMaterialDeclare( char *name, ZGLMaterial *material ) {
	zglMaterialHash.putP( name, material );
}

ZGLMaterial *zglMaterialFetch( char *name ) {
	return (ZGLMaterial *)zglMaterialHash.getp( name );
}

static ZGLMaterial defaultMaterial;

void zglMaterialReset() {
	defaultMaterial.active = 1;
	defaultMaterial.setGL();
}

void zglMaterialSetup( char *name ) {
	// Find the material structure in the database
	ZGLMaterial *material = zglMaterialFetch( name );
	if( material && material->active ) {
		material->setGL();
	}
	else {
		defaultMaterial.active = 1;
		defaultMaterial.setGL();
	}
}

void zglMaterialSaveAll( char *filename ) {
	FILE *f = fopen( filename, "wt" );

	char sorted[128][32];
	int count = 0, i;
	for( i=0; i<zglMaterialHash.size(); i++ ) {
		char *k = zglMaterialHash.getKey(i);
		if( k ) {
			strcpy( sorted[count++], k );
		}
	}
	qsort( sorted, count, 32, zglSort );

	for( i=0; i<count; i++ ) {
		char *k = sorted[i];
		ZGLMaterial *l = zglMaterialFetch( k );
		fprintf( f, ":%s\n", k );
		fprintf( f, "ambient: %f %f %f %f\n", l->ambient[0], l->ambient[1], l->ambient[2], l->ambient[3] );
		fprintf( f, "diffuse: %f %f %f %f\n", l->diffuse[0], l->diffuse[1], l->diffuse[2], l->diffuse[3] );
		fprintf( f, "specular: %f %f %f %f\n", l->specular[0], l->specular[1], l->specular[2], l->specular[3] );
		fprintf( f, "emission: %f %f %f %f\n", l->emission[0], l->emission[1], l->emission[2], l->emission[3] );
		fprintf( f, "shininess: %f\n", l->shininess );
		fprintf( f, "side: %d\n", l->side );
		fprintf( f, "active: %d\n", l->active );
		fprintf( f, "\n" );
	}
	fclose( f );
}

void zglMaterialLoad( char *filename ) {
	FILE *f = fopen( filename, "rt" );
	if( !f ) return;
	char line[256];
	ZGLMaterial *mater = 0;
	while( fgets( line, 256, f ) ) {
		// SPLIT on spaces
		char *fields[10] = {0,};
		fields[0] = line;
		int fieldCount = 1;
		for( char *c = line; *c; c++ ) {
			if( *c == ' ' || *c == '\r' || *c == '\n' || *c == 0 ) {
				*c = 0;
				fields[fieldCount++] = c+1;
			}
		}

		if( fields[0][0] == ':' ) {
			mater = zglMaterialFetch( &fields[0][1] );
			if( !mater ) {
				mater = new ZGLMaterial();
				zglMaterialDeclare( &fields[0][1], mater );
			}
		}
		if( !stricmp( fields[0], "ambient:" ) ) {
			mater->ambient[0] = (float)atof( fields[1] );
			mater->ambient[1] = (float)atof( fields[2] );
			mater->ambient[2] = (float)atof( fields[3] );
			mater->ambient[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "diffuse:" ) ) {
			mater->diffuse[0] = (float)atof( fields[1] );
			mater->diffuse[1] = (float)atof( fields[2] );
			mater->diffuse[2] = (float)atof( fields[3] );
			mater->diffuse[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "specular:" ) ) {
			mater->specular[0] = (float)atof( fields[1] );
			mater->specular[1] = (float)atof( fields[2] );
			mater->specular[2] = (float)atof( fields[3] );
			mater->specular[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "emission:" ) ) {
			mater->emission[0] = (float)atof( fields[1] );
			mater->emission[1] = (float)atof( fields[2] );
			mater->emission[2] = (float)atof( fields[3] );
			mater->emission[3] = (float)atof( fields[4] );
		}
		else if( !stricmp( fields[0], "shininess:" ) ) {
			mater->shininess = (float)atof( fields[1] );
		}
		else if( !stricmp( fields[0], "side:" ) ) {
			mater->side = atoi( fields[1] );
		}
		else if( !stricmp( fields[0], "active:" ) ) {
			mater->active = atoi( fields[1] );
		}
	}
	fclose( f );
}

