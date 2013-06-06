
#ifndef ZGRANNY_H
#define ZGRANNY_H

#include "granny.h"

// This is a simple interface into Granny based on the sample
// code provided by Casey.

struct ZGrannyTexture {
	char *name;
		// The name used for this textures when meshes reference it

	unsigned int textureHandle;
		// The handle GL gave me when I submitted the ZGrannyTexture image
};

struct ZGrannyMesh {
	granny_mesh *grannyMesh;
		// Granny's mesh information

	granny_mesh_binding *grannyBinding;
		// The binding between this mesh and the model its deformed with

	granny_mesh_deformer *grannyDeformer;
		// The deformer that I'm using to deform the ZGrannyMesh

	unsigned int displayList;
		// If it's a rigid ZGrannyMesh, the GL display list handle I generated
		// for it at startup

	int textureCount;
	ZGrannyTexture **textureReferences;
		// The textures used by this ZGrannyMesh
};

struct ZGrannyModel {
	float matrix[4][4];
		// The matrix that says where this ZGrannyModel should be placed in the world
	
	granny_model_instance *grannyInstance;
		// Granny's instance of this model

	granny_world_pose *worldPose;
		// The current world-space state of the model

	int meshCount;	  
	ZGrannyMesh *meshes;
		// The meshes that comprise this model
};

struct ZGrannyScene {
	granny_camera camera;
		// The current camera

	granny_file *loadedFile;
		// The Granny file I've loaded

	int textureCount;
	ZGrannyTexture *textures;
		// All the textures I've loaded

	int modelCount;
	ZGrannyModel *models;
		// All the models I've loaded

	int maxBoneCount;
	granny_local_pose *sharedLocalPose;
		// An animation blending buffer that I use for all models, since
		// it's contents don't need to be preserved
};



ZGrannyTexture *zGrannyFindTexture( ZGrannyScene *scene, granny_material *grannyMaterial );
void zGrannyCreateTexture( ZGrannyTexture *texture, granny_texture *grannyTexture );

void zGrannyCreateModel( ZGrannyModel *model, ZGrannyScene *inScene, granny_model *grannyModel );
void zGrannyCreateMesh( ZGrannyMesh *mesh, granny_mesh *grannyMesh, granny_model_instance *inModel, ZGrannyScene *inScene );

void zGrannyRenderModel( ZGrannyScene *inScene, ZGrannyModel *model );
void zGrannyRenderMesh( ZGrannyMesh *mesh, granny_pnt332_vertex *vertices );
void zGrannyRenderSkeleton( granny_skeleton *skeleton, granny_world_pose *worldPose );

ZGrannyScene *zGrannyCreateScene( char *filename );
void zGrannyShutdownScene( ZGrannyScene *scene );

void zGrannyRenderScene( ZGrannyScene *scene );

#endif

