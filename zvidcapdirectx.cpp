// @ZBS {
//		*MODULE_NAME zvidcap plugin for DirectShow capture
//		*MASTER_FILE 1
//		*PORTABILITY win32
//		*SDK_DEPENDS dx8
//		*REQUIRED_FILES zvidcap.cpp zvidcap.h zvidcapdirectx.cpp
//		*VERSION 4.0
//		+HISTORY {
//			2.0 Revised to eliminate dependency on Microsoft's deprecated APIs.
//				Thanks to Jon Blow for the new improved way.
//			3.0 Revised how the modes enumerate themselves.  Now the
//              system can give back a list of modes and you can select on start.
//              Previously there was inernal black magic about how it
//              selected the mode, now the API can specify exactly.
//			4.0 Separated directx into a sepate file so that you could use
//				either the cmu1394 or the directx without the other
//		}
//		+TODO {
//          * Some sort of "pick the best mode" kind of function is probably inevitable
//          * Add a callback method thing for extrenally handling unsupported modes
//		}
//		+BUILDINFO {
//          This module requires the headers from DirectX\include
//          This module requires the following libraries
//			 * strmiids.lib (DirectX)
//			 * ole32.lib oleaut32.lib winmm.lib (win32)
//		}
//		*PUBLISH yes
//		*WIN32_LIBS_DEBUG strmiids.lib ole32.lib oleaut32.lib winmm.lib
//		*WIN32_LIBS_RELEASE strmiids.lib ole32.lib oleaut32.lib winmm.lib
// }
// OPERATING SYSTEM specific includes:
#include "strmif.h"
#include "vfwmsgs.h"
#include "control.h"
#include "uuids.h"
#include "amvideo.h"
#include "olectl.h"
#include "initguid.h"
DEFINE_GUID( CLSID_Vidcap_Filter, 0x17d93618, 0xe0a3, 0x4dde, 0x9b, 0x64, 0xea, 0x50, 0xa6, 0xfe, 0xa, 0x31);
	// The GUID for our DirectShow filter which grabs the frame
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
// MODULE includes:
#include "zvidcap.h"
// ZBSLIB includes:

extern bool zVidcapSkipFirstEnumeration;

namespace ZVidcapDirectX {

char *zVidcapDirectXDevices[zVidcapDevicesMax] = {0,};
int zVidcapDirectXNumDevices = 0;

char *zVidcapDirectXModes[zVidcapModesMax] = {0,};
int zVidcapDirectXNumModes = 0;

struct ZVidcapDirectXConnection {
	IGraphBuilder *graphBuilderIFACE;
	ICaptureGraphBuilder2 *captureGraphBuilderIFACE;
	IMediaControl *mediaControlIFACE;
	class ZVidcapFilter *filter;
	IBaseFilter *cameraFilter;
	IMediaSeeking *seekIFACE;
	IFilterGraph *filterGraphIFACE;
};
ZVidcapDirectXConnection vidcapConns[zVidcapDevicesMax] = {0,};
	// Holds onto all the DirectShow interfaces we need to accesss

int zVidcapDirectXCriticalSectionInitialized = 0;
CRITICAL_SECTION zVidcapDirectXCriticalSection;
	// A critical section is used to mutex locks on the double buffering system

// Utility
//------------------------------------------------------------------------------------------

DEFINE_GUID(MEDIASUBTYPE_I420,0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

static char *zVidcapDirectXMakeTypeString( AM_MEDIA_TYPE *media_type ) {
	char *type = 0;

	// These are ripped from uuids.h
		 if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_YVU9) ) type = "YVU9";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Y411) ) type = "Y411";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Y41P) ) type = "Y41P";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_YUY2) ) type = "YUY2";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_YVYU) ) type = "YVYU";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_UYVY) ) type = "UYVY";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Y211) ) type = "Y211";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_YV12) ) type = "YV12";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_CLJR) ) type = "CLJR";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_IF09) ) type = "IF09";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_CPLA) ) type = "CPLA";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MJPG) ) type = "MJPG";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_TVMJ) ) type = "TVMJ";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_WAKE) ) type = "WAKE";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_CFCC) ) type = "CFCC";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_IJPG) ) type = "IJPG";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Plum) ) type = "Plum";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_DVCS) ) type = "DVCS";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_DVSD) ) type = "DVSD";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB1) ) type = "RGB1";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB4) ) type = "RGB4";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB8) ) type = "RGB8";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB565) ) type = "RGB565";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB555) ) type = "RGB555";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB24) ) type = "RGB24";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB32) ) type = "RGB32";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Overlay) ) type = "Overlay";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1Packet) ) type = "MPEG1Packet";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1Payload) ) type = "MPEG1Payload";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1AudioPayload) ) type = "MPEG1AudioPayload";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1System) ) type = "MPEG1System";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1VideoCD) ) type = "MPEG1VideoCD";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1Video) ) type = "MPEG1Video";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_MPEG1Audio) ) type = "MPEG1Audio";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Avi) ) type = "Avi";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_QTMovie) ) type = "QTMovie";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_QTRpza) ) type = "QTRpza";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_QTSmc) ) type = "QTSmc";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_QTRle) ) type = "QTRle";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_QTJpeg) ) type = "QTJpeg";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_PCMAudio_Obsolete) ) type = "PCMAudio_Obsolete";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_PCM) ) type = "PCM";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_WAVE) ) type = "WAVE";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_AU) ) type = "AU";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_AIFF) ) type = "AIFF";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_dvsd) ) type = "dvsd";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_dvhd) ) type = "dvhd";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_dvsl) ) type = "dvsl";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Line21_BytePair) ) type = "Line21_BytePair";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Line21_GOPPacket) ) type = "Line21_GOPPacket";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_Line21_VBIRawData) ) type = "Line21_VBIRawData";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_DssVideo) ) type = "DssVideo";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_DssAudio) ) type = "DssAudio";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_VPVideo) ) type = "VPVideo";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_VPVBI) ) type = "VPVBI";
	else if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_I420) ) type = "I420";

	VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);

	int supported = 0;
	if(
 		   IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB24)
// 		|| IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB32)
		|| IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB8)
		|| IsEqualGUID(media_type->subtype, MEDIASUBTYPE_I420)
	) {
		supported = 1;
	}

	char buf[256];
	sprintf( buf, "%d %d %s %s", pVih->bmiHeader.biWidth, pVih->bmiHeader.biHeight, type?type:"unknown", supported?"supported":"" );

	return strdup( buf );
}

static HRESULT zVidcapDirectXGetUnconnectedPin(
	IBaseFilter *pFilter,	// Pointer to the filter.
	PIN_DIRECTION PinDir,	// Direction of the pin to find.
	IPin **ppPin			// Receives a pointer to the pin.
) {
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if( FAILED(hr) ) {
		return hr;
	}

	while (pEnum->Next(1, &pPin, NULL) == S_OK) {
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir) {
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr)) {
				// Already connected, not the pin we want.
				pTmp->Release();
			}
			else {
				// Unconnected, this is the pin we want.
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}


// VidCap Filter and Pin overloads.
// These are custom DirectShow filters which, although ridiculously complicated
// due to the complex DirectShow paradigm, do nothing more than
// copy memory from the input pin into a system buffer!
//------------------------------------------------------------------------------------------

class ZVidcapPin : public IMemInputPin, public IPin {
	class ZVidcapFilter *receiver;
  public:
	IPin *connected_to;
	IMemAllocator *allocator;

	AM_MEDIA_TYPE my_media_type;
		// @TODO: This is not used correctly.

	ZVidcapPin(ZVidcapFilter *receiver);
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	HRESULT BreakConnect();
	HRESULT SetMediaType(const AM_MEDIA_TYPE *media_type);
	HRESULT CheckMediaType(const AM_MEDIA_TYPE *media_type);
	HRESULT Active();
	HRESULT Inactive();
	STDMETHODIMP QueryId(LPWSTR *Id);
	STDMETHODIMP Receive(IMediaSample *media_sample);
	STDMETHODIMP Connect(IPin * pReceivePin, const AM_MEDIA_TYPE *media_type);
	STDMETHODIMP ReceiveConnection(IPin *connector, const AM_MEDIA_TYPE *media_type);
	STDMETHODIMP Disconnect();
	STDMETHODIMP ConnectedTo(IPin **pin_return);
	STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *media_type_return);
	STDMETHODIMP QueryPinInfo(PIN_INFO *pin_info_return);
	STDMETHODIMP QueryDirection(PIN_DIRECTION *pin_direction_return);
	STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **enumerator_return);
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *media_type);
	STDMETHODIMP QueryInternalConnections(IPin **pins_return, ULONG *num_pins_return);
	STDMETHODIMP EndOfStream();
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
	STDMETHODIMP SetSink(IQualityControl * piqc);
	STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);
	STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);
	STDMETHODIMP ReceiveMultiple (IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
	STDMETHODIMP ReceiveCanBlock();
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
	BOOL IsConnected();
	IPin *GetConnected();
};

class ZVidcapFilter : public IBaseFilter {
  public:
	ZVidcapFilter();
	~ZVidcapFilter();

  public:
	char *mode;
	int acceptRGB24;
	int width, height, stride, depth;
	char *incoming_image_buffers[2];
	int locks[2];
	int frameNums[2];
	int readLock;
	int frameTimings[16];
	unsigned int lastTime;
	const IUnknown *m_pUnknown;
	ZVidcapPin *input_pin;
	IFilterGraph *filter_graph;
	WCHAR *name;
	LONG reference_count;
	GUID mediaType;
	FILE *rawDumpFile;
		// If this is set, instead of capturing the data it writes the
		// raw bits to the open file.

	void setModeString( char *_mode );
	int getAvgFrameTimings();
	void endOfStream();
	HRESULT CheckMediaType(const AM_MEDIA_TYPE *media_type);
	HRESULT SetMediaType(const AM_MEDIA_TYPE *media_type);
	virtual HRESULT Receive(IMediaSample *incoming_sample);
	virtual HRESULT BreakConnect();
	STDMETHODIMP GetClassID(CLSID *class_id_return);
	STDMETHODIMP GetState(DWORD milliseconds, FILTER_STATE *state_return);
	STDMETHODIMP SetSyncSource(IReferenceClock *clock_return);
	STDMETHODIMP GetSyncSource(IReferenceClock **clock_return);
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP EnumPins(IEnumPins **enumerator_return);
	STDMETHODIMP FindPin(LPCWSTR name, IPin **pin_return);
	STDMETHODIMP QueryFilterInfo(FILTER_INFO * pInfo);
	STDMETHODIMP JoinFilterGraph(IFilterGraph *graph, LPCWSTR desired_name);
	STDMETHODIMP QueryVendorInfo(LPWSTR *vendor_info_return);
	STDMETHODIMP Register();
	STDMETHODIMP Unregister();
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
};

inline HRESULT zVidcapGetInterface(IUnknown *unknown, void **result) {
	*result = unknown;
	unknown->AddRef();
	return NOERROR;
}

class ZVidcapMediaTypeEnumerator : public IEnumMediaTypes {
	int index;
	ZVidcapPin *my_pin;
	LONG version;
	LONG reference_count;
  public:
	ZVidcapMediaTypeEnumerator(ZVidcapPin *my_pin, ZVidcapMediaTypeEnumerator *enumerator_to_copy);
	virtual ~ZVidcapMediaTypeEnumerator();
	STDMETHODIMP QueryInterface(REFIID riid, void **result);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP Next(ULONG num_media_types, AM_MEDIA_TYPE **media_types_return, ULONG *num_fetched_return);
	STDMETHODIMP Skip(ULONG numMediaTypes);
	STDMETHODIMP Reset();
	STDMETHODIMP Clone(IEnumMediaTypes **enumerator_return);
};

ZVidcapPin::ZVidcapPin(ZVidcapFilter *_receiver) {
	receiver = _receiver;
	connected_to = NULL;
	allocator = NULL;
}

STDMETHODIMP ZVidcapPin::QueryInterface(REFIID riid, void **result) {
	if (riid == IID_IMemInputPin) {
		return zVidcapGetInterface((IMemInputPin *)this, result);
	}

	if (riid == IID_IPin) {
		return zVidcapGetInterface((IPin *)this, result);
	}

	if (riid == IID_IUnknown) {
		return zVidcapGetInterface((IUnknown *)(IPin *)this, result);
	}

	*result = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) ZVidcapPin::AddRef() {  
	return receiver->AddRef();	
};								  

STDMETHODIMP_(ULONG) ZVidcapPin::Release() {  
	return receiver->Release(); 
};

STDMETHODIMP ZVidcapPin::Receive(IMediaSample *pSample) {
	HRESULT hr = receiver->Receive(pSample);
	return hr;
}

HRESULT ZVidcapPin::BreakConnect() {
	HRESULT hr = receiver->BreakConnect();
	return hr;
}

STDMETHODIMP ZVidcapPin::QueryId(LPWSTR *Id) {
	LPCWSTR src = receiver->name;
	LPWSTR *result = Id;		

	DWORD name_len = sizeof(WCHAR) * (lstrlenW(src) + 1);

	LPWSTR dest = (LPWSTR)CoTaskMemAlloc(name_len);
	if (dest == NULL) return E_OUTOFMEMORY;

	*result = dest;
	CopyMemory(dest, src, name_len);

	return NOERROR;
}

HRESULT ZVidcapPin::CheckMediaType(const AM_MEDIA_TYPE *media_type) {
	return receiver->CheckMediaType(media_type);
}

HRESULT ZVidcapPin::Active() {
	return TRUE;
}

HRESULT ZVidcapPin::Inactive() {
	return FALSE;
}

HRESULT ZVidcapPin::SetMediaType(const AM_MEDIA_TYPE *media_type) {
	return receiver->SetMediaType(media_type);
}

STDMETHODIMP ZVidcapPin::Connect(IPin *receive_pin, const AM_MEDIA_TYPE *media_type) {
	return E_NOTIMPL;
}

STDMETHODIMP ZVidcapPin::ReceiveConnection(IPin *connector_pin, const AM_MEDIA_TYPE *media_type) {
	// The graph builder (or something) calls this function over
	// and over for each media type until something returns success

	// Respond to a connection that an output pin is making.
	if( connected_to ) return VFW_E_ALREADY_CONNECTED;

	// See whether media type is OK.
	HRESULT result = receiver->CheckMediaType( media_type );
		// This just turns around and asks the filter to check the media type
		// Its all very confusing which is doing the checking, because
		// in our simple filter there's only one pin so it's the same
		// thing as asking the filter.
		//
		// Our filter keeps track of what the requested mode is so
		// it returns an S_OK only in the case that it has found the mode

	if( result != NOERROR ) {
		BreakConnect();

		if( SUCCEEDED(result) || result==E_FAIL || result==E_INVALIDARG ) {
			result = VFW_E_TYPE_NOT_ACCEPTED;
		}

		return result;
	}

	// Complete the connection.

	connected_to = connector_pin;
	connected_to->AddRef();
	result = SetMediaType(media_type);

	if( !SUCCEEDED(result) ) {
		connected_to->Release();
		connected_to = NULL;

		BreakConnect();
		return result;
	}

	return NOERROR;
}

STDMETHODIMP ZVidcapPin::Disconnect() {
	if (!connected_to) return S_FALSE;

	HRESULT result = BreakConnect();
	if (FAILED(result)) return result;

	connected_to->Release();
	connected_to = NULL;

	return S_OK;
}

STDMETHODIMP ZVidcapPin::ConnectedTo(IPin **pin_return) {
	IPin *pin = connected_to;
	*pin_return = pin;

	if (pin == NULL) return VFW_E_NOT_CONNECTED;

	pin->AddRef();
	return S_OK;
}

STDMETHODIMP ZVidcapPin::ConnectionMediaType(AM_MEDIA_TYPE *media_type) {
	assert( 0 );
		// I don't think that this code is correct but I also don't
		// think it is ever called so I'm just asserting here for now.
		// Specifically, my_media_type is never set so this is bogus
		// and I'm not sure where and how a copy of the media type should be made.

	if (IsConnected()) {
		AM_MEDIA_TYPE *dest = media_type;
		AM_MEDIA_TYPE *source = &my_media_type;

		assert(source != dest);

		*dest = *source;
		if (source->cbFormat != 0) {
			assert(source->pbFormat != NULL);
			dest->pbFormat = (PBYTE)CoTaskMemAlloc(source->cbFormat);
			if (dest->pbFormat == NULL) {
				dest->cbFormat = 0;
				assert( 0 );
				return VFW_E_NOT_CONNECTED;
			}
			else {
				CopyMemory((PVOID)dest->pbFormat, (PVOID)source->pbFormat, dest->cbFormat);
			}
		}

		if (dest->pUnk != NULL) dest->pUnk->AddRef();
		return S_OK;
	}
	else {
		memset(media_type, 0, sizeof(*media_type));
		return VFW_E_NOT_CONNECTED;
	}
}

STDMETHODIMP ZVidcapPin::QueryPinInfo(PIN_INFO *pin_info_return) {
	pin_info_return->pFilter = receiver;
	if (receiver) receiver->AddRef();

	lstrcpynW( pin_info_return->achName, receiver->name, sizeof(pin_info_return->achName) / sizeof(WCHAR));

	pin_info_return->dir = PINDIR_INPUT;

	return NOERROR;
}

STDMETHODIMP ZVidcapPin::QueryDirection(PIN_DIRECTION *direction_return) {
	*direction_return = PINDIR_INPUT;
	return NOERROR;
}

STDMETHODIMP ZVidcapPin::QueryAccept(const AM_MEDIA_TYPE *media_type) {
	HRESULT result = CheckMediaType(media_type);
	if (FAILED(result)) return S_FALSE;

	return result;
}

STDMETHODIMP ZVidcapPin::EnumMediaTypes(IEnumMediaTypes **enumerator_return) {
	*enumerator_return = new ZVidcapMediaTypeEnumerator(this, NULL);
	return S_OK;
}

STDMETHODIMP ZVidcapPin::EndOfStream(void) {
	if( receiver ) receiver->endOfStream();
	return S_OK;
}

STDMETHODIMP ZVidcapPin::SetSink(IQualityControl * piqc) {
	return NOERROR;
}

STDMETHODIMP ZVidcapPin::Notify(IBaseFilter *sender, Quality quality) {
	UNREFERENCED_PARAMETER(quality);
	UNREFERENCED_PARAMETER(sender);
	return E_NOTIMPL;
}

STDMETHODIMP ZVidcapPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
	return S_OK;
}

STDMETHODIMP ZVidcapPin::BeginFlush() {
	return S_OK;
}

STDMETHODIMP ZVidcapPin::EndFlush(void) {
	return S_OK;
}

STDMETHODIMP ZVidcapPin::ReceiveCanBlock() {
	return S_FALSE;
}

STDMETHODIMP ZVidcapPin::ReceiveMultiple(IMediaSample **samples, long num_samples, long *num_processed_return) {
	HRESULT result = S_OK;
	int num_processed = 0;

	while (num_samples-- > 0) {
		 result = Receive(samples[num_processed]);

		 if (result != S_OK) break;

		 num_processed++;
	}

	*num_processed_return = num_processed;
	return result;
}

STDMETHODIMP ZVidcapPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps) {
	return E_NOTIMPL;
}

STDMETHODIMP ZVidcapPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly) {
	if( allocator ) allocator->Release();
	allocator = NULL;
	allocator = pAllocator;
	pAllocator->AddRef();
	return NOERROR;
}

STDMETHODIMP ZVidcapPin::GetAllocator(IMemAllocator **allocator_return) {
	if (allocator == NULL) {
		HRESULT result = CoCreateInstance(CLSID_MemoryAllocator, 0, CLSCTX_INPROC_SERVER,IID_IMemAllocator, (void **)&allocator);
		if (FAILED(result)) return result;
	}

	assert(allocator != NULL);
	*allocator_return = allocator;
	allocator->AddRef();
	return NOERROR;
}

STDMETHODIMP ZVidcapPin::QueryInternalConnections(IPin **pins_return, ULONG *num_pins_return) {
	return E_NOTIMPL;
}

BOOL ZVidcapPin::IsConnected() {
	return (connected_to != NULL); 
}

IPin *ZVidcapPin::GetConnected() {
	return connected_to;
}

ZVidcapMediaTypeEnumerator::ZVidcapMediaTypeEnumerator(ZVidcapPin *pPin, ZVidcapMediaTypeEnumerator *pEnumMediaTypes) {
	assert(pPin != NULL);

	reference_count = 1;
	my_pin = pPin;
	my_pin->AddRef();

	if (pEnumMediaTypes != NULL) {
		// This is a copy of another enumerator.
		index = pEnumMediaTypes->index;
		version = pEnumMediaTypes->version;
		return;
	}

	// This is a new enumerator, not a copy.
	index = 0;
	version = 0;
}

ZVidcapMediaTypeEnumerator::~ZVidcapMediaTypeEnumerator() {
	my_pin->Release();
}


STDMETHODIMP ZVidcapMediaTypeEnumerator::QueryInterface(REFIID riid, void **result) {
	if (riid == IID_IEnumMediaTypes || riid == IID_IUnknown) {
		return zVidcapGetInterface((IEnumMediaTypes *) this, result);
	}
	else {
		*result = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) ZVidcapMediaTypeEnumerator::AddRef() {
	return InterlockedIncrement(&reference_count);
}

STDMETHODIMP_(ULONG) ZVidcapMediaTypeEnumerator::Release() {
	ULONG cRef = InterlockedDecrement(&reference_count);
	if (cRef == 0) {
		delete this;
	}
	return cRef;
}

STDMETHODIMP ZVidcapMediaTypeEnumerator::Clone(IEnumMediaTypes **enumerator_return) {
	HRESULT result = NOERROR;

	*enumerator_return = new ZVidcapMediaTypeEnumerator(my_pin, this);
	if (*enumerator_return == NULL) result =  E_OUTOFMEMORY;

	return result;
}

STDMETHODIMP ZVidcapMediaTypeEnumerator::Next(ULONG num_media_types, AM_MEDIA_TYPE **media_types_return, ULONG *num_fetched_return) {
	// We don't have to enumerate media types; only output
	// pins need to do this.
	*num_fetched_return = 0;
	return S_FALSE;
}

STDMETHODIMP ZVidcapMediaTypeEnumerator::Skip(ULONG num_to_skip) {
	// Since we know we only have one media type, this is
	// real simple.
	const int NUM_TYPES = 1;
	ULONG types_left = NUM_TYPES - index;
	if (num_to_skip > types_left) return S_FALSE;

	index += num_to_skip;
	return S_OK;
}

STDMETHODIMP ZVidcapMediaTypeEnumerator::Reset() {
	index = 0;
	version = 0;
	return NOERROR;
}


class ZVidcapPinEnumerator : public IEnumPins {
  public:
	ZVidcapPinEnumerator(ZVidcapFilter *receiver, ZVidcapPinEnumerator *enumerator_to_copy);
	virtual ~ZVidcapPinEnumerator();

	int index;
	ZVidcapFilter *receiver;
	LONG reference_count;

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **result);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IEnumPins
	STDMETHODIMP Next(ULONG num_pins, IPin **pins_return, ULONG *num_fetched_return);

	STDMETHODIMP Skip(ULONG num_to_skip);
	STDMETHODIMP Reset();
	STDMETHODIMP Clone(IEnumPins **result);
	STDMETHODIMP Refresh();
};

#pragma warning(disable:4355)
ZVidcapFilter::ZVidcapFilter() : m_pUnknown( reinterpret_cast<IUnknown *>(this) ) {
	reference_count = 1;
	name = NULL;
	filter_graph = NULL;
	input_pin = NULL;

	mode = 0;
	acceptRGB24 = 0;

	width = 0;
	height = 0;
	stride = 0;

	incoming_image_buffers[0] = 0;
	incoming_image_buffers[1] = 0;
	locks[0] = 0;
	locks[1] = 0;
	frameNums[0] = 0;
	frameNums[1] = 0;
	readLock = -1;

	memset( frameTimings, 0, sizeof(frameTimings) );
	lastTime = 0;
	rawDumpFile = 0;

	input_pin = new ZVidcapPin(this);
}

ZVidcapFilter::~ZVidcapFilter() {
	if (incoming_image_buffers[0]) delete [] incoming_image_buffers[0];
	if (incoming_image_buffers[1]) delete [] incoming_image_buffers[1];
	if( mode ) free( mode );
}

void ZVidcapFilter::setModeString( char *_mode ) {
	if( mode ) free( mode );
	mode = 0;
	if( _mode ) {
		mode = strdup( _mode );
	}
}

int ZVidcapFilter::getAvgFrameTimings() {
	int i;
	unsigned int sum = 0;
	for( i=0; i<sizeof(frameTimings)/sizeof(frameTimings[0]); i++ ) {
		sum += frameTimings[i];
	}
	return int( sum / (unsigned)i );
}

STDMETHODIMP ZVidcapFilter::QueryInterface(REFIID riid, void **result) {
	if (riid == IID_IBaseFilter) {
		return zVidcapGetInterface((IBaseFilter *) this, result);
	}
	else if (riid == IID_IMediaFilter) {
		return zVidcapGetInterface((IMediaFilter *) this, result);
	}
	else if (riid == IID_IPersist) {
		return zVidcapGetInterface((IPersist *) this, result);
	}
	else if (riid == IID_IAMovieSetup) {
		return zVidcapGetInterface((IAMovieSetup *) this, result);
	}
	else if (riid == IID_IUnknown) {
		return zVidcapGetInterface((IUnknown *) this, result);
	}
	else {
		*result = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) ZVidcapFilter::AddRef() {	
	return InterlockedIncrement(&reference_count);
};								  

STDMETHODIMP_(ULONG) ZVidcapFilter::Release() {  
	ULONG cRef = InterlockedDecrement(&reference_count);
	if (cRef == 0) {
		delete this;
	}

	return cRef;
};

HRESULT ZVidcapFilter::CheckMediaType( const AM_MEDIA_TYPE *media_type ) {
	if( media_type->formattype != FORMAT_VideoInfo ) {
		return S_FALSE;
	}

	if( !IsEqualGUID(media_type->majortype, MEDIATYPE_Video) ) {
		return S_FALSE;
	}

	char *type = zVidcapDirectXMakeTypeString( (AM_MEDIA_TYPE *)media_type );

	int ret = S_FALSE;

	if( acceptRGB24 ) {
		if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB24) ) {
			return S_OK;
		}
	}

	if( !mode ) {
		char *type = zVidcapDirectXMakeTypeString( (AM_MEDIA_TYPE *)media_type );

		// CHECK for duplicates
		int dup = 0;
		for( int i=0; i<zVidcapDirectXNumModes; i++ ) {
			if( !strcmp(zVidcapDirectXModes[i],type) ) {
				dup = 1;
				break;
			}
		}

		// Add the mode to the mode list if not a duplicate
		if( !dup ) {
			zVidcapDirectXModes[zVidcapDirectXNumModes++] = strdup( type );
		}
	}
	else {
		// There is a request for a specific mode

		// TRIM off just the first three words to eliminate "supported", etc
		char *copy[2];
		copy[0] = strdup( mode );
		copy[1] = strdup( type );
	
		for( int i=0; i<2; i++ ) {
			int count = 0;
			for( char *c = copy[i]; *c; c++ ) {
				if( *c == ' ' || *c == 0 ) {
					if( ++count==3 ) {
						*c = 0;
						break;
					}
				}
			}
		}

		if( !strcmp(copy[0],copy[1]) ) {
			ret = S_OK;
		}

		free( copy[0] );
		free( copy[1] );
	}

	return ret;
}

HRESULT ZVidcapFilter::SetMediaType(const AM_MEDIA_TYPE *media_type) {
	// Set up out OUTPUT buffers.  Note that we always
	// supply either 24 bit RGB or 8 bit BW.  Everything else
	// is decoded into these formats

	VIDEOINFO *pviBmp;
	pviBmp = (VIDEOINFO *)media_type->pbFormat;
	width  = pviBmp->bmiHeader.biWidth;
	height = abs(pviBmp->bmiHeader.biHeight);
	depth = pviBmp->bmiHeader.biBitCount / 8;
	if( depth == 4 && IsEqualGUID(media_type->subtype, MEDIASUBTYPE_RGB32) ) {
		depth = 3;
	}
	if( IsEqualGUID(media_type->subtype, MEDIASUBTYPE_UYVY) ) {
		depth = 1;
	}
	stride = (width * depth + 3) & ~(3);

	mediaType = media_type->subtype;

	if( incoming_image_buffers[0] ) {
		delete incoming_image_buffers[0];
	}
	if( incoming_image_buffers[1] ) {
		delete incoming_image_buffers[1];
	}
	incoming_image_buffers[0] = new char[width * height * depth];
	incoming_image_buffers[1] = new char[width * height * depth];

	locks[0] = 0;
	locks[1] = 0;
	frameNums[0] = 0;
	frameNums[1] = 0;
	readLock = -1;

	return S_OK;
}

HRESULT ZVidcapFilter::BreakConnect() {
	if (input_pin->IsConnected() == FALSE) {
		return S_FALSE;
	}

	return NOERROR;
}

void ZVidcapFilter::endOfStream() {
	if( rawDumpFile ) {
		fclose( rawDumpFile );
		rawDumpFile = 0;
	}
}

HRESULT ZVidcapFilter::Receive(IMediaSample *incoming_sample) {
	BYTE  *input_image;

	incoming_sample->GetPointer(&input_image);
	int bpp = depth;

	if( rawDumpFile ) {
		// This is a hacky mode that just lets us stupidly convert video
		// to a raw file that is then easy to parse frame by frame.

		assert( IsEqualGUID(mediaType, MEDIASUBTYPE_RGB24) );

		char *dest_line_start = incoming_image_buffers[0];
		BYTE *input_line_start = input_image + (height-1)*stride;
		assert( bpp == 3 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			int i;
			for (i = 0; i < width; i++) {
				dest[0] = src[2];
				dest[1] = src[1];
				dest[2] = src[0];
			
				src += 3;
				dest += bpp;
			}

			input_line_start -= stride;
			fwrite( dest_line_start, width * bpp, 1, rawDumpFile );
			fflush( rawDumpFile );
		}

		return NOERROR;
	}

	// DECIDE on which buffer.	Pick the oldest one that isn't locked
	EnterCriticalSection( &zVidcapDirectXCriticalSection );

	int which = 0;
	if( !locks[0] && !locks[1] ) {
		// Both are unlocked, pick the older
		which = frameNums[0] < frameNums[1] ? 0 : 1;
	}
	else if( locks[0] && locks[1] ) {
		// Both locked, do nothing
		LeaveCriticalSection( &zVidcapDirectXCriticalSection );
		return NULL;
	}
	else {
		// One is unlocked, other locked.  Pick the unlocked one.
		which = !locks[0] ? 0 : 1;
	}

	int nextFrameNum = max( frameNums[0], frameNums[1] ) + 1;

	char *dest_line_start = incoming_image_buffers[which];
	locks[which] = 1;

	LeaveCriticalSection( &zVidcapDirectXCriticalSection );

	int numTimingSlots = sizeof(frameTimings) / sizeof(frameTimings[0]);
	unsigned int now = timeGetTime();
	frameTimings[nextFrameNum % numTimingSlots] = now - lastTime;
	lastTime = now;

	if( IsEqualGUID(mediaType, MEDIASUBTYPE_I420) ) {
		BYTE *input_line_start = input_image;
		assert( bpp==1 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			memcpy( dest, src, width );
			input_line_start += stride;
			dest_line_start += width * bpp;
		}
	}

	else if( IsEqualGUID(mediaType, MEDIASUBTYPE_RGB8) ) {
		BYTE *input_line_start = input_image + (height-1)*stride;
		assert( bpp==1 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			memcpy( dest, src, width );
			input_line_start -= stride;
			dest_line_start += width * bpp;
		}
	}

	else if( IsEqualGUID(mediaType, MEDIASUBTYPE_UYVY) ) {
		BYTE *input_line_start = input_image + (height-1)*width*2;//stride;
		assert( bpp == 1 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			int i;
			for (i = 0; i < width; i++) {
				dest[0] = src[0];
				src += 2;
				dest += 1;
			}

			input_line_start -= width * 2;
			dest_line_start += width * bpp;
		}
	}
	else if( IsEqualGUID(mediaType, MEDIASUBTYPE_RGB24) ) {
		BYTE *input_line_start = input_image + (height-1)*stride;
		assert( bpp == 3 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			int i;
			for (i = 0; i < width; i++) {
				dest[0] = src[2];
				dest[1] = src[1];
				dest[2] = src[0];
			
				src += 3;
				dest += bpp;
			}

			input_line_start -= stride;
			dest_line_start += width * bpp;
		}
	}

	else if( IsEqualGUID(mediaType, MEDIASUBTYPE_RGB32) ) {
		assert( 0 );
		// This code is not yet working correctly.  For now I've removed it from the supported list
		BYTE *input_line_start = input_image + (height-1)*stride;

		assert( bpp == 3 );

		for (int j = 0; j < height; j++) {
			BYTE *src = input_line_start;
			char *dest = dest_line_start;
			int i;
			for (i = 0; i < width; i++) {
				dest[0] = src[2];
				dest[1] = src[1];
				dest[2] = src[0];
			
				src += 4;
				dest += bpp;
			}

			input_line_start -= stride;
			dest_line_start += width * bpp;
		}
	}

	EnterCriticalSection( &zVidcapDirectXCriticalSection );
	locks[which] = 0;
	frameNums[which] = nextFrameNum;
	LeaveCriticalSection( &zVidcapDirectXCriticalSection );
		
	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::JoinFilterGraph(IFilterGraph *graph, LPCWSTR desired_name) {
	filter_graph = graph;

	if (name) {
		delete[] name;
		name = NULL;
	}

	if (desired_name) {
		DWORD len = lstrlenW(desired_name)+1;
		name = new WCHAR[len];
		assert(name);

		if (name) memcpy(name, desired_name, len * sizeof(WCHAR));
	}

	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::QueryVendorInfo(LPWSTR *vendor_info_return) {
	UNREFERENCED_PARAMETER(vendor_info_return);
	return E_NOTIMPL;
}

STDMETHODIMP ZVidcapFilter::Register() {
	return S_FALSE;
}

STDMETHODIMP ZVidcapFilter::Unregister() {
	return S_FALSE;
}

STDMETHODIMP ZVidcapFilter::SetSyncSource(IReferenceClock *clock_return) {
	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::GetSyncSource(IReferenceClock **clock_return) {
	*clock_return = NULL;
	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::Stop() {
	// For some reason we might want to tell the receiver to stop, or something.
	HRESULT hr = NOERROR;
	return hr;
}

STDMETHODIMP ZVidcapFilter::Pause() {
	return S_OK;
}

STDMETHODIMP ZVidcapFilter::Run(REFERENCE_TIME start_timex) {
	return S_OK;
}

STDMETHODIMP ZVidcapFilter::GetClassID(CLSID *class_id_return) {
	*class_id_return = CLSID_Vidcap_Filter;
	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::QueryFilterInfo(FILTER_INFO * pInfo) {
	if (name) {
		lstrcpynW(pInfo->achName, name, sizeof(pInfo->achName)/sizeof(WCHAR));
	}
	else {
		pInfo->achName[0] = L'\0';
	}

	pInfo->pGraph = filter_graph;
	if (filter_graph) filter_graph->AddRef();

	return NOERROR;
}

STDMETHODIMP ZVidcapFilter::FindPin(LPCWSTR Id, IPin **pin_return) {
	ZVidcapPin *pPin = input_pin;

	if( lstrcmpW(name, Id) == 0 ) {	// The pin's name is actually my name.
		*pin_return = pPin;
		pPin->AddRef();
		return S_OK;
	}

	*pin_return = NULL;
	return VFW_E_NOT_FOUND;
}

STDMETHODIMP ZVidcapFilter::GetState(DWORD milliseconds, FILTER_STATE *state_return) {
	*state_return = State_Running;
	return S_OK;
}

STDMETHODIMP ZVidcapFilter::EnumPins(IEnumPins **ppEnum) {
	*ppEnum = new ZVidcapPinEnumerator(this, NULL);
	if (*ppEnum == NULL) return E_OUTOFMEMORY;
	return NOERROR;
}

ZVidcapPinEnumerator::ZVidcapPinEnumerator(ZVidcapFilter *_receiver, ZVidcapPinEnumerator *to_copy) {
	reference_count = 1;
	index = 0;

	receiver = _receiver;
	receiver->AddRef();

	if (to_copy) index = to_copy->index;
}

ZVidcapPinEnumerator::~ZVidcapPinEnumerator() {
	receiver->Release();
}

STDMETHODIMP ZVidcapPinEnumerator::QueryInterface(REFIID riid, void **result) {
	if (riid == IID_IEnumPins || riid == IID_IUnknown) {
		return zVidcapGetInterface((IEnumPins *)this, result);
	}
	else {
		*result = NULL;
		return E_NOINTERFACE;
	}
}

STDMETHODIMP_(ULONG) ZVidcapPinEnumerator::AddRef() {
	return InterlockedIncrement(&reference_count);
}

STDMETHODIMP_(ULONG) ZVidcapPinEnumerator::Release() {
	ULONG cRef = InterlockedDecrement(&reference_count);
	if (cRef == 0) delete this;  // I thought this was not valid C++,
	return cRef;				 // but it is how COM works.
}

STDMETHODIMP ZVidcapPinEnumerator::Clone(IEnumPins **result) {
	*result = new ZVidcapPinEnumerator(receiver, this);
	if (*result == NULL) return E_OUTOFMEMORY;

	return NOERROR;
}

STDMETHODIMP ZVidcapPinEnumerator::Next(ULONG num_pins, IPin **pins_return, ULONG *num_fetched_return) {
	if (num_fetched_return != NULL) *num_fetched_return = 0;

	const int NUM_PINS = 1;
	int pins_left = NUM_PINS - index;
	if (pins_left <= 0) return S_FALSE;

	assert(index == 0);
	pins_return[0] = receiver->input_pin;
	receiver->input_pin->AddRef();
	index++;
	*num_fetched_return = 1;

	return NOERROR;
}

STDMETHODIMP ZVidcapPinEnumerator::Skip(ULONG num_to_skip) {
	// Since we know we only have one pin, this is
	// real simple.

	const int NUM_PINS = 1;
	ULONG pins_left = NUM_PINS - index;
	if(num_to_skip > pins_left) return S_FALSE;

	index += num_to_skip;
	return S_OK;
}

STDMETHODIMP ZVidcapPinEnumerator::Reset() {
	index = 0;
	return S_OK;
}

STDMETHODIMP ZVidcapPinEnumerator::Refresh() {
	return Reset();
}


// VidCap Internal Interfaces
//------------------------------------------------------------------------------------------

int zVidcapDirectXTraverseAndBindDevices( char *bindTo, void **boundFilter ) {
	// CLEAR the device list
	for( int i=0; i<zVidcapDirectXNumDevices; i++ ) {
		if( zVidcapDirectXDevices[i] ) {
			free( zVidcapDirectXDevices[i] );
			zVidcapDirectXDevices[i] = 0;
		}
	}
	zVidcapDirectXNumDevices = 0;

	// SETUP Microsoft COM
	CoInitialize(NULL);

	// INIT local
	HRESULT hr = 0;
	if( boundFilter ) *boundFilter = NULL;
	int success = 0;

	// ASSUME success if we are merely enumerating and not binding
	if( !bindTo ) success = 1;

	// INSTANCIATE a device enumerator
	ICreateDevEnum *pDeviceEnumerator;
	hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDeviceEnumerator );
	if( hr != S_OK ) goto error;

	// INSTANCIATE a class enumerator on vidcap inputs
	IEnumMoniker *pEnumMoniker;
	hr = pDeviceEnumerator->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0 );
	if( hr != S_OK ) goto error;

	{ // ENUMARATE the devices and potentially bind to them
		pEnumMoniker->Reset();

		ULONG cFetched;
		IMoniker *pMoniker = NULL;
		while( hr = pEnumMoniker->Next(1, &pMoniker, &cFetched), hr==S_OK ) {
			IPropertyBag *pBag;
			char name[256] = {0,};
			char dxName[256] = {0,};

			hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void **)&pBag );
			if( hr == S_OK ) {
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read( L"FriendlyName", &var, NULL );
				if( hr == S_OK ) {
					WideCharToMultiByte( CP_ACP, 0, var.bstrVal, -1, name, 80, NULL, NULL );
					strcpy( dxName, "DX " );
					strcat( dxName, name );
					zVidcapDirectXDevices[zVidcapDirectXNumDevices++] = strdup( dxName );
					zVidcapTrace( "%s %d. Device: %s\n", __FILE__, __LINE__, name );
					assert( zVidcapDirectXNumDevices < zVidcapDevicesMax );
					SysFreeString( var.bstrVal );
				}
				hr = pBag->Release();
				if( bindTo && !*boundFilter ) {
					// If we are being asked to bind to a filter and we haven't already...
					if( !stricmp( bindTo, dxName ) ) {
						zVidcapTrace( "%s %d. Binding Device: %s\n", __FILE__, __LINE__, name );
						hr = pMoniker->BindToObject( 0, 0, IID_IBaseFilter, boundFilter );
						zVidcapTrace( "%s %d. hr = %d\n", __FILE__, __LINE__, hr );
						success = hr == S_OK ? 1 : 0;
					}
				}
			}
			pMoniker->Release();
		}

	}

  error:
	if( pDeviceEnumerator )
		pDeviceEnumerator->Release();
	if( pEnumMoniker )
		pEnumMoniker->Release();
	return success;
}

ZVidcapDirectXConnection *zVidcapDirectXGetConnectionFromName( char *deviceName ) {
	if( !deviceName ) {
		deviceName = zVidcapLastDevice;
	}
	if( !deviceName ) {
		return NULL;
	}

	for( int i=0; i<zVidcapDirectXNumDevices; i++ ) {
		if( !stricmp(zVidcapDirectXDevices[i],deviceName) ) {
			return &vidcapConns[i];
		}
	}
	return NULL;
}

void zVidcapDirectXRecursiveDestoryGraph( IFilterGraph *filterGraph, IBaseFilter *pf ) {
	IPin *pP, *pTo;
	ULONG u;
	IEnumPins *pins = NULL;
	PIN_INFO pininfo;
	HRESULT hr = pf->EnumPins(&pins);
	pins->Reset();
	while (hr == NOERROR) {
		hr = pins->Next(1, &pP, &u);
		if (hr == S_OK && pP) {
			pP->ConnectedTo(&pTo);
			if (pTo) {
				hr = pTo->QueryPinInfo(&pininfo);
				if (hr == NOERROR) {
					if (pininfo.dir == PINDIR_INPUT) {
						zVidcapDirectXRecursiveDestoryGraph(filterGraph,pininfo.pFilter);
						filterGraph->Disconnect(pTo);
						filterGraph->Disconnect(pP);
						filterGraph->RemoveFilter(pininfo.pFilter);
					}
					pininfo.pFilter->Release();
				}
				pTo->Release();
			}
			pP->Release();
		}
	}
	if (pins) {
		pins->Release();
	}
}

// ZVidcapPluginInterface
//--------------------------------------------------------------

void zVidcapDirectXGetDevices( char **names, int *count ) {
	zVidcapDirectXTraverseAndBindDevices( NULL, NULL );
	if( count ) {
		*count = zVidcapDirectXNumDevices;
	}
	for( int i=0; i<zVidcapDirectXNumDevices; i++ ) {
		names[i] = strdup( zVidcapDirectXDevices[i] );
	}
}

void zVidcapDirectXQueryModes( char *deviceName, char **modes, int *count ) {
	int ret = zVidcapStartDevice( deviceName, 0 );
	if( count ) {
		*count = zVidcapDirectXNumModes;
	}
	for( int i=0; i<zVidcapDirectXNumModes; i++ ) {
		modes[i] = strdup( zVidcapDirectXModes[i] );
	}
}

void zVidcapDirectXShutdownDevice( char *deviceName ) {
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		if( conn->mediaControlIFACE ) {
			conn->mediaControlIFACE->Stop();
			conn->mediaControlIFACE->Release();
			conn->mediaControlIFACE = NULL;
		}

		if( conn->filter ) {
			conn->filter->Release();
			conn->filter = NULL;
		}

		if( conn->cameraFilter ) {
			zVidcapDirectXRecursiveDestoryGraph( conn->graphBuilderIFACE, conn->cameraFilter );
			conn->cameraFilter->Release();
			conn->cameraFilter = NULL;
		}

		if( conn->filterGraphIFACE ) {
			//conn->filterGraphIFACE->Release();
			// If I release this then the release of conn->graphBuilderIFACE
			// crashes or vice versa.  There is definately something wrong
			// here having to do with reference counting or something
			conn->filterGraphIFACE = NULL;
		}

		if( conn->seekIFACE ) {
			conn->seekIFACE->Release();
			conn->seekIFACE = NULL;
		}

		if( conn->captureGraphBuilderIFACE ) {
			conn->captureGraphBuilderIFACE->Release();
			conn->captureGraphBuilderIFACE= NULL;
		}

		if( conn->graphBuilderIFACE ) {
			conn->graphBuilderIFACE->Release();
			conn->graphBuilderIFACE = NULL;
		}

		if( zVidcapLastDevice && !strcmp(zVidcapLastDevice,deviceName) ) {
			free( zVidcapLastDevice );
			zVidcapLastDevice = 0;
			free( zVidcapLastDeviceMode );
			zVidcapLastDeviceMode = 0;
		}
	}
}

int zVidcapDirectXStartDevice( char *deviceName, char *mode ) {
	// If mode is null then it simply enumerates the modes and returns
	// otherwise it opens that specific mode string

	if( !zVidcapDirectXCriticalSectionInitialized ) {
		InitializeCriticalSection( &zVidcapDirectXCriticalSection );
		zVidcapDirectXCriticalSectionInitialized = 1;
	}

	// CLEAR out the modes arary if we are doing a mode enumerate
	if( !mode ) {
		for( int i=0; i<zVidcapModesMax; i++ ) {
			if( zVidcapDirectXModes[i] ) {
				free( zVidcapDirectXModes[i] );
				zVidcapDirectXModes[i] = 0;
			}
		}
		zVidcapDirectXNumModes = 0;
	}

	HRESULT hr;

	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( !conn ) return 0;

	// CREATE the filter graph
	hr = CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&conn->graphBuilderIFACE );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}
	assert( conn->graphBuilderIFACE );

	// CREATE the graph builder
	hr = CoCreateInstance( CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void **)&conn->captureGraphBuilderIFACE );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}
	assert( conn->captureGraphBuilderIFACE );

	// FIND the media control	 
	hr = conn->graphBuilderIFACE->QueryInterface( IID_IMediaControl,(LPVOID *) &conn->mediaControlIFACE );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}
	assert( conn->mediaControlIFACE );

	// TELL the graph builder which filter graph it is working on
	hr = conn->captureGraphBuilderIFACE->SetFiltergraph( conn->graphBuilderIFACE );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}

	// BIND to the requested camera
	zVidcapDirectXTraverseAndBindDevices( deviceName, (void**)&conn->cameraFilter );

	// ADD capture filter to our graph.
	hr = conn->graphBuilderIFACE->AddFilter( conn->cameraFilter, L"Video Capture" );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}

	// CREATE an instance of our capture filter which just grabs the data into memory
	conn->filter = new ZVidcapFilter();

	// ATTACH our capture filter to the filterGraph
	hr = conn->graphBuilderIFACE->AddFilter( conn->filter, L"MEMORYRENDERER");
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}

	conn->filter->setModeString( mode );

	if( zVidcapSkipFirstEnumeration )
	{
		static bool skipFirst = true;
		if( skipFirst )
		{
			skipFirst = false;
			zVidcapDirectXShutdownDevice( deviceName );
			return 1;
		}
	}

	hr = conn->captureGraphBuilderIFACE->RenderStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, conn->cameraFilter, NULL, conn->filter );
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		if( !mode ) {
			// We were only enumerating modes so this is success
			return 1;
		}
		return 0;
	}
	assert( mode );

	// START capturing video
	hr = conn->mediaControlIFACE->Run();
	if( !SUCCEEDED(hr) ) {
		zVidcapDirectXShutdownDevice( deviceName );
		return 0;
	}

	zVidcapLastDevice = strdup( deviceName );
	zVidcapLastDeviceMode = strdup( mode );

	return 1;
}

void zVidcapDirectXGetBitmapDesc( char *deviceName, int &w, int &h, int &d ) {
	w = 0;
	h = 0;
	d = 0;
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		if( conn->filter ) {
			w = conn->filter->width;
			h = conn->filter->height;
			d = conn->filter->depth;
		}
	}
}

int zVidcapDirectXShowFilterPropertyPageModalDialog( char *deviceName ) {
	HRESULT hr = 0;
	int w0, h0, d0, w1, h1, d1;
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		// Don't do anything if we don't have a media control
		if( !conn->mediaControlIFACE ) return 0;

		// STORE old, see if it changed.
		zVidcapDirectXGetBitmapDesc( deviceName, w0, h0, d0 );

		// FETCH the camera filter's IID_ISpecifyPropertyPages interface
		ISpecifyPropertyPages *pispp = NULL;
		hr = conn->captureGraphBuilderIFACE->FindInterface( NULL, NULL, conn->cameraFilter, IID_ISpecifyPropertyPages, (void **)&pispp );
		if( !SUCCEEDED(hr) ) {
			return 0;
		}

		// STOP the media
		hr = conn->mediaControlIFACE->Stop();

		// FETCH the property page
		CAUUID caGUID;
		hr = pispp->GetPages(&caGUID);
		if( !SUCCEEDED(hr) ) {
			pispp->Release();
			hr = conn->mediaControlIFACE->Run();
			return 0;
		}
		pispp->Release();

		// CREATE the modal dialog box
		hr = OleCreatePropertyFrame(
			NULL, 0, 0, L"Filter",	1, (IUnknown **)&conn->cameraFilter,
			caGUID.cElems, caGUID.pElems, 0, 0, NULL
		);

		// RESUME the media
		hr = conn->mediaControlIFACE->Run();

		// CHECK if the format changed
		zVidcapDirectXGetBitmapDesc( deviceName, w1, h1, d1 );

		if( w0!=w1 || h0!=h1 || d0!=d1 ) {
			return 1;
		}
	}
	return 0;
}

int zVidcapDirectXShowPinPropertyPageModalDialog( char *deviceName ) {
	HRESULT hr = 0;
	int w0, h0, d0, w1, h1, d1;
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		// Don't do anything if we don't have a media control
		if( !conn->mediaControlIFACE ) return 0;

		// STORE old, see if it changed.
		zVidcapDirectXGetBitmapDesc( deviceName, w0, h0, d0 );

		// FETCH the output pin on the camera filter
		IPin *cameraPin = 0;
		hr = conn->captureGraphBuilderIFACE->FindPin( conn->cameraFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, NULL, FALSE, 0, &cameraPin );
		if( !SUCCEEDED(hr) ) {
			return 0;
		}

		// FETCH the pin's IID_ISpecifyPropertyPages interface
		ISpecifyPropertyPages *pispp = NULL;
		hr = cameraPin->QueryInterface( IID_ISpecifyPropertyPages, (void **)&pispp );
		if( !SUCCEEDED(hr) ) {
			cameraPin->Release();
			return 0;
		}

		// STOP the media
		hr = conn->mediaControlIFACE->Stop();

		// FETCH the property page
		CAUUID caGUID;
		hr = pispp->GetPages(&caGUID);
		if( !SUCCEEDED(hr) ) {
			cameraPin->Release();
			pispp->Release();
			hr = conn->mediaControlIFACE->Run();
			return 0;
		}
		pispp->Release();

		// CREATE the modal dialog
		hr = OleCreatePropertyFrame(
			NULL, 0, 0, L"Filter",	1, (IUnknown **)&cameraPin,
			caGUID.cElems, caGUID.pElems, 0, 0, NULL
		);
		cameraPin->Release();

		// RESUME the media
		hr = conn->mediaControlIFACE->Run();

		// CHECK if the format changed
		zVidcapDirectXGetBitmapDesc( deviceName, w1, h1, d1 );
		if( w0!=w1 || h0!=h1 || d0!=d1 ) {
			return 1;
		}
	}

	return 0;
}

char *zVidcapDirectXLockNewest( char *deviceName, int *frameNumber ) {
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		EnterCriticalSection( &zVidcapDirectXCriticalSection );

		// WHICH one is newer and unlocked?
		int which = 0;
		if( !conn->filter->locks[0] && !conn->filter->locks[1] ) {
			// Both are unlocked, pick the newer
			which = conn->filter->frameNums[0] > conn->filter->frameNums[1] ? 0 : 1;
		}
		else if( conn->filter->locks[0] && conn->filter->locks[1] ) {
			// Both locked, do nothing
			LeaveCriticalSection( &zVidcapDirectXCriticalSection );
			conn->filter->readLock = -1;
			return NULL;
		}
		else {
			// One is unlocked, other locked.  Pick the unlocked one.
			which = !conn->filter->locks[0] ? 0 : 1;
		}

		if( frameNumber ) {
			if( *frameNumber >= conn->filter->frameNums[which] ) {
				// This isn't any newer than the one we already have
				LeaveCriticalSection( &zVidcapDirectXCriticalSection );
				conn->filter->readLock = -1;
				return NULL;
			}
			else {
				*frameNumber = conn->filter->frameNums[which];
			}
		}

		conn->filter->locks[which] = 1;
		conn->filter->readLock = which;
		LeaveCriticalSection( &zVidcapDirectXCriticalSection );

		return conn->filter->incoming_image_buffers[which];
	}
	return NULL;
}

void zVidcapDirectXUnlock( char *deviceName ) {
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		EnterCriticalSection( &zVidcapDirectXCriticalSection );
		if( conn->filter->readLock != -1 ) {
			conn->filter->locks[ conn->filter->readLock ] = 0;
		}
		LeaveCriticalSection( &zVidcapDirectXCriticalSection );
	}
}

int zVidcapDirectXGetAvgFrameTimeInMils( char *deviceName ) {
	ZVidcapDirectXConnection *conn = zVidcapDirectXGetConnectionFromName( deviceName );
	if( conn ) {
		return conn->filter->getAvgFrameTimings();
	}
	return 0;
}

void zVidcapDirectXSendDriverMessage( char *deviceName, char *msg ) {
}

static ZVidcapPluginRegister zvidcapDirectXRegister(
	"DirectX",
	zVidcapDirectXGetDevices,
	zVidcapDirectXQueryModes,
	zVidcapDirectXStartDevice,
	zVidcapDirectXShutdownDevice,
	zVidcapDirectXLockNewest,
	zVidcapDirectXUnlock,
	zVidcapDirectXGetAvgFrameTimeInMils,
	zVidcapDirectXGetBitmapDesc,
	zVidcapDirectXShowFilterPropertyPageModalDialog,
	zVidcapDirectXShowPinPropertyPageModalDialog,
	zVidcapDirectXSendDriverMessage
);

} // End namespace
