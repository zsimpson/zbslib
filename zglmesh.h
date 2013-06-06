#ifndef GLMESH_H
#define GLMESH_H

struct GLMesh {
	int vCount, fCount;
	float *verts;
	float *norms;
	float *uvcrd;
	unsigned short *faces;
	int hasUV;

	GLMesh();
	void alloc( int _vCount, int _fCount );
	void clear();
	void render();
	void renderNorms( float len );
};

struct GLHeirMesh {
	GLHeirMesh *parent;
	GLHeirMesh *nextSib;
	GLHeirMesh *prevSib;
	GLHeirMesh *firstChild;
	GLMesh *mesh;
};

#endif

