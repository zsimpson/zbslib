// @ZBS {
//		+DESCRIPTION {
//			Yet another mesh system
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmesh.cpp zmesh.h
//		*OPTIONAL_FILES
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Elmininate GL dependendy
//			Elmininate vec3 dependendy
//			Elmininate hashtable dependendy (using as string anyway so it is inefficient)
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
	// Above two only for the gl stuff which should probably
	// be moved to another file.
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"
#include "math.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
// ZBSLIB includes:
#include "zmesh.h"
#include "zvec.h"
#include "zhashtable.h"


#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif



//////////////////////////////////////////////////////////////////////////
// HASHTABLES

static ZHashTable meshDescByName;
	// Each mesh has a name.

static ZHashTable skinDescByName;
	// A hash of ZMeshSkinDesc

static ZHashTable meshByLocalName;
	// A hash of ZMesh by the local name.

static ZHashTable animDescByName;
	// Each animation has a name

static ZHashTable animControllerByRoot;
	// Holds pointers to ZMeshAnimController by the
	// object to which it is attached



ZMeshDesc *zMeshGetMeshDesc( char *name ) {
	return (ZMeshDesc *)meshDescByName.getI( name );
}

ZMeshDesc *zMeshGetDesc( char *meshName ) {
	return (ZMeshDesc *)meshDescByName.getI( meshName );
}

ZMeshDesc *zMeshEnumMeshDesc( int &i ) {
	for( int _i=i; _i<meshDescByName.size(); _i++ ) {
		ZMeshDesc *d = (ZMeshDesc *)meshDescByName.getValI( i );
		if( d ) {
			i++;
			return d;
		}
		i++;
	}
	return 0;
}

ZMesh *zMeshEnum( int &i ) {
	for( int _i=i; _i<meshByLocalName.size(); _i++ ) {
		ZMesh *m = (ZMesh *)meshByLocalName.getValI( i );
		if( m ) {
			i++;
			return m;
		}
		i++;
	}
	return 0;
}

ZMesh *zMeshGet( char *localName ) {
	return (ZMesh *)meshByLocalName.getI( localName );
}

ZMesh *zMeshFindChild( char *meshName, ZMesh *root ) {
	if( !strcmp(root->desc->name,meshName) ) {
		return root;
	}
	for( ZMesh *c=root->headChild; c; c=c->nextSib ) {
		ZMesh *found = zMeshFindChild( meshName, c );
		if( found ) {
			return found;
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// IMPORT

char **splitLine( char *line ) {
	int i;
	
	static char *fields[64];
	for( i=0; i<64; i++ ) {
		fields[i] = 0;
	}

	char *c;
	for( c = line; *c; c++ ) {
		if( *c == '/' && *(c+1)=='/' ) {
			*c = 0;
			break;
		}
	}

	for( c = line; *c && (*c==' ' || *c=='\t'); c++ );

	fields[0] = c;
	i = 1;
	int inQuote = 0;
	for( ; *c; c++ ) {
		if( *c == '\"' ) {
			inQuote = !inQuote;
		}

		if( (!inQuote && *c == ' ') || *c == '\r' || *c == '\n' ) {
			*c = 0;
			fields[i++] = c+1;
		}
	}
	return fields;
}

int compareToken( char *s, char **list ) {
	int i = 0;

	if( s[0] == '}' ) return -1;

	for( char **l = list; *l; l++ ) {
		if( !stricmp(s,*l) ) {
			return i;
		}
		i++;
	}
	return -2;
}

char *deQuote( char *s ) {
	if( *s == '\"' ) {
		char *start = s+1;
		s[strlen(s)-1] = 0;
		return start;
	}
	return s;
}

FMat4 parseMatrix( char **fields ) {
	FMat4 m;
	m.m[0][0] = (float)atof( fields[ 0] );
	m.m[0][1] = (float)atof( fields[ 1] );
	m.m[0][2] = (float)atof( fields[ 2] );
	m.m[0][3] = 0.f;

	m.m[1][0] = (float)atof( fields[ 3] );
	m.m[1][1] = (float)atof( fields[ 4] );
	m.m[1][2] = (float)atof( fields[ 5] );
	m.m[1][3] = 0.f;

	m.m[2][0] = (float)atof( fields[ 6] );
	m.m[2][1] = (float)atof( fields[ 7] );
	m.m[2][2] = (float)atof( fields[ 8] );
	m.m[2][3] = 0.f;

	m.m[3][0] = (float)atof( fields[ 9] );
	m.m[3][1] = (float)atof( fields[10] );
	m.m[3][2] = (float)atof( fields[11] );
	m.m[3][3] = 1.f;

	return m;
}


void loadVectorArrayVert( FILE *f, ZMeshVertType *vecArray, int count ) {
	int i = 0;
	char line[256];
	while( fgets( line, 256, f ) ) {
		char **fields = splitLine( line );
		if( fields[0][0] == '}' ) break;
		vecArray[i++] = (ZMeshVertType)atof(fields[0]);
		vecArray[i++] = (ZMeshVertType)atof(fields[1]);
		vecArray[i++] = (ZMeshVertType)atof(fields[2]);
		assert( i <= count*3 );
	}
}

void loadVectorArrayFace( FILE *f, ZMeshFaceType *vecArray, int count ) {
	int i = 0;
	char line[256];
	while( fgets( line, 256, f ) ) {
		char **fields = splitLine( line );
		if( fields[0][0] == '}' ) break;
		if( !fields[0][0] ) continue;
		vecArray[i++] = (ZMeshFaceType)atoi(fields[0]);
		vecArray[i++] = (ZMeshFaceType)atoi(fields[1]);
		vecArray[i++] = (ZMeshFaceType)atoi(fields[2]);
		assert( i <= count*3 );
	}
}

void loadStringArray( FILE *f, char **array, int count ) {
	int i = 0;
	char line[256];
	while( fgets( line, 256, f ) ) {
		char **fields = splitLine( line );
		if( fields[0][0] == '}' ) break;
		array[i++] = strdup( deQuote(fields[0]) );
		assert( i <= count );
	}
}

void loadAnimMatArray( FILE *f, FMat4 *array, int count ) {
	int i = 0;
	char line[256];
	while( fgets( line, 256, f ) ) {
		char **fields = splitLine( line );
		if( fields[0][0] == '}' ) break;
		array[i++] = parseMatrix( fields );
		assert( i <= count );
	}
}


ZMeshDesc *zMeshDescAlloc( int vertCount, int normCount, int faceCount, char *name ) {
	ZMeshDesc meshDescTemp;
	memset( &meshDescTemp, 0, sizeof(meshDescTemp) );
	meshDescTemp.vertCount = vertCount;
	meshDescTemp.faceCount = faceCount;
	meshDescTemp.normCount = normCount;
	meshDescTemp.restMatLocal.identity();
	meshDescTemp.restMatWorld.identity();
	strcpy( meshDescTemp.name, name );
	ZMeshDesc *meshDesc = (ZMeshDesc *)malloc( meshDescTemp.size() );
	memcpy( meshDesc, &meshDescTemp, sizeof(meshDescTemp) );
	meshDescByName.putI( meshDesc->name, (int)meshDesc );
	return meshDesc;
}

ZMeshDesc *zMeshDescAlloc( ZMeshDesc *meshDescTemp ) {
	ZMeshDesc *meshDesc = (ZMeshDesc *)malloc( meshDescTemp->size() );
	memcpy( meshDesc, meshDescTemp, sizeof(*meshDescTemp) );
	meshDescByName.putI( meshDesc->name, (int)meshDesc );
	return meshDesc;
}

void zMeshDescFree( char *name ) {
	ZMeshDesc *meshDesc = (ZMeshDesc *)meshDescByName.getI( name );
	if( meshDesc ) {
		delete meshDesc;
		meshDescByName.del( name );
	}
}


ZMeshAnimDesc *zMeshAnimDescAlloc( ZMeshAnimDesc *meshAnimDescTemp ) {
	ZMeshAnimDesc *meshAnimDesc = (ZMeshAnimDesc *)malloc( meshAnimDescTemp->size() );
	memcpy( meshAnimDesc, meshAnimDescTemp, sizeof(*meshAnimDesc) );
	animDescByName.putI( meshAnimDesc->animName, (int)meshAnimDesc );
	return meshAnimDesc;
}


#define READLINES( tokens ) \
	while( fgets( line, 256, f ) ) { \
		char **fields = splitLine( line ); \
		int i = compareToken( fields[0], tokens ); \
		if( i == -1 ) break; \
		switch( i ) {

#define ENDREADLINES }}

void zMeshImport( char *filename ) {
	FILE *f = fopen( filename, "rb" );
	if( !f ) return;
	char line[256];

	char *tokens[] = { "@GEOM_OBJECT", "@ANIMATION", 0 };
	READLINES( tokens )
		case 0: { // READING MESH
			ZMeshDesc meshDescTemp;
			memset( &meshDescTemp, 0, sizeof(meshDescTemp) );
			ZMeshDesc *meshDesc = 0;
			const float PI2 = 3.1415926539f * 2.f;

			char *tokens[] = { "*NAME", "*WORLD_TRANSFORM", "*LOCAL_TRANSFORM", "*PARENT", "*NUM_VERTS", "*NUM_NORMALS", "*NUM_FACES", "+VERTS", "+NORMALS", "+FACES", "@SKIN", 0 };
			READLINES( tokens )
				case 0: strcpy( meshDescTemp.name, deQuote(fields[1]) ); break;
				case 1: meshDescTemp.restMatWorld = parseMatrix( &fields[1] ); break;
				case 2: meshDescTemp.restMatLocal = parseMatrix( &fields[1] ); break;
				case 3: strcpy( meshDescTemp.parentName, deQuote(fields[1]) ); break;
				case 4: meshDescTemp.vertCount = atoi( fields[1] ); break;
				case 5: meshDescTemp.normCount = atoi( fields[1] ); break;
				case 6: meshDescTemp.faceCount = atoi( fields[1] ); break;

				case 7: {
					if(!meshDesc) meshDesc = zMeshDescAlloc( &meshDescTemp );
					loadVectorArrayVert( f, meshDesc->verts(), meshDesc->vertCount );
					break;
				}
				case 8: {
					if(!meshDesc) meshDesc = zMeshDescAlloc( &meshDescTemp );
					loadVectorArrayVert( f, meshDesc->norms(), meshDesc->normCount );
					for( int i=0; i<meshDesc->normCount; i++ ) {
						((FVec3 *)meshDesc->norms())[i].normalize();
					}
					break;
				}
				case 9: {
					if(!meshDesc) meshDesc = zMeshDescAlloc( &meshDescTemp );
					loadVectorArrayFace( f, meshDesc->faces(), meshDesc->faceCount );
					break;
				}
				case 10: {
					// SKIN
					if(!meshDesc) meshDesc = zMeshDescAlloc( &meshDescTemp );
					const int SKIN_BUFFER_SIZE = 4096 * 128;
					ZMeshSkinDesc *skin = (ZMeshSkinDesc *)alloca( SKIN_BUFFER_SIZE );
					memset( skin, 0, SKIN_BUFFER_SIZE );
					strcpy( skin->meshName, meshDesc->name );
					skin->numVerts = meshDesc->vertCount;
					char *tokens[] = { "*NUM_BONES", "+BONES", "*NUM_WEIGHTS", "+NUM_BONES_PER_WEIGHT", "+WEIGHTS", 0 };
					READLINES( tokens )
						case 0: skin->numBones = atoi(fields[1]); break;
						case 1: {
							char **array = (char **)alloca( sizeof(char *) * skin->numBones );
							loadStringArray( f, array, skin->numBones );
							for( int i=0; i<skin->numBones; i++ ) {
								strcpy( skin->boneName(i), array[i] );
								free( array[i] );
							}
							break;
						}

						case 2:
							skin->numWeights = atoi( fields[1] );
							break;

						case 3: {
							unsigned char *numBonesPerVert = skin->numBonesPerVert(0);
							while( fgets( line, 256, f ) ) {
								char **fields = splitLine( line );
								if( fields[0][0] == '}' ) break;
								*numBonesPerVert++ = atoi( fields[0] );
							}
							break;
						}

						case 4: {
							unsigned char *boneIdPerWeight = skin->boneIdPerWeight(0);
							float *weight = skin->weight(0);
							FVec3 *offsetVector = skin->offsetVector(0);
							while( fgets( line, 256, f ) ) {
								char **fields = splitLine( line );
								if( fields[0][0] == '}' ) break;
								*boneIdPerWeight++ = atoi( fields[0] );
								*weight++ = (float)atof( fields[1] );
								*offsetVector++ = FVec3( (float)atof( fields[2] ), (float)atof( fields[3] ), (float)atof( fields[4] ) );
							}
							ZMeshSkinDesc *_skin = (ZMeshSkinDesc *)malloc( skin->size() );
							memcpy( _skin, skin, skin->size() );
							skinDescByName.putI( _skin->meshName, (int)_skin );
							break;
						}
					ENDREADLINES;
					break;
				}
			ENDREADLINES
			break;
		}

		case 1: { // READING ANIMATION
			ZMeshAnimDesc animTemp;
			memset( &animTemp, 0, sizeof(animTemp) );
			ZMeshAnimDesc *anim = 0;
			int nodeCount = 0;

			char *tokens[] = { "*ANIMATION_NAME", "*NUM_FRAMES", "*NUM_NODES", "@NODE", 0 };
			READLINES( tokens )
				case 0: strcpy( animTemp.animName, deQuote(fields[1]) ); break;
				case 1: animTemp.numFrames = atoi( fields[1] ); break;
				case 2: animTemp.numNodes = atoi( fields[1] ); break;
				case 3: {
					if( !anim ) anim = zMeshAnimDescAlloc( &animTemp );
					char *tokens[] = { "*NAME", "+DATA", 0 };
					READLINES( tokens )
						case 0: strncpy( anim->nodeName(nodeCount), deQuote(fields[1]), 64 ); break;
						case 1: loadAnimMatArray( f, anim->mat(nodeCount,0), anim->numFrames ); break;
					ENDREADLINES;
					nodeCount++;
				}
			ENDREADLINES;
		}
	ENDREADLINES

	fclose( f );
}


//////////////////////////////////////////////////////////////////////////
// ZMesh

ZMesh *zMeshInstanciateMeshRecurse( char *pathName, char *meshName, int recurse, ZMesh *parent ) {
	ZMeshDesc *meshDesc = (ZMeshDesc *)meshDescByName.getI( meshName );
	assert( meshDesc );
	ZMesh *zMesh = new ZMesh;

	if( pathName ) {
		char fullName[128] = {0,};
		strcat( fullName, pathName );
		strcat( fullName, "/" );
		strcat( fullName, meshName );
		meshByLocalName.putI( fullName, (int)zMesh );
		strcpy( zMesh->localName, fullName );
	}

	zMesh->desc = meshDesc;

	// INIT
	zMesh->parent = parent;
	zMesh->nextSib = 0;
	zMesh->prevSib = 0;
	zMesh->headChild = 0;

	// LINK to heirarchy
	if( parent ) {
		if( parent->headChild ) {
			parent->headChild->prevSib = zMesh;
		}
		zMesh->nextSib = parent->headChild;
		parent->headChild = zMesh;
	}

	// SETUP mats
	zMesh->matWorld = meshDesc->restMatWorld;
	zMesh->matLocal.identity();
	if( parent ) {
		zMesh->matLocal = meshDesc->restMatLocal;

		// @TODO: Only doing this because that lcoal mats are fucked from the export
//		FMat4 parentMat = parent->matWorld;
//		parentMat.inverse();
//		parentMat.cat( zMesh->matWorld );
//		zMesh->matLocal = parentMat;
	}
	else {
		zMesh->matLocal = zMesh->matWorld;
	}						 
//	zMesh->rotConstrainMax = meshDesc->rotConstrainMax;
//	zMesh->rotConstrainMin = meshDesc->rotConstrainMin;
	zMesh->rotConstrainMax = FVec3(  3.14f,  3.14f,  3.14f );
	zMesh->rotConstrainMin = FVec3( -3.14f, -3.14f, -3.14f );

	// RECURSE
	if( recurse ) {
		// Find all mesh that have this name as their parent
		for( int i=0; i<meshDescByName.size(); i++ ) {
			ZMeshDesc *d = (ZMeshDesc *)meshDescByName.getValI( i );
			if( d && !strcmp(d->parentName,meshName) ) {
				zMeshInstanciateMeshRecurse( pathName, d->name, 1, zMesh );
			}
		}
	}

	return zMesh;
}

ZMesh *zMeshInstanciateMesh( char *localName, char *meshName, int recurse ) {
	ZMesh *zMesh = 0;
	if( !recurse ) {
		// For a non-recursive mesh, the only entry in the hashtable will
		// be the local name.
		zMesh = zMeshInstanciateMeshRecurse( 0, meshName, 0, 0 );
		strcpy( zMesh->localName, localName );
		meshByLocalName.putI( localName, (int)zMesh );
	}
	else {
		// For a recursive mesh, there will be an entry for each child-mesh
		// in the form "localName/meshName". Also, the root object will have
		// and extra entry of just the local name so that you can fetch
		// it without knowing the names of the mesh objects
		zMesh = zMeshInstanciateMeshRecurse( localName, meshName, 1, 0 );
		meshByLocalName.putI( localName, (int)zMesh );
	}
	return zMesh;
}

ZMesh::ZMesh() {
	desc = 0;
	matWorld.identity();
	matLocal.identity();
	parent = 0;
	headChild = 0;
	nextSib = 0;
	prevSib = 0;
	vertCopy = 0;
	normCopy = 0;
	vertWorld = 0;
	normWorld = 0;
	const float PI = 3.1415926539f;
	rotConstrainMin = FVec3(-PI,-PI,-PI);
	rotConstrainMax = FVec3( PI, PI, PI );
}

ZMesh::~ZMesh() {
	clear();
}

void ZMesh::clear() {
	desc = 0;
	matWorld.identity();
	matLocal.identity();
	parent = 0;
	headChild = 0;
	nextSib = 0;
	prevSib = 0;
	const float PI = 3.1415926539f;
	rotConstrainMin = FVec3(-PI,-PI,-PI);
	rotConstrainMax = FVec3( PI, PI, PI );

	if( vertCopy ) {
		free( vertCopy );
	}
	if( normCopy ) {
		free( normCopy );
	}
	if( vertWorld ) {
		free( vertWorld );
	}
	if( normWorld ) {
		free( normWorld );
	}
	vertCopy = 0;
	normCopy = 0;
	vertWorld = 0;
	normWorld = 0;

	meshByLocalName.del( localName );
}

void ZMesh::allocVertCopy() {
	if( vertCopy ) {
		free( vertCopy );
	}
	vertCopy = (ZMeshVertType *)malloc( sizeof(ZMeshVertType) * 3 * vertCount() );
}

void ZMesh::allocNormCopy() {
	if( normCopy ) {
		free( normCopy );
	}
	normCopy = (ZMeshVertType *)malloc( sizeof(ZMeshVertType) * 3 * normCount());
}

void ZMesh::allocVertWorld() {
	if( vertWorld ) {
		free( vertWorld );
	}
	vertWorld = (ZMeshVertType *)malloc( sizeof(ZMeshVertType) * 3 * vertCount() );
}

void ZMesh::allocNormWorld() {
	if( normWorld ) {
		free( normWorld );
	}
	normWorld = (ZMeshVertType *)malloc( sizeof(ZMeshVertType) * 3 * normCount());
}

int ZMesh::vertCount() {
	return desc->vertCount;
}

int ZMesh::normCount() {
	return desc->normCount;
}

int ZMesh::faceCount() {
	return desc->faceCount;
}

ZMeshVertType *ZMesh::verts() {
	if( vertCopy ) return vertCopy;
	return desc->verts();
}

ZMeshVertType *ZMesh::norms() {
	if( normCopy ) return normCopy;
	return desc->norms();
}

ZMeshVertType *ZMesh::worldVerts() {
	if( vertWorld ) return vertWorld;
	return desc->verts();
}

ZMeshVertType *ZMesh::worldNorms() {
	if( normWorld ) return normWorld;
	return desc->norms();
}

ZMeshFaceType *ZMesh::faces() {
	return desc->faces();
}

void ZMesh::constrainJoint() {
	float y, p, r;
	mat4ToEulerAnglesMax( matLocal, y, p, r );

	r = max( r, rotConstrainMin.x );
	y = max( y, rotConstrainMin.y );
	p = max( p, rotConstrainMin.z );

	r = min( r, rotConstrainMax.x );
	y = min( y, rotConstrainMax.y );
	p = min( p, rotConstrainMax.z );

	FVec3 trans = matLocal.getTrans();
	matLocal = rotate3D_EulerAnglesMax( y, p, r );
	matLocal.setTrans( trans );
}

void ZMesh::resetToRestMatLocal() {
	matLocal = desc->restMatLocal;
}

void ZMesh::detachFromParent() {
	if( parent ) {
		if( parent->headChild == this ) {
			parent->headChild = nextSib;
		}
		if( nextSib ) {
			nextSib->prevSib = prevSib;
		}
		if( prevSib ) {
			prevSib->nextSib = nextSib;
		}
		parent = 0;
		nextSib = 0;
		prevSib = 0;
	}
}

void ZMesh::attachTo( ZMesh *_parent ) {
	assert( !parent );
	parent = _parent;
	nextSib = parent->headChild;
	prevSib = 0;
	if( parent->headChild ) {
		parent->headChild->prevSib = this;
	}
	parent->headChild = this;
}

//////////////////////////////////////////////////////////////////////////
// Heirarchy

void zMeshTransformVectorArray( float *input, float *output, float *mat4x4, int numVerts, int inputStepInBytes, int outputStepInBytes ) {
	// This is duplicated from zmath to avoid depenedency

	float *vi = input;
	float *vo = output;
	int i;

	if( input == output ) {
		// If the input and output arrays are the same, a temporary copy must be made during the multiply
		for( i=0; i<numVerts; i++ ) {
			float _0 = mat4x4[0*4+0] * vi[0]  +  mat4x4[1*4+0] * vi[1]  +  mat4x4[2*4+0] * vi[2]  +  mat4x4[3*4+0];
			float _1 = mat4x4[0*4+1] * vi[0]  +  mat4x4[1*4+1] * vi[1]  +  mat4x4[2*4+1] * vi[2]  +  mat4x4[3*4+1];
			float _2 = mat4x4[0*4+2] * vi[0]  +  mat4x4[1*4+2] * vi[1]  +  mat4x4[2*4+2] * vi[2]  +  mat4x4[3*4+2];

			vo[0] = _0;
			vo[1] = _1;
			vo[2] = _2;

			vi = (float *)( (char *)vi + inputStepInBytes );
			vo = (float *)( (char *)vo + outputStepInBytes );
		}
	}
	else {
		// The arrays can be traversed more efficiently with a temp copy
		for( i=0; i<numVerts; i++ ) {
			vo[0] = mat4x4[0*4+0] * vi[0]  +  mat4x4[1*4+0] * vi[1]  +  mat4x4[2*4+0] * vi[2]  +  mat4x4[3*4+0];
			vo[1] = mat4x4[0*4+1] * vi[0]  +  mat4x4[1*4+1] * vi[1]  +  mat4x4[2*4+1] * vi[2]  +  mat4x4[3*4+1];
			vo[2] = mat4x4[0*4+2] * vi[0]  +  mat4x4[1*4+2] * vi[1]  +  mat4x4[2*4+2] * vi[2]  +  mat4x4[3*4+2];

			vi = (float *)( (char *)vi + inputStepInBytes );
			vo = (float *)( (char *)vo + outputStepInBytes );
		}
	}
}

void zMeshUpdateWorldVerts( ZMesh *root ) {
	root->allocVertWorld();
	root->allocNormWorld();

	zMeshTransformVectorArray( root->verts(), root->vertWorld, root->matWorld, root->vertCount() );
	FMat4 normMat = root->matWorld;
	normMat.setTrans( FVec3::Origin );
	zMeshTransformVectorArray( root->norms(), root->normWorld, normMat, root->normCount() );

	// RECURSE setup the world verts
	for( ZMesh *b = root->headChild; b; b=b->nextSib ) {
		zMeshUpdateWorldVerts( b );
	}
}

void zMeshUpdateWorldMats( ZMesh *root ) {
	// RECURSE setup the world mat from relative mats
	if( root->parent ) {
		root->matWorld = root->parent->matWorld;
		root->matWorld.cat( root->matLocal );
	}
	else {
		root->matWorld = root->matLocal;
	}

	for( ZMesh *b = root->headChild; b; b=b->nextSib ) {
		zMeshUpdateWorldMats( b );
	}
}

void zMeshResetHeirarchy( ZMesh *root ) {
//	root->matLocal = root->desc->restMatWorld;
//	for( ZMeshBone *b = root->headChild; b; b=b->nextSib ) {
//		zMeshBoneResetHeirarchy( b );
//	}
}

//////////////////////////////////////////////////////////////////////////
// Bone related functions

void zMeshIkSolve( ZMesh *effector, ZMesh *root, FVec3 goal ) {
	// The stub bones are not allowed to be effectors so we go up one
	if( !effector->headChild ) {
		effector = effector->parent;
	}

	for( int tries=0; tries<10; tries++ ) {

		// NOTE quality of solution at start
		FVec3 effectorToGoalStart = goal;
		effectorToGoalStart.sub( effector->headChild->matWorld.getTrans() );

		// ASCEND the bone chain from finger to shoulder
		for( ZMesh *curJoint=effector; ; curJoint=curJoint->parent ) {

			// ROTATE each joint so that the effector moves toward
			// the goal.  That is, you want to rotate the vector from
			// pivot to effector so that it is colinear with
			// the vector pivot to goal.

			// COMPUTE the world coord vector joint to effector
			FVec3 jointToEffector = effector->headChild->matWorld.getTrans();
			jointToEffector.sub( curJoint->matWorld.getTrans() );
			jointToEffector.normalize();

			// COMPUTE the world coord vector joint to goal
			FVec3 jointToGoal = goal;
			jointToGoal.sub( curJoint->matWorld.getTrans() );
			jointToGoal.normalize();

			float cosA = jointToGoal.dot( jointToEffector );
			if( cosA < 0.999 ) {
				// If the vectors are not co-linear then rotate to make them so.

				// CROSS product gives us the axis of rotation in world coords
				FVec3 cross = jointToGoal;
				cross.cross( jointToEffector );
				cross.normalize();
				cross.mul( -1.f );

				// PUT the angle of rotation into the coordinate
				// space of the bone itself by taking the inverse
				// This inv mat can covert the world axis of rotation into the local axis
				FMat4 invMat = curJoint->matWorld;
				invMat.setTrans( FVec3::Origin );
				invMat.transpose();
				cross = invMat.mul( cross );

				// ROTATE the joint toward the target
				float angle = acosf( cosA );
				curJoint->matLocal.cat( rotate3D( cross, angle ) );

				// CONSTRAIN joints
				curJoint->constrainJoint();

				// UPDATE the world positions of all bones down the chain.
				zMeshUpdateWorldMats( curJoint );
			}

			if( curJoint == root ) {
				break;
			}
		}

//		FVec3 effectorToGoalEnd = goal;
//		effectorToGoalEnd.sub( effector->endPosWorld );

//		FVec3 changeInSolution = effectorToGoalEnd;
//		changeInSolution.sub( effectorToGoalStart );
//		Ik_test = changeInSolution.mag();

// Next step: print out the delta of the change of solutoin
// Work out a way to detect when the solution is not converging and
// then try the approach from a completely random orientation.

//		if( effectorToGoalEnd.mag2() < 0.001 ) {
//			break;
//		}
	}
}

void zMeshDeformSkin( ZMesh *skinMesh, ZMesh *boneRoot ) {
	ZMeshSkinDesc *skinDesc = (ZMeshSkinDesc *)skinDescByName.getI( skinMesh->desc->name );
	assert( skinDesc );

	// MAKE a working copy of the verts destination buffer
	skinMesh->allocVertCopy();
	FVec3 *dst = (FVec3 *)skinMesh->verts();
	memset( dst, 0, sizeof(ZMeshVertType)*3*skinMesh->desc->vertCount );

	// FETCH pointers to the bones
	// @TODO: Cache
	int numBones = skinDesc->numBones;
	ZMesh **bones = (ZMesh **)alloca( sizeof(ZMesh **) * numBones );
	for( int i=0; i<numBones; i++ ) {
		bones[i] = zMeshFindChild( skinDesc->boneName(i), boneRoot );
		assert( bones[i] );
	}

	// TRAVERSE the weights, transforming them into the bone space and accumulate into the working copy
	int numWeights = skinDesc->numWeights;

	unsigned char *numBonesPerVert = skinDesc->numBonesPerVert(0);
	unsigned char *boneIdPerWeight = skinDesc->boneIdPerWeight(0);
	float *weight = skinDesc->weight(0);
	FVec3 *offsetVector = skinDesc->offsetVector(0);

	int bonesRemainForThisVert = *numBonesPerVert++;
	for( int w=0; w<numWeights; w++ ) {
		FVec3 t = bones[*boneIdPerWeight]->matWorld.mul( *offsetVector );
		t.mul( *weight );
		dst->add( t );

		bonesRemainForThisVert--;
		boneIdPerWeight++;
		weight++;
		offsetVector++;

		if( bonesRemainForThisVert == 0 ) {
			dst++;
			bonesRemainForThisVert = *numBonesPerVert++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// ANIMATION 

void zMeshAnimStart( char *animName, ZMesh *root, float fps, double startTime ) {
	zMeshAnimStop( root );
	ZMeshAnimController *cont = new ZMeshAnimController;

	cont->rootMesh = root;
	cont->animDesc = (ZMeshAnimDesc *)animDescByName.getI( animName );

	cont->startLocalMat = root->matLocal;

	assert( !strcmp(cont->animDesc->nodeName(0),root->desc->name) );
		// For now, we're just assuming that the first node is the root.
		// If this were ever not the case we'd have to go searching.

	cont->startAnimMatInverse = *(cont->animDesc->mat(0,0));
	cont->startAnimMatInverse.inverse();

	cont->fps = fps;
	cont->startTime = startTime;
	cont->nodes = (ZMesh **)malloc( sizeof(ZMesh *) * cont->animDesc->numNodes );
	for( int i=0; i<cont->animDesc->numNodes; i++ ) {
		cont->nodes[i] = zMeshFindChild( cont->animDesc->nodeName(i), root );
	}

	cont->debugIgnoreTime = 0;
	cont->debugFrame = 0;
	//cont->debugIgnoreTime = 1;

	char buf[32];
	sprintf( buf, "%d", (int)root );
	animControllerByRoot.putI( buf, (int)cont );
}

void zMeshAnimStart( char *animName, char *localName, float fps, double startTime ) {
	zMeshAnimStart( animName, (ZMesh *)meshByLocalName.getI(localName), fps, startTime );
}

void zMeshAnimStop( ZMesh *root ) {
	char buf[32];
	sprintf( buf, "%d", (int)root );
	ZMeshAnimController *cont = (ZMeshAnimController *)animControllerByRoot.getI( buf );
	if( cont ) {
		free( cont->nodes );
		delete cont;
		animControllerByRoot.del( buf );
	}
}

void zMeshAnimUpdate( ZMesh *root, double time ) {
	char buf[32];
	sprintf( buf, "%d", (int)root );
	ZMeshAnimController *cont = (ZMeshAnimController *)animControllerByRoot.getI( buf );
	if( cont ) {
		// FIGURE what frame we should be showing.  If

		int frame = 0;
		if( cont->debugIgnoreTime ) {
			frame = cont->debugFrame;
			cont->debugFrame++;
		}
		else {
			double duration = double(cont->animDesc->numFrames / cont->fps);
			frame = (int)((double)cont->animDesc->numFrames * (time - cont->startTime) / duration);
		}

		// CHECK for end of animation
		if( frame == cont->animDesc->numFrames ) {
			zMeshAnimStop( root );
			return;
		}

		for( int i=0; i<cont->animDesc->numNodes; i++ ) {
			if( cont->nodes[i] == root ) {
				root->matLocal.identity();
				root->matLocal.cat( cont->startLocalMat );
				root->matLocal.cat( cont->startAnimMatInverse );
				root->matLocal.cat( *(cont->animDesc->mat(i,frame)) );
			}
			else {
				cont->nodes[i]->matLocal = *(cont->animDesc->mat(i,frame));
			}
		}
	}
}

void zMeshAnimUpdateAll( double time ) {
	for( int i=0; i<animControllerByRoot.size(); i++ ) {
		char *key = animControllerByRoot.getKey(i);
		if( key ) {
			zMeshAnimUpdate( (ZMesh *)(atoi(key)), time );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// GL Rendering functions
// @TODO: May want to move all this into another file

#include "zgltools.h"

void zMeshGLRender( ZMesh *mesh, int recurse, int wireframe ) {
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glPushMatrix();
	glMultMatrixf( mesh->matLocal );
//	glMultMatrixf( mesh->matWorld );

	glEnable( GL_NORMALIZE );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	// SETUP vertex array
	glEnable( GL_VERTEX_ARRAY );
	glVertexPointer( 3, sizeof(ZMeshVertType)==sizeof(float)?GL_FLOAT:GL_DOUBLE, 0, mesh->verts() );

	// SETUP normal array
	glEnableClientState( GL_NORMAL_ARRAY );
	glNormalPointer( sizeof(ZMeshVertType)==sizeof(float)?GL_FLOAT:GL_DOUBLE, 0, mesh->norms() );

	int faceCount = mesh->faceCount();
	ZMeshFaceType *faces = mesh->faces();

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glColor3ub( 255, 0, 0 );
	glBegin( GL_POINTS );
		for( int _j=0; _j<mesh->vertCount(); _j++ ) {
			glArrayElement( _j );
		}
	glEnd();
	glPopAttrib();

	// @TODO: Build display lists caches?
	glBegin( wireframe ? GL_LINE_STRIP : GL_TRIANGLES );
		for( int j=0; j<faceCount; j++ ) {
			glArrayElement( faces[j*3+0] );
			glArrayElement( faces[j*3+1] );
			glArrayElement( faces[j*3+2] );
		}
	glEnd();
	glDisableClientState( GL_NORMAL_ARRAY );

//glPopMatrix();
	if( recurse ) {
		for( ZMesh *child=mesh->headChild; child; child=child->nextSib ) {
			zMeshGLRender( child, 1, wireframe );
		}
	}

	glPopMatrix();
	glPopAttrib();
}


//extern float Cald_rotObj;

void zMeshGLRenderFaceted( ZMesh *mesh, int recurse, int wireframe ) {
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glPushMatrix();
	glMultMatrixf( mesh->matLocal );

	glEnable( GL_NORMALIZE );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	// SETUP vertex array
	glEnable( GL_VERTEX_ARRAY );
	glVertexPointer( 3, sizeof(ZMeshVertType)==sizeof(float)?GL_FLOAT:GL_DOUBLE, 0, mesh->verts() );

	int faceCount = mesh->faceCount();
	ZMeshFaceType *faces = mesh->faces();
	ZMeshVertType *verts = mesh->verts();

	// @TODO: Build display lists caches?
	glBegin( wireframe ? GL_LINE_STRIP : GL_TRIANGLES );
		for( int j=0; j<faceCount; j++ ) {
			FVec3 faceNormal0( &verts[ faces[j*3+1]*3 ] );
			faceNormal0.sub( FVec3(&verts[ faces[j*3+0]*3 ]) );
			FVec3 faceNormal1( &verts[ faces[j*3+2]*3 ] );
			faceNormal1.sub( FVec3(&verts[ faces[j*3+0]*3 ]) );
			faceNormal0.cross( faceNormal1 );

			glNormal3fv( faceNormal0 );
			glArrayElement( faces[j*3+0] );
			glArrayElement( faces[j*3+1] );
			glArrayElement( faces[j*3+2] );
		}
	glEnd();


//glPopMatrix();

	if( recurse ) {
		for( ZMesh *child=mesh->headChild; child; child=child->nextSib ) {
			zMeshGLRenderFaceted( child, 1, wireframe );
		}
	}

	glPopMatrix();
	glPopAttrib();
}

void zMeshGLRenderVerts( ZMesh *mesh, int recurse ) {
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glPushMatrix();
	glMultMatrixf( mesh->matLocal );

	glEnable( GL_NORMALIZE );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	// SETUP vertex array
	glEnable( GL_VERTEX_ARRAY );
	glVertexPointer( 3, sizeof(ZMeshVertType)==sizeof(float)?GL_FLOAT:GL_DOUBLE, 0, mesh->verts() );

	int vertCount = mesh->vertCount();
	glBegin( GL_POINTS );
		for( int i=0; i<vertCount; i++ ) {
			glArrayElement( i );
		}
	glEnd();

	if( recurse ) {
		for( ZMesh *child=mesh->headChild; child; child=child->nextSib ) {
			zMeshGLRenderVerts( child, 1 );
		}
	}

	glPopMatrix();
	glPopAttrib();
}

void zMeshGLRenderNormals( ZMesh *mesh, int recurse, float len ) {
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glDisable( GL_LIGHTING );
	glPushMatrix();
	glMultMatrixf( mesh->matLocal );
//	glMultMatrixf( mesh->matWorld );

	glColor3ub( 0, 0, 100 );

	int vertCount = mesh->vertCount();
	glBegin( GL_LINES );
		assert( sizeof(ZMeshVertType)==sizeof(float) );
		FVec3 *verts = (FVec3 *)mesh->verts();
		FVec3 *norms = (FVec3 *)mesh->norms();
		for( int j=0; j<vertCount; j++ ) {
			glVertex3fv( verts[j] );
			FVec3 n = norms[j];
			n.mul( len );
			n.add( verts[j] );
			glVertex3fv( n );
		}
	glEnd();

//glPopMatrix();
	if( recurse ) {
		for( ZMesh *child=mesh->headChild; child; child=child->nextSib ) {
			zMeshGLRenderNormals( child, 1, len );
		}
	}

	glPopMatrix();
	glPopAttrib();
}

void zMeshGLRenderShadowVolume( ZMesh *mesh, FVec3 _light, float extrusionMultiplier, int recurse ) {
	// Given some geometry and a light source build an edge list to be extruded
	// verts is assumed to be a 3-d tightly packed list of floats
	// faces is assumed to be a list of vert numbers packed in triplets (triangles)
	// light is a 3-d float

	int faceCount = mesh->faceCount();

	ZMeshVertType *verts = mesh->worldVerts();
	ZMeshFaceType *faces = mesh->faces();

	glBegin( GL_TRIANGLES );
	for( int i=0; i<faceCount; i++ ) {
		FVec3 a( &verts[ faces[i*3+0] * 3 ] );
		FVec3 b( &verts[ faces[i*3+1] * 3 ] );
		FVec3 c( &verts[ faces[i*3+2] * 3 ] );

		FVec3 _a = a;
		FVec3 _b = b;
		FVec3 _c = c;

		_b.sub( a );
		_c.sub( a );
		_b.cross( _c );
		FVec3 _l( _light );
		_l.sub( a );
		if( _b.dot( _l ) > 0.f ) {
			_a = a;
			_b = b;
			_c = c;

			_a.sub( _light );
			_a.mul( extrusionMultiplier );
			_a.add( a );

			_b.sub( _light );
			_b.mul( extrusionMultiplier );
			_b.add( b );

			_c.sub( _light );
			_c.mul( extrusionMultiplier );
			_c.add( c );

			glVertex3fv(  c );
			glVertex3fv(  b );
			glVertex3fv( _b );

			glVertex3fv(  c );
			glVertex3fv( _b );
			glVertex3fv( _c );

			glVertex3fv(  a );
			glVertex3fv(  c );
			glVertex3fv( _c );

			glVertex3fv(  a );
			glVertex3fv( _c );
			glVertex3fv( _a );

			glVertex3fv(  b );
			glVertex3fv(  a );
			glVertex3fv( _a );

			glVertex3fv(  b );
			glVertex3fv( _a );
			glVertex3fv( _b );

			// top cap
			glVertex3fv(  a );
			glVertex3fv(  b );
			glVertex3fv(  c );

			// bot cap
			glVertex3fv( _a );
			glVertex3fv( _c );
			glVertex3fv( _b );

		}
	}
	glEnd();

	if( recurse ) {
		for( ZMesh *child=mesh->headChild; child; child=child->nextSib ) {
			zMeshGLRenderShadowVolume( child, _light, extrusionMultiplier, recurse );
		}
	}
}

