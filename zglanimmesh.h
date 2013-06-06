#ifndef ZGLZNIMMESH_H
#define ZGLZNIMMESH_H

typedef float ZGLAnimMeshVertType;
typedef unsigned short ZGLAnimMeshFaceIndexType;

struct ZGLAnimMeshDesc {
	char name[128];
	int vertCount;
	int faceCount;
	int normCount;

	ZGLAnimMeshVertType *verts() { return (ZGLAnimMeshVertType *)( (char *)this + sizeof(name) + 4 * 3 ); }
	ZGLAnimMeshFaceIndexType *faces() { return (ZGLAnimMeshFaceIndexType *)( (char *)this + sizeof(*this) + vertCount * sizeof(ZGLAnimMeshVertType) * 3 ); }
	ZGLAnimMeshVertType *norms() { return (ZGLAnimMeshVertType *)( (char *)this + sizeof(*this) + vertCount * sizeof(ZGLAnimMeshVertType) * 3 + faceCount * sizeof( ZGLAnimMeshFaceIndexType ) * 3 ); }

	ZGLAnimMeshDesc *next() {
		return (ZGLAnimMeshDesc *)( (char *)this + sizeof(*this) + vertCount*sizeof(ZGLAnimMeshVertType)*3 + faceCount*sizeof(ZGLAnimMeshFaceIndexType)*3 + normCount*sizeof(ZGLAnimMeshVertType)*3 );
	}
};

struct ZGLAnimMeshFile {
	int frameCount;
	int shapeCount;
	int frameOffsets[1];

	void fillHashOfShapes( int frame, ZHashTable *hash );
		// Fills the hash table with the addresses of ZGLAnimMeshDesc by name on given frame

	ZGLAnimMeshDesc *getFrame( int i ) {
		return (ZGLAnimMeshDesc *)( (char *)this + frameOffsets[i] );
	}
};


extern ZGLAnimMeshFile *zglAnimMeshOpen( char *filename );
	// Opens the file as a read-only memory map.  Be sure to close when you are done
	// It caches the pointer so it is a safe & fast to re-open the same file again & again

extern void zglAnimMeshClose( char *filename );
	// Closes the memory map.

#endif


