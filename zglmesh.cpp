#include "wingl.h"
#include "GL/gl.h"
#include "zglmesh.h"
#include "stdlib.h"

GLMesh::GLMesh() {
	vCount = 0;
	fCount = 0;
	verts = 0;
	faces = 0;
	norms = 0;
	hasUV = 0;
}

void GLMesh::alloc( int _vCount, int _fCount ) {
	vCount = _vCount;
	fCount = _fCount;
	verts = (float*)malloc( sizeof(float)*_vCount*3 );
	norms = (float*)malloc( sizeof(float)*_vCount*3 );
	uvcrd = (float*)malloc( sizeof(float)*_vCount*2 );
	faces = (unsigned short*)malloc( sizeof(unsigned short)*_fCount*3 );
}

void GLMesh::clear() {
	if( verts ) free( verts );
	if( norms ) free( norms );
	if( uvcrd ) free( uvcrd );
	if( faces ) free( faces );
	verts = norms = uvcrd = 0;
	faces = 0;
}

void GLMesh::render() {
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, verts );
	glEnableClientState( GL_NORMAL_ARRAY );
	glNormalPointer( GL_FLOAT, 0, norms );
	if( hasUV ) {
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, uvcrd );
	}

	glDrawElements( GL_TRIANGLES, fCount*3, GL_UNSIGNED_SHORT, faces );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void GLMesh::renderNorms( float len ) {
	glBegin( GL_LINES );
	for( int i=0; i<vCount; i++ ) {
		float v[3] = { verts[i*3+0], verts[i*3+1], verts[i*3+2] };
		float n[3] = { norms[i*3+0], norms[i*3+1], norms[i*3+2] };
		n[0]*=len;
		n[1]*=len;
		n[2]*=len;
		glVertex3fv( v );
		v[0]+=n[0];
		v[1]+=n[1];
		v[2]+=n[2];
		glVertex3fv( v );
	}
	glEnd();
}
