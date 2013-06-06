// @ZBS {
//		*MODULE_OWNER_NAME zglpbuffer
// }

#ifndef ZGLPBUFFER_H
#define ZGLPBUFFER_H

#include "GL/gl.h"
#include "GL/glu.h"

// Base class for offscreen rendering
// We can have different types of this active within the same program
class ZRenderBuffer
{
public:
  ZRenderBuffer(int width,int height) { this->width = width; this->height = height; }  
  virtual ~ZRenderBuffer() { };

  virtual void enable() = 0;
  virtual void disable() = 0;

    // Return the actual data.  Let the caller know if they have to delete it or not.
  virtual char *getBits(bool &isResponsible) = 0;

    int width;
    int height;
};

// Factory for render buffers, so we can hide Mesa support
class ZRenderBufferFactory
{
public:
	ZRenderBufferFactory();
	virtual ~ZRenderBufferFactory();

	virtual ZRenderBuffer *makeBuffer(int width,int height) = 0;

	// Override the default factory
	// Caller responsible for deletion
	static void setFactory(ZRenderBufferFactory *);

	// Return the current factory
	static ZRenderBufferFactory *getFactory();
};

// PBuffer version.  Works on Linux and windows with #ifdefs
struct ZGLPBufferData;
class ZGLPBuffer : public ZRenderBuffer {
public:

	#define ZGLPBUFFER_RGB 1
	#define ZGLPBUFFER_RGBA 2
	#define ZGLPBUFFER_DEPTH 4
	#define ZGLPBUFFER_STENCIL 8
	#define ZGLPBUFFER_FLOAT16 16
	#define ZGLPBUFFER_FLOAT32 32
	#define ZGLPBUFFER_MULTISAMPLE_2 64
	#define ZGLPBUFFER_MULTISAMPLE_4 128
    ZGLPBuffer( int width, int height, int flags = ZGLPBUFFER_RGBA | ZGLPBUFFER_DEPTH | ZGLPBUFFER_STENCIL );

    ~ZGLPBuffer();

    void enable();
    void disable();

	// mallocs memory and the RGBA bits into the buffer
    char *getBits(bool &isResponsible);

	// Copy into the texture
    void copyToTex( int texture );

    int flags;
    ZGLPBufferData *data;
};

#endif
