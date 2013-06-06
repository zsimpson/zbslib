// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A platform independent wrapper around pbuffers
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES zglpbuffer.cpp zglpbuffer.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
#include "GL/glew.h"
	#include "GL/wglew.h"
	#include "wingl.h"
#else
	#include "GL/glx.h"
#endif
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
// MODULE includes:
#include "zglpbuffer.h"
// ZBSLIB includes:
#include "zbits.h"

// ZGLPBufferFactory
//----------------------------------------------------------------------------------------------------------

class ZGLPBufferFactory : public ZRenderBufferFactory
{
public:
	ZGLPBufferFactory() { };
	~ZGLPBufferFactory() { };

	ZRenderBuffer *makeBuffer(int width,int height)
	{
		return new ZGLPBuffer(width,height);
	}
};

ZGLPBufferFactory defaultFactory;

// ZRenderBufferFactory
//----------------------------------------------------------------------------------------------------------

ZRenderBufferFactory::ZRenderBufferFactory()
{
}

ZRenderBufferFactory::~ZRenderBufferFactory()
{
}

ZRenderBufferFactory *theFactory = &defaultFactory;
void ZRenderBufferFactory::setFactory(ZRenderBufferFactory *newFactory)
{
	theFactory = newFactory;
}

ZRenderBufferFactory *ZRenderBufferFactory::getFactory()
{
	return theFactory;
}


#ifdef WIN32

	// Windows
	//----------------------------------------------------------------------------------------------------------

	struct ZGLPBufferData {
		HDC hdc;
		HPBUFFERARB pbuffer;
		HGLRC context;
		HDC old_hdc;
		HGLRC old_context;
	};

        ZGLPBuffer::ZGLPBuffer( int width, int height, int flags ) : ZRenderBuffer(width,height) {
		this->flags = flags;

		HDC old_hdc = wglGetCurrentDC();
		HGLRC old_context = wglGetCurrentContext();

		int attrib[32];
		int attribCount = 0;

		attrib[attribCount++] = WGL_PIXEL_TYPE_ARB;
		attrib[attribCount++] = WGL_TYPE_RGBA_ARB;
		attrib[attribCount++] = WGL_DRAW_TO_PBUFFER_ARB;
		attrib[attribCount++] = 1;
		attrib[attribCount++] = WGL_SUPPORT_OPENGL_ARB;
		attrib[attribCount++] = 1;

		if( flags & ZGLPBUFFER_RGB || flags & ZGLPBUFFER_RGBA ) {
			attrib[attribCount++] = WGL_RED_BITS_ARB;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT16 ? 32 : 8;
			attrib[attribCount++] = WGL_GREEN_BITS_ARB;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT16 ? 32 : 8;
			attrib[attribCount++] = WGL_BLUE_BITS_ARB;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT16 ? 32 : 8;
			if( flags & ZGLPBUFFER_RGBA ) {
				attrib[attribCount++] = WGL_ALPHA_BITS_ARB;
				attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT16 ? 32 : 8;
			}
		}
		if( flags & ZGLPBUFFER_DEPTH ) {
			attrib[attribCount++] = WGL_DEPTH_BITS_ARB;
			attrib[attribCount++] = 24;
		}
		if( flags & ZGLPBUFFER_STENCIL ) {
			attrib[attribCount++] = WGL_STENCIL_BITS_ARB;
			attrib[attribCount++] = 8;
		}
		if( flags & ZGLPBUFFER_FLOAT16 ) {
			attrib[attribCount++] = WGL_FLOAT_COMPONENTS_NV;
			attrib[attribCount++] = 1;
		}
		if( flags & ZGLPBUFFER_MULTISAMPLE_2 || flags & ZGLPBUFFER_MULTISAMPLE_4 ) {
			attrib[attribCount++] = WGL_SAMPLE_BUFFERS_ARB;
			attrib[attribCount++] = 1;
			attrib[attribCount++] = WGL_SAMPLES_ARB;
			attrib[attribCount++] = flags & ZGLPBUFFER_MULTISAMPLE_2 ? 2 : 4;
		}
		attrib[attribCount++] = 0;

		HDC hdc;
		HPBUFFERARB pbuffer;
		HGLRC context;

//		try {
			if(!wglChoosePixelFormatARB) wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
			assert( wglChoosePixelFormatARB );

			if(!wglCreatePbufferARB) wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
			assert( wglCreatePbufferARB );

			if(!wglGetPbufferDCARB) wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
			assert( wglGetPbufferDCARB );

			if(!wglReleasePbufferDCARB) wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
			assert( wglReleasePbufferDCARB );

			if(!wglDestroyPbufferARB) wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
			assert( wglDestroyPbufferARB );

			int pixelformat;
			unsigned int count;
			wglChoosePixelFormatARB(old_hdc,&attrib[0],NULL,1,&pixelformat,&count);
			assert( count != 0 );

			int pattrib[] = { 0 };

			pbuffer = wglCreatePbufferARB(old_hdc,pixelformat,width,height,pattrib);
			assert( pbuffer );

			hdc = wglGetPbufferDCARB(pbuffer);
			assert( hdc );

			context = wglCreateContext(hdc);
			assert( context );

			int ret = wglShareLists( old_context, context );
			assert( ret );
			if(!wglShareLists(old_context,context)) throw("wglShareLists() failed");
//		}
//		catch( const char *error ) {
//			fprintf(stderr,"PBuffer::PBuffer(): %s\n",error);
//			hdc = old_hdc;
//			context = old_context;
//		}

		data = new ZGLPBufferData;
		data->hdc = hdc;
		data->pbuffer = pbuffer;
		data->context = context;
		data->old_hdc = old_hdc;
		data->old_context = old_context;
	}

	ZGLPBuffer::~ZGLPBuffer() {
		wglDeleteContext(data->context);
		wglReleasePbufferDCARB(data->pbuffer,data->hdc);
		wglDestroyPbufferARB(data->pbuffer);
		wglMakeCurrent(data->old_hdc,data->old_context);
		delete data;
	}

	void ZGLPBuffer::enable() {
		data->old_hdc = wglGetCurrentDC();
		data->old_context = wglGetCurrentContext();

		int ret = wglMakeCurrent(data->hdc,data->context);
		assert( ret );
//		if(!) {
//			fprintf(stderr,"PBuffer::disable(): wglMakeCurrent() failed\n");
//		}
	}

	void ZGLPBuffer::disable() {
		int ret = wglMakeCurrent(data->old_hdc,data->old_context);
		assert( ret );
//		if(!wglMakeCurrent(data->old_hdc,data->old_context)) {
//			fprintf(stderr,"PBuffer::disable(): wglMakeCurrent() failed\n");
//		}
	}

#else
	// LINUX
	//----------------------------------------------------------------------------------------------------------

	struct ZGLPBufferData {
		Display *display;
		GLXPbuffer pbuffer;
		GLXContext context;
		GLXPbuffer old_pbuffer;
		GLXContext old_context;
	};

        ZGLPBuffer::ZGLPBuffer(int width,int height,int flags) : ZRenderBuffer(width,height) {
		this->flags = flags;
		glBindTexture(GL_TEXTURE_2D,0);

		Display *display = glXGetCurrentDisplay();
		assert( display );
		int screen = DefaultScreen(display);
		GLXContext old_context = glXGetCurrentContext();

		int attrib[32];
		int attribCount = 0;
		attrib[attribCount++] = GLX_RENDER_TYPE;
		attrib[attribCount++] = GLX_RGBA_BIT;
		attrib[attribCount++] = GLX_DRAWABLE_TYPE;
		attrib[attribCount++] = GLX_PBUFFER_BIT;

		if(flags & ZGLPBUFFER_RGB || flags & ZGLPBUFFER_RGBA) {
			attrib[attribCount++] = GLX_RED_SIZE;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8);
			attrib[attribCount++] = GLX_GREEN_SIZE;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8);
			attrib[attribCount++] = GLX_BLUE_SIZE;
			attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8);

//			attrib.push_back(GLX_RED_SIZE);
//			attrib.push_back(flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8) );
//			attrib.push_back(GLX_GREEN_SIZE);
//			attrib.push_back(flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8) );
//			attrib.push_back(GLX_BLUE_SIZE);
//			attrib.push_back(flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8) );
			if(flags & ZGLPBUFFER_RGBA) {
				attrib[attribCount++] = GLX_ALPHA_SIZE;
				attrib[attribCount++] = flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8);
	
//				attrib.push_back(GLX_ALPHA_SIZE);
//				attrib.push_back(flags & ZGLPBUFFER_FLOAT32 ? 32 : (flags & ZGLPBUFFER_FLOAT16 ? 16 : 8) );
			}
		}
		if(flags & ZGLPBUFFER_DEPTH) {
			attrib[attribCount++] = GLX_DEPTH_SIZE;
			attrib[attribCount++] = 24;
//			attrib.push_back(GLX_DEPTH_SIZE);
//			attrib.push_back(24);
		}
		if(flags & ZGLPBUFFER_STENCIL) {
			attrib[attribCount++] = GLX_STENCIL_SIZE;
			attrib[attribCount++] = 8;
//			attrib.push_back(GLX_STENCIL_SIZE);
//			attrib.push_back(8);
		}
		if((flags & ZGLPBUFFER_FLOAT32) || (flags & ZGLPBUFFER_FLOAT16)) {
			attrib[attribCount++] = GLX_FLOAT_COMPONENTS_NV;
			attrib[attribCount++] = 1;
//			attrib.push_back(GLX_FLOAT_COMPONENTS_NV);
//			attrib.push_back(true);
		}
		if(flags & ZGLPBUFFER_MULTISAMPLE_2 || flags & ZGLPBUFFER_MULTISAMPLE_4) {
			attrib[attribCount++] = GLX_SAMPLE_BUFFERS_ARB;
			attrib[attribCount++] = 1;
			attrib[attribCount++] = GLX_SAMPLES_ARB;
			attrib[attribCount++] = flags & ZGLPBUFFER_MULTISAMPLE_2 ? 2 : 4;
//			attrib.push_back(GLX_SAMPLE_BUFFERS_ARB);
//			attrib.push_back(true);
//			attrib.push_back(GLX_SAMPLES_ARB);
//			attrib.push_back(flags & ZGLPBUFFER_MULTISAMPLE_2 ? 2 : 4);
		}
		attrib[attribCount++] = 0;
//		attrib.push_back(0);

//		std::vector<int> pattrib;
		int pattrib[32];
		int pattribCount = 0;
		pattrib[pattribCount++] = GLX_LARGEST_PBUFFER;
		pattrib[pattribCount++] = 1;
		pattrib[pattribCount++] = GLX_PRESERVED_CONTENTS;
		pattrib[pattribCount++] = 1;

//		pattrib.push_back(GLX_LARGEST_PBUFFER);
//		pattrib.push_back(true);
//		pattrib.push_back(GLX_PRESERVED_CONTENTS);
//		pattrib.push_back(true);

		GLXPbuffer pbuffer;
		GLXContext context;

		try {
			int count;
			GLXFBConfig *config;

//			const char *extensions = glXQueryExtensionsString(display,screen);

			pattrib[pattribCount++] = GLX_PBUFFER_WIDTH;
			pattrib[pattribCount++] = width;
			pattrib[pattribCount++] = GLX_PBUFFER_HEIGHT;
			pattrib[pattribCount++] = height;
			pattrib[pattribCount++] = 0;

//			pattrib.push_back(GLX_PBUFFER_WIDTH);
//			pattrib.push_back(width);
//			pattrib.push_back(GLX_PBUFFER_HEIGHT);
//			pattrib.push_back(height);
//			pattrib.push_back(0);

			config = glXChooseFBConfig(display,screen,&attrib[0],&count);
			assert( config );
//			if(!config) throw("glXChooseFBConfig() failed");

			pbuffer = glXCreatePbuffer(display,config[0],&pattrib[0]);
			assert( pbuffer );
//			if(!pbuffer) throw("glXCreatePbuffer() failed");

			context = glXCreateNewContext(display,config[0],GLX_RGBA_TYPE,old_context,true);
			assert( context );
//			if(!context) throw("glXCreateContextWithConfigSGIX() failed");
		}
		catch(const char *err) {
//			fprintf(stderr,"PBuffer::PBuffer(): %s\n",error);
//			pbuffer = glXGetCurrentDrawable();
//			context = old_context;
		}

		data = new ZGLPBufferData;
		data->display = display;
		data->pbuffer = pbuffer;
		data->context = context;
		data->old_pbuffer = glXGetCurrentDrawable();
		data->old_context = old_context;
	}

	ZGLPBuffer::~ZGLPBuffer() {
		if(data->context) glXDestroyContext(data->display,data->context);
		if(data->pbuffer) glXDestroyPbuffer(data->display,data->pbuffer);
		delete data;
	}

	void ZGLPBuffer::enable() {
		data->old_pbuffer = glXGetCurrentDrawable();
		data->old_context = glXGetCurrentContext();
		int ret = glXMakeCurrent(data->display,data->pbuffer,data->context);
		assert( ret );
//		if(!) {
//			fprintf(stderr,"PBuffer::enable(): glXMakeCurrent() failed\n");
//		}
	}

	void ZGLPBuffer::disable() {
		int ret = glXMakeCurrent(data->display,data->old_pbuffer,data->old_context);
		assert( ret );
//		if(!glXMakeCurrent(data->display,data->old_pbuffer,data->old_context)) {
//			fprintf(stderr,"PBuffer::disable(): glXMakeCurrent() failed\n");
//		}
	}

#endif


// Platform common
//----------------------------------------------------------------------------------------------------------

char *ZGLPBuffer::getBits(bool &isResponsible) {
	char *bits = (char *)malloc( width * height * 4 );
	glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bits );
	isResponsible = true;
	return bits;
}

void ZGLPBuffer::copyToTex( int texture ) {
	glBindTexture( GL_TEXTURE_2D, texture );

	assert( flags & ZGLPBUFFER_RGBA );
		// @TODO: Deal with the other attribute flags regarding format

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height );
}
