#ifndef ZGLEXTENSIONS
#define ZGLEXTENSIONS


extern int zglNumMultiTextures;
	// Loaded with glActiveTextureARB

#define GL_MAX_TEXTURES_UNITS_ARB           0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2

typedef void (APIENTRY * GLMULTITEXCOORD2FARBPROC) (GLenum target, GLfloat s, GLfloat t);
extern GLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

typedef void (APIENTRY * GLACTIVETEXTUREARBPROC) (GLenum target);
extern GLACTIVETEXTUREARBPROC glActiveTextureARB;

typedef void (APIENTRY * GLCLIENTACTIVETEXTUREARBPROC) (GLenum target);
extern GLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;

typedef void (APIENTRY * GLCOLORTABLEEXTPROC)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format,GLenum type, const GLvoid *data);
extern GLCOLORTABLEEXTPROC glColorTableEXT;


void zglUseExtension( char *funcName );
	// Loads the function address and asserts if it isn't supported

#endif


