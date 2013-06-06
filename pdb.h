// @ZBS {
//		*MODULE_OWNER_NAME pdb
// }

#ifndef PDB_H
#define PDB_H

struct PdbMole {
	int atomCount;
	float *atomPos;
	char *atomType;
	static float radius[36];

	int getAtom( int i ) {
		return atomType[i];
	}
	float *getPos( int i ) {
		return &atomPos[i*3];
	}
	float getRadius( int i ) {
		return radius[atomType[i]];
	}
};

PdbMole *pdbLoad( char *filename );

void pdbMoleRender( PdbMole *pdbMole, int tessLevel=10, float moleScale=1.f, int cut0=-1, int cut1=-1 );

#endif