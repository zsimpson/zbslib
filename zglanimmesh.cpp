#include "zhashtable.h"
#include "zglanimmesh.h"
#include "zwinutil.h"

ZHashTable zglAnimMeshOpenFiles;

void ZGLAnimMeshFile::fillHashOfShapes( int frame, ZHashTable *hash ) {
	if( hash ) {
		ZGLAnimMeshDesc *d = getFrame( frame );
		for( int i=0; i<shapeCount; i++ ) {
			hash->putI( d->name, (int)d );
			d = d->next();
		}
	}
}

ZGLAnimMeshFile *zglAnimMeshOpen( char *filename ) {
	ZGLAnimMeshFile *file = (ZGLAnimMeshFile *)zglAnimMeshOpenFiles.getI( filename );
	if( !file ) {
		file = (ZGLAnimMeshFile *)winUtilMemoryMapFileAsReadonly( filename );
		zglAnimMeshOpenFiles.putI( filename, (int)file );
	}
	return file;
}

void zglAnimMeshClose( char *filename ) {
	ZGLAnimMeshFile *file = (ZGLAnimMeshFile *)zglAnimMeshOpenFiles.getI( filename );
	if( file ) {
		winUtilUnmapFile( file );
		zglAnimMeshOpenFiles.del( filename );
	}
}
