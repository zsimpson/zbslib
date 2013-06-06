 // @ZBS {
//		*MODULE_NAME zgranny
//		+DESCRIPTION {
//			A wrapper around granny
//		}
//		*PORTABILITY win32
//		*SDK_DEPENDS granny
//		*REQUIRED_FILES zgranny.cpp zgranny.h
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
#include "GL/gl.h"
// STDLIB includes:
#include "string.h"
#include "assert.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zgranny.h"
// ZBSLIB includes:


void zGrannyCreateTexture( ZGrannyTexture *texture, granny_texture *grannyTexture ) {
	/* The name of the ZGrannyTexture is just the file name that
	   the ZGrannyTexture came from.  I'll use this later (in zGrannyFindTexture())
	   to match ZGrannyTexture references to the textures I create here. */
	texture->name = grannyTexture->FromFileName;

	// Loop over all the MIP levels and fill them.
	if( (grannyTexture->TextureType == GrannyColorMapTextureType) && (grannyTexture->ImageCount == 1) ) {
		// Grab the single image
		granny_texture_image *grannyImage = &grannyTexture->Images[0];

		int width = grannyTexture->Width;
		int height = grannyTexture->Height;
		if( grannyImage->MIPLevelCount > 0 ) {
			granny_texture_mip_level &GrannyMIPLevel = grannyImage->MIPLevels[0];

			glGenTextures(1 , &texture->textureHandle );
			if( texture->textureHandle ) {
				glBindTexture( GL_TEXTURE_2D, texture->textureHandle );

				int hasAlpha = GrannyTextureHasAlpha( grannyTexture );
				unsigned char *pixelBuffer = new unsigned char[ width * height * 4 ];
			
				/* GrannyGetImageInFormat takes any arbitrarily formatted
				   source ZGrannyTexture, including Bink-compressed textures, and
				   spits them out in the RGBA format of your choice.  In this
				   case we pick either RGBA8888 (if the ZGrannyTexture has alpha) or
				   BGR888, since that's how we submit the OpenGL ZGrannyTexture. */
				GrannyCopyTextureImage(
					grannyTexture, 0, 0,
					hasAlpha ? GrannyRGBA8888PixelFormat :
					GrannyRGB888PixelFormat,
					width, height,
					width * (hasAlpha ? 4 : 3),
					pixelBuffer
				);

				glTexImage2D(
					GL_TEXTURE_2D, 0, hasAlpha ? 4 : 3, width, height, 0,
					hasAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer
				);
		
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			
				delete [] pixelBuffer;
			}
		}
	}	 
	else {
		/* I don't handle cube maps or other whacky varieties of
		   ZGrannyTexture maps in this sample app, but if you wanted to,
		   you'd do it here. */
		assert(0);
		//ErrorMessage("This sample only works with single-image ColorMapTextureType textures");
	}
}

void zGrannyCreateModel( ZGrannyModel *model, ZGrannyScene *inScene, granny_model *grannyModel ) {
	/* First, I grab the bone count for the ZGrannyModel and check to see
	   if it's the biggest I've seen so far.  If it is, I record it.
	   This is used back in OSStartup() to determine how big to make
	   the shared local pose buffer. */
	int boneCount = grannyModel->Skeleton->BoneCount;
	if( inScene->maxBoneCount < boneCount ) {
		inScene->maxBoneCount = boneCount;
	}

	/* Now I ask Granny for two objects I'm going to use to animate
	   the ZGrannyModel.  The first is the granny_model_instance, which keeps
	   track of what animations are playing on the ZGrannyModel... */
	model->grannyInstance = GrannyInstantiateModel( grannyModel );
	
	/* ... the second is the granny_world_pose, which is a buffer that
	   stores the world-space matrices for all the bones of the ZGrannyModel
	   so that I can use them for rendering (or collision detection,
	   etc.) at any time. */
	model->worldPose = GrannyNewWorldPose( boneCount );

	/* Now, I loop through all the meshes in the ZGrannyModel and process
	   them. */
	model->meshCount = grannyModel->MeshBindingCount;
	model->meshes = new ZGrannyMesh[ model->meshCount ];
	for( int i = 0; i < model->meshCount; ++i ) {
		zGrannyCreateMesh(
			&model->meshes[i],
			grannyModel->MeshBindings[i].Mesh,
			model->grannyInstance, inScene
		);
	}
}

void zGrannyCreateMesh( ZGrannyMesh *mesh, granny_mesh *grannyMesh, granny_model_instance *inModel, ZGrannyScene *inScene ) {
	mesh->grannyMesh = grannyMesh;
	
	/* First I create the Granny binding for this ZGrannyMesh.	The binding
	   is basically a table that says what bone slots of the ZGrannyMesh go
	   with which bones of the ZGrannyModel.  It's used during rendering to
	   pull the correct matrices from the skeleton to use for
	   deformation (or just to load the correct single transform in
	   the case of rigid meshes). */
	granny_skeleton *skeleton = GrannyGetSourceSkeleton( inModel );
	mesh->grannyBinding = GrannyNewMeshBinding( grannyMesh, skeleton, skeleton );

	/* Because I use GL display lists for rigid meshes, before processing
	   the vertex data, I make sure I bind the textures.  This is to
	   make sure the zGrannyRenderMesh() call that's used to send down the
	   data for the display can access this ZGrannyMesh's ZGrannyTexture table. */
	mesh->textureCount = grannyMesh->MaterialBindingCount;
	mesh->textureReferences = new ZGrannyTexture *[ mesh->textureCount ];
	for( int i = 0; i < mesh->textureCount; ++i ) {
		mesh->textureReferences[i] = zGrannyFindTexture( inScene, grannyMesh->MaterialBindings[i].Material );
	}
	
	int vertexCount = GrannyGetMeshVertexCount( grannyMesh );
	
	/* Now, I process the vertex data. */
	if( GrannyMeshIsRigid(grannyMesh) ) {
		/* It's a rigid ZGrannyMesh, so I can store the whole thing in a 
		   (possibly card resident) display list. */
		granny_pnt332_vertex *tempVertices = new granny_pnt332_vertex[vertexCount];

		/* GrannyCopyMeshVertices can do arbitrary vertex format
		   conversion, so in this case I just request that, whatever
		   format the ZGrannyMesh is in, it gets converted to 3-float
		   position, 3-float normal, 2-float ZGrannyTexture coordinate
		   format.	I use the pre-defined layout
		   GrannyPNT332VertexLayout for this, but you can also define
		   your own, so you're not limited to Granny's presets if you
		   want to use other more fanciful vertex formats (more
		   ZGrannyTexture coordinates, tangent spaces, etc., can all be in
		   there). */
		GrannyCopyMeshVertices( grannyMesh, GrannyPNT332VertexType, tempVertices );

		mesh->displayList = glGenLists(1);
		if( mesh->displayList ) {
			glNewList( mesh->displayList, GL_COMPILE );

			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_NORMAL_ARRAY );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );

			zGrannyRenderMesh( mesh, tempVertices );
			
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_NORMAL_ARRAY );
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
			
			glEndList();
		}
		else {
			assert(0);
			//ErrorMessage( "Unable to allocate GL display list for ZGrannyMesh %s - this ZGrannyMesh will not render properly.", grannyMesh->Name );
		}
		
		delete [] tempVertices;

		// Since this is a rigid ZGrannyMesh, we won't need a deformer.
		mesh->grannyDeformer = 0;
	}
	else {
		/* ... and then I create a Granny deformer for this ZGrannyMesh.  I
		   ask for deformations for the position and normal in this
		   case, since that's all I've got, but I could also ask for
		   deformations of the tangent space if I was doing bump
		   mapping. */
		mesh->grannyDeformer = GrannyNewMeshDeformer(
			GrannyGetMeshVertexType(grannyMesh), GrannyPNT332VertexType,
			GrannyDeformPositionNormal
		);
		if( !mesh->grannyDeformer ) {
			assert(0);
			//ErrorMessage( "Granny didn't find a matching deformer for the vertex format used by ZGrannyMesh \"%s\".  This ZGrannyMesh won't be rendered properly.", grannyMesh->Name );
		}
	}
}

ZGrannyTexture *zGrannyFindTexture( ZGrannyScene *scene, granny_material *grannyMaterial ) {
	/* I ask Granny for the diffuse color ZGrannyTexture of this material,
	   if there is one.  For a more complicated shader system,
	   you would probably ask for more maps (like bump maps) and
	   maybe query extra properties like shininess and so on. */
	granny_texture *grannyTexture = GrannyGetMaterialTextureByType( grannyMaterial, GrannyDiffuseColorTexture );
	if( grannyTexture ) {
		// Look through all the textures in the file for a match-by-name.
		for(int i = 0; i < scene->textureCount; ++i ) {
			ZGrannyTexture &texture = scene->textures[i];
			if( strcmp(texture.name, grannyTexture->FromFileName) == 0 ) {
				return(&texture);
			}
		}
	}

	return(0);
}

void zGrannyRenderModel( ZGrannyScene *inScene, ZGrannyModel *model ) {
	/* Before I do any rendering, I enable the arrays for the vertex
	   format I'm using.  You could do this once for the entire app,
	   since I never render anything else, but I figured it'd me more
	   instructive to put it here where I actually do the rendering. */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	/* Since I'm going to need it constantly, I dereference the composite
	   transform buffer for the ZGrannyModel's current world-space pose.  This
	   buffer holds the transforms that move vertices from the position
	   in which they were modeled to their current position in world space. */
	granny_matrix_4x4 *compositeBuffer = GrannyGetWorldPoseComposite4x4Array( model->worldPose );

	// Render all the meshes.
	for( int i = 0; i < model->meshCount; ++i ) {
		ZGrannyMesh &mesh = model->meshes[i];

		// Dereference the index table that maps ZGrannyMesh bone indices to
		// bone indices in the ZGrannyModel.
		int *toBoneIndices = GrannyGetMeshBindingToBoneIndices( mesh.grannyBinding );

		// Load the ZGrannyMesh's vertices, or deform into a temporary
		// buffer and load those, depending on whether the ZGrannyMesh is
		// rigid or not.
		int vertexCount = GrannyGetMeshVertexCount( mesh.grannyMesh );
		if( GrannyMeshIsRigid(mesh.grannyMesh) ) {
			// It's a rigid ZGrannyMesh, so I use the display list we've
			// previously created for it.
			glPushMatrix();
			granny_matrix_4x4 *transform = &compositeBuffer[toBoneIndices[0]];
			glMultMatrixf( (GLfloat *)transform );

			glCallList( mesh.displayList );

			glPopMatrix();
		}
		else if( mesh.grannyDeformer ) {
			// Tell Granny to deform the vertices of the ZGrannyMesh with the
			// current world pose of the ZGrannyModel, and dump the results
			// into the mutable vertex buffer.
			granny_pnt332_vertex *vertices = (granny_pnt332_vertex *)alloca( sizeof(granny_pnt332_vertex) * vertexCount );
			GrannyDeformVertices(
				mesh.grannyDeformer, 
				toBoneIndices, (float *)compositeBuffer,
				vertexCount,
				GrannyGetMeshVertices( mesh.grannyMesh ),
				vertices
			);

			zGrannyRenderMesh( &mesh, vertices );
		}
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void zGrannyRenderMesh( ZGrannyMesh *mesh, granny_pnt332_vertex *vertices ) {
	glVertexPointer( 3, GL_FLOAT, sizeof(*vertices), vertices->Position );
	glNormalPointer( GL_FLOAT, sizeof(*vertices), vertices->Normal );
	glTexCoordPointer( 2, GL_FLOAT, sizeof(*vertices), vertices->UV );
		
	/* Now both the indices and vertices are loaded, so I can
	   render.	I grab the material groups and spin over them,
	   changing to the appropriate ZGrannyTexture and rendering each batch.
	   A more savvy rendering loop might have instead built a
	   sorted list of material groups to minimize ZGrannyTexture changes,
	   etc., but this is the most basic way to render. */
	int indexSize = GrannyGetMeshBytesPerIndex( mesh->grannyMesh );
	unsigned char *Indices = (unsigned char *)GrannyGetMeshIndices( mesh->grannyMesh );
	int groupCount = GrannyGetMeshTriangleGroupCount( mesh->grannyMesh );
	granny_tri_material_group *group = GrannyGetMeshTriangleGroups( mesh->grannyMesh );
	while( groupCount-- ) {
		if( group->MaterialIndex < mesh->textureCount ) {
			ZGrannyTexture *texture = mesh->textureReferences[group->MaterialIndex];
			if( texture ) {
				glBindTexture( GL_TEXTURE_2D, texture->textureHandle );
			}
		}
		else {
			glBindTexture( GL_TEXTURE_2D, 0 );
		}

		glDrawElements(
			GL_TRIANGLES,
			group->TriCount*3,
			(indexSize == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
			&Indices[group->TriFirst*3*indexSize]
		);

		++group;
	}
}

void zGrannyRenderSkeleton( granny_skeleton *skeleton, granny_world_pose *worldPose ) {
	int boneCount = skeleton->BoneCount;
	granny_bone *bones = skeleton->Bones;
	
	// Draw points at all the bone locations
	glColor3f( 1.0f, 1.0f, 1.0f );
	glBegin( GL_POINTS );
	for( int i = 0; i < boneCount; ++i ) {
		float *transform = GrannyGetWorldPose4x4( worldPose, i );
		glVertex3fv(&transform[12]);
	}
	glEnd();

	glBegin(GL_LINES);
	for( i = 0; i < boneCount; ++i ) {
		float *transform = GrannyGetWorldPose4x4( worldPose, i );

		// Draw a line connecting each child bone to its parent
		if( bones[i].ParentIndex != GrannyNoParentBone ) {
			float *parentTransform = GrannyGetWorldPose4x4( worldPose, bones[i].ParentIndex );

			glColor3f( 1.0f, 1.0f, 1.0f );
			glVertex3fv( &transform[12] );

			glColor3f( 0.7f, 0.7f, 0.7f );
			glVertex3fv( &parentTransform[12] );
		}

		// Draw a scaled axis system at each of the bones
		float scale = 10.0f;

		glColor3f( 1.0f, 0.0f, 0.0f );
		glVertex3fv( &transform[12] );
		glVertex3f( transform[12] + scale*transform[0], transform[13] + scale*transform[1], transform[14] + scale*transform[2] );
			
		glColor3f( 0.0f, 1.0f, 0.0f );
		glVertex3fv( &transform[12] );
		glVertex3f( transform[12] + scale*transform[4], transform[13] + scale*transform[5], transform[14] + scale*transform[6] );

		glColor3f( 0.0f, 0.0f, 1.0f );
		glVertex3fv( &transform[12] );
		glVertex3f( transform[12] + scale*transform[8], transform[13] + scale*transform[9], transform[14] + scale*transform[10] );
	}
	glEnd();
}

// TODO:
// I want to be able to import multiple files into a scene
// and then update and render that scene how I want
// I want to be able to fetch model pointers conveniently out of the scene
// and manipulate them bu name.


ZGrannyScene *zGrannyCreateScene( char *filename ) {
	ZGrannyScene *scene = new ZGrannyScene;
	memset( scene, 0, sizeof(*scene) );

	scene->loadedFile = GrannyReadEntireFile( filename );
	assert( scene->loadedFile );

	granny_file_info *fileInfo = GrannyGetFileInfo( scene->loadedFile );
//            GlobalScene.TextureCount = FileInfo->TextureCount;
//            GlobalScene.Textures = new texture[GlobalScene.TextureCount];
//            {for(int TextureIndex = 0;
//                 TextureIndex < GlobalScene.TextureCount;
//                 ++TextureIndex)
//            {
//                CreateTexture(&GlobalScene.Textures[TextureIndex],
//                              FileInfo->Textures[TextureIndex]);
//            }}            

	scene->modelCount = fileInfo->ModelCount;
	scene->models = new ZGrannyModel[scene->modelCount];
	memset( scene->models, 0, sizeof(ZGrannyModel)*scene->modelCount );
	for( int i = 0; i < scene->modelCount; ++i ) {
		// Create the model
		granny_model *grannyModel = fileInfo->Models[i];
		ZGrannyModel *model = &scene->models[i];
		zGrannyCreateModel( model, scene, grannyModel );

		GrannyGetModelInitialPlacement4x4( grannyModel, (float *)model->matrix );

		if( fileInfo->AnimationCount ) {
			granny_animation *animation = fileInfo->Animations[0];
			granny_control *control = GrannyPlayControlledAnimation( 0.0f, animation, model->grannyInstance );
			if( control ) {
				GrannySetControlLoopCount( control, 0 );
				GrannyFreeControlOnceUnused( control );
			}
		}

	}

// redo:
	scene->sharedLocalPose = GrannyNewLocalPose( scene->maxBoneCount );

	return scene;
}

void zGrannyShutdownScene( ZGrannyScene *scene ) {
	GrannyFreeFile( scene->loadedFile );
}

void zGrannyRenderScene( ZGrannyScene *scene ) {
	for( int i = 0; i < scene->modelCount; ++i ) {
		zGrannyRenderModel( scene, &scene->models[i] );
	}
}

