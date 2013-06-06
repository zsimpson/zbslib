// @ZBS {
//		*MODULE_NAME pdb
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			AFuncs for loading pdb molecular file format
//		}
//		*PORTABILITY win32 unx
//		*REQUIRED_FILES pdb.cpp pdb.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "pdb.h"
// ZBSLIB includes:


float PdbMole::radius[36] = {
	// From http://web.mit.edu/3.091/www/pt/pert1.html
	// I presume these are un-ionized radii defined by some sort of iso-electronic surface
	0.45f, 1.f, 
	1.55f, 1.12f, 0.98f, 0.91f, 0.92f, 0.65f, 0.57f, 0.51f,
	1.9f, 1.6f, 1.43f, 1.32f, 1.28f, 1.27f, 0.97f, 0.88f,
	2.35f, 1.97f, 1.62f, 1.45f, 1.34f, 1.3f, 1.35f, 1.26f, 1.25f, 1.24f, 1.28f, 1.38f, 1.41f, 1.37f, 1.39f, 1.4f, 1.12f, 1.03f,
};

char *elements[] = {
	"H", "He",
	"Li", "Be", "B", "C", "N", "O", "F", "Ne",
	"Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar",
	"K", "Ca", "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr",
};

PdbMole *pdbLoad( char *filename ) {
	FILE *f = fopen( filename, "rt" );
	assert( f );

	// PASS 1: Count lines
	int count = 0;
	char line[256];
	while( fgets(line,sizeof(line),f) ) {
		if( !strncmp(&line[0],"TER",3) ) {
			break;
		}
		if( !strncmp(&line[0],"ATOM",4) ) {
			count++;
		}
//		if( !strncmp(&line[0],"TER",3) ) {
//			break;
//		}
	}

	// ALLOC
	PdbMole *pdbMole = new PdbMole;
	pdbMole->atomCount = count;
	pdbMole->atomPos = new float[ 3 * count ];
	pdbMole->atomType = new char[ count ];

	// PASS 2: Load
	float mean[3] = { 0, };
	int i = 0;
	fseek( f, 0, SEEK_SET );
	memset( line, 0, sizeof(line) );
	while( fgets(line,sizeof(line),f) ) {
		if( !strncmp(&line[0],"TER",3) ) {
			break;
		}
		if( !strncmp(&line[0],"ATOM",4) ) {
			char atom[3] = {0,};
			memcpy( atom, &line[77], 2 );
			if( atom[1] == ' ' ) { atom[1] = 0; }
			if( ! (atom[0] >= 'A' && atom[0] <= 'Z') ) {
				atom[0] = line[13]; atom[1] = 0;
			}

			char buf[8];
			memset( buf, 0, sizeof(buf) );
			memcpy( buf, &line[30], 8 );
			float x = (float)atof( buf );
			memset( buf, 0, sizeof(buf) );
			memcpy( buf, &line[38], 8 );
			float y = (float)atof( buf );
			memset( buf, 0, sizeof(buf) );
			memcpy( buf, &line[46], 8 );
			float z = (float)atof( buf );
			pdbMole->atomPos[i*3+0] = x;
			pdbMole->atomPos[i*3+1] = y;
			pdbMole->atomPos[i*3+2] = z;
			mean[0] += x;
			mean[1] += y;
			mean[2] += z;
			int found = 0;
			for( int e=0; e<sizeof(elements)/sizeof(elements[0]); e++ ) {
				if( !stricmp(elements[e],atom) ) {
					pdbMole->atomType[i] = e;
					found++;
					break;
				}
			}
			assert( found );
			i++;
		}
//		if( !strncmp(&line[0],"TER",3) ) {
//			break;
//		}
		memset( line, 0, sizeof(line) );
	}
	fclose( f );

	// PASS 3: Center about mean
	for( i=0; i<count; i++ ) {
		pdbMole->atomPos[i*3+0] -= mean[0] / (float)count;
		pdbMole->atomPos[i*3+1] -= mean[1] / (float)count;
		pdbMole->atomPos[i*3+2] -= mean[2] / (float)count;
	}

	return pdbMole;
}

// If we ever need pdb data without a renderer then 
// it would make sense to isolate this into its own file

void pdbMoleRender( PdbMole *pdbMole, int tessLevel, float moleScale, int cut0, int cut1 ) {
	if( !pdbMole ) return;
	if( cut0 == -1 ) cut0 = 0;
	if( cut1 == -1 ) cut1 = pdbMole->atomCount;

	GLuint sphereList = 0;
	sphereList = glGenLists( 1 );
	glNewList(1, GL_COMPILE);
	GLUquadricObj *q = gluNewQuadric();
	gluSphere( q, 1.0, tessLevel, tessLevel );
	gluDeleteQuadric(q);
	glEndList();

	static unsigned char elemColors[][3] = {
		{ 255, 255, 255 }, // H
		{ 255, 0, 0 }, // He
		{ 255, 0, 0 }, // Li
		{ 255, 0, 0 }, // Be
		{ 255, 0, 0 }, // B
		{ 50, 50, 50 }, // C
		{ 100, 100, 200 }, // N
		{ 200, 100, 100 }, // O
		{ 255, 0, 0 }, // F
		{ 255, 0, 0 }, // Ne
		{ 255, 0, 0 }, // Na
		{ 255, 0, 0 }, // Mg
		{ 255, 0, 0 }, // Al
		{ 100, 100, 50 }, // Si
		{ 50, 200, 50 }, // P
		{ 200, 200, 0 }, // S
		{ 255, 0, 0 }, // Cl
		{ 255, 0, 0 }, // Ar
	};

	int end = min( cut1, pdbMole->atomCount );
	for( int i=cut0; i<end; i++ ) {
		glPushMatrix();
		float *pos = pdbMole->getPos(i);
		float r = moleScale * pdbMole->getRadius(i);
		int type = pdbMole->getAtom(i);
		glColor3ubv( &elemColors[type][0] );
		glTranslatef( pos[0], pos[1], pos[2] );
		glScalef( r, r, r );
		glCallList( sphereList );
		glPopMatrix();
	}

	glDeleteLists( sphereList, 1 );
}

