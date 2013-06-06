// @ZBS {
//		*MODULE_OWNER_NAME zmesh
// }

#include "zvec.h"

// @TODO: Eliminate the dependency on vec

typedef float ZMeshVertType;
typedef unsigned short ZMeshFaceType;

struct ZMeshDesc {
	char name[64];
	char parentName[64];
	FMat4 restMatWorld;
	FMat4 restMatLocal;
	int vertCount;
	int normCount;
	int faceCount;
	FVec3 rotConstrainMin;
	FVec3 rotConstrainMax;

	ZMeshVertType *verts() { return (ZMeshVertType *)( (char *)this + sizeof(*this) ); }
	ZMeshVertType *norms() { return (ZMeshVertType *)( (char *)this + sizeof(*this) + vertCount * sizeof(ZMeshVertType) * 3 ); }
	ZMeshFaceType *faces() { return (ZMeshFaceType *)( (char *)this + sizeof(*this) + vertCount * sizeof(ZMeshVertType) * 3 + normCount * sizeof(ZMeshVertType) * 3 ); }

	int size() { return sizeof(*this) + sizeof(ZMeshVertType)*vertCount*3 + sizeof(ZMeshVertType)*normCount*3 + sizeof(ZMeshFaceType)*faceCount*3; }
};

struct ZMeshSkinDesc {
	// This is a dynamiclly sized packet structure.  It is never instanciated
	// directly as the internal structs are never in the order here.  Always
	// use the accessor methods.

	char meshName[64];
	int numBones;
	int numVerts;
	int numWeights;
	int inWorldSpace;
	// DYNAMIC SIZED:
	//   char boneName[64][numBones]
	//   unsigned char numBonesPerVert[numVerts]
	//   unsigned char boneIdPerWeight[numWeights]
	//   float weight[numWeights]
	//   FVec3 offsetVector[numWeights]

	char *boneName(int b) { return (char *)this + sizeof(*this) + 64*b; }
	unsigned char *numBonesPerVert(int v) { return (unsigned char *)this + sizeof(*this) + 64*numBones + v*sizeof(unsigned char); }
	unsigned char *boneIdPerWeight(int w) { return (unsigned char *)this + sizeof(*this) + 64*numBones + numVerts*sizeof(unsigned char) + w*sizeof(unsigned char); }
	float *weight(int w) { return (float *)( (char *)this + sizeof(*this) + 64*numBones + numVerts*sizeof(unsigned char) + numWeights*sizeof(unsigned char) + w*sizeof(float) ); }
	FVec3 *offsetVector(int w) { return (FVec3 *)( (char *)this + sizeof(*this) + 64*numBones + numVerts*sizeof(unsigned char) + numWeights*sizeof(unsigned char) + numWeights*sizeof(float) + w*sizeof(FVec3) ); }

	int size() { return sizeof(*this) + 64*numBones + numVerts*sizeof(unsigned char) + numWeights*sizeof(unsigned char) + numWeights*sizeof(float) + numWeights*sizeof(FVec3); }
};

struct ZMeshAnimDesc {
	char animName[64];
	int numFrames;
	int numNodes;
	// DYNAMIC SIZED:
	//   char nodeName[numNodes][64];
	//   FMat4 mat[numNodes][numFrames]

	char *nodeName(int node) { return (char *)this + sizeof(*this) + 64*node; }
	FMat4 *mat(int node,int frame) { return (FMat4 *)((char *)this + sizeof(*this) + 64*numNodes + sizeof(FMat4)*numFrames*node + sizeof(FMat4)*frame); }

	int size() { return sizeof(*this) + 64*numNodes + sizeof(FMat4)*numNodes*numFrames; }
};

struct ZMesh {
	ZMeshDesc *desc;
	ZMesh *parent;
	ZMesh *headChild;
	ZMesh *nextSib;
	ZMesh *prevSib;
	ZMeshVertType *vertCopy;
	ZMeshVertType *normCopy;
	ZMeshVertType *vertWorld;
	ZMeshVertType *normWorld;
	char localName[64];

	FMat4 matWorld;
		// Takes local verts into the world space
	FMat4 matLocal;
		// Takes local verts into the parent space
	FVec3 rotConstrainMin;
		// Joint constraint
	FVec3 rotConstrainMax;
		// Joint constraint

	ZMesh();
	~ZMesh();

	void clear();
		// Frees all buffers, resets to defaults

	void allocVertCopy();
	void allocNormCopy();
	void allocVertWorld();
	void allocNormWorld();

	int vertCount();
	int normCount();
	int faceCount();
	ZMeshVertType *verts();
	ZMeshVertType *norms();
	ZMeshFaceType *faces();

	ZMeshVertType *worldVerts();
	ZMeshVertType *worldNorms();

	void constrainJoint();
	void resetToRestMatLocal();

	void detachFromParent();
	void attachTo( ZMesh *_parent );
};

struct ZMeshAnimController {
	ZMesh *rootMesh;
	ZMeshAnimDesc *animDesc;
	FMat4 startLocalMat;
		// The state of the root mesh at the time that this animation began
	FMat4 startAnimMatInverse;
		// The inverse of the first frame of the animation
	float fps;
	double startTime;
	ZMesh **nodes;
		// An allocated array cache of the nodes

	int debugIgnoreTime;
		// This is a useful debugging flag.  When it is on,
		// the time is ignored and instead the animation is
		// advanced one frame per call to update.
	int debugFrame;
		// When ignoring time, this is the current frame

	void update();
	float percentComplete();
};

// IMPORT AND CACHE -------------------------------------------------------------------------
void zMeshImport( char *filename );

ZMeshDesc *zMeshDescAlloc( int vertCount, int normCount, int faceCount, char *name );
	// Create a new ZMeshDesc and stick a pointer to it in the cache
	// Be careful that you aren't overwriting some other name

void zMeshDescFree( char *name );
	// Deletes the mesh desc

// INSTANCE MANIPULATION
ZMeshDesc *zMeshEnumMeshDesc( int &i );
	// Enumerates all mesh descriptions in random order.  Init i to 0

ZMeshDesc *zMeshGetDesc( char *meshName );
	// Lookup the mesh description from a hash

ZMesh *zMeshEnum( int &i );
	// Enumerates all of the meshes in random order. Init i to 0

ZMesh *zMeshInstanciateMesh( char *localName, char *meshName, int recurse=0 );
	// For a non-recursive mesh, the only entry in the hashtable will
	// be the local name.
	//   For a recursive mesh, there will be an entry for each child-mesh
	// in the form "localName/meshName". Also, the root object will have
	// and extra entry of just the local name so that you can fetch
	// it without knowing the names of the mesh objects

ZMesh *zMeshGet( char *localName );
	// Lookup the local name in a the hash.  Remember to specify full path for recursively created
	// object.  For example, it you instanciated "arm" and there's a child mesh called "thumb" then
	// you would look for "arm/thumb"

ZMesh *zMeshFindChild( char *meshName, ZMesh *root );
	// Find the child named meshName under root


// HEIRARCHY -------------------------------------------------------------------------

void zMeshUpdateWorldMats( ZMesh *root );
	// Recursively traverses the heirarchy and updates all the
	// matWorlds from the matLocals

void zMeshResetHeirarchy( ZMesh *root );
	// Resets all matLocals to their rest position

void zMeshUpdateWorldVerts( ZMesh *root );
	// Recursively traverses the heirarchy and updates all the verts from matWorld into the world buffer
	// This must be called after the world mat is set with zMeshUpdateWorldMats

void zMeshTransformVectorArray( float *input, float *output, float *mat4x4, int numVerts, int inputStepInBytes=sizeof(float)*3, int outputStepInBytes=sizeof(float)*3 );
	// Same as zmath function but here to avoid dependency

// BONE MOVEMENT AND IK -------------------------------------------------------------------------

void zMeshIkSolve( ZMesh *effector, ZMesh *root, FVec3 goal );
	// CCDIK solve for effector to goal from root pivot

void zMeshDeformSkin( ZMesh *skinMesh, ZMesh *boneRoot );
	// Deform the skinMesh onto the bones.  Note that a copy of the skin
	// verts will be made by a call to "copyVerts" on the skinMesh
	// The skin description will be looked up in a hash
	// Assumes that you have already updated the worldmats on the boneChain

// ANIMATION -------------------------------------------------------------------------
void zMeshAnimStart( char *animName, ZMesh *root, float fps, double startTime );
	// Start an animation on the heirarchy
	// Starting an animation on a heirarchy that already has
	// an animation playing will stop the previous animation and
	// begin the new one from the current position

void zMeshAnimStart( char *animName, char *localName, float fps, double startTime );
	// Start an animation, lookup by local name

void zMeshAnimStop( ZMesh *root );
	// Stop any animation playing on the heirarchy

void zMeshAnimUpdate( ZMesh *root, double time );
	// Update the state of animation on the heirarchy
	// time is the current time (passed to avoid dependency on a given time module)

void zMeshAnimUpdateAll( double time );
	// Update the state of all running animations.
	// time is the current time (passed to avoid dependency on a given time module)


// RENDER -------------------------------------------------------------------------
void zMeshGLRender( ZMesh *mesh, int recurse=0, int wireframe=0 );
void zMeshGLRenderFaceted( ZMesh *mesh, int recurse=0, int wireframe=0 );
void zMeshGLRenderNormals( ZMesh *mesh, int recurse=0, float len=1.f );
void zMeshGLRenderVerts( ZMesh *mesh, int recurse=0 );
void zMeshGLRenderShadowVolume( ZMesh *mesh, FVec3 light, float extrusionMultiplier, int recurse=0 );

