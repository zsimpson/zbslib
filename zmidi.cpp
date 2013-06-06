// @ZBS {
//		*MODULE_NAME Midi Fascade
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Interface for connecting to midi
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zmidi.cpp zmidi.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		*SELF_TEST no
//		*PUBLISH no
//		*SDK_DEPENDS maxmidi
// }

#include "windows.h"
#include "zmidi.h"
#include "maxmidi.h"

static HMOUT hMidiOut;
extern void *frontWindowHWND; 

void zmidiOpenOutput( int port ) {
	hMidiOut = OpenMidiOut( (HWND)frontWindowHWND, port, 0, QUEUE_512 );
}

void zmidiCloseOutput() {
	zmidiAllNotesOff();
	CloseMidiOut( hMidiOut );
}

void zmidiProgramChange( int channel, int generalMidiInstrument ) {
	MidiEvent midiEvt;
	midiEvt.status = 0xC0 | channel;
	midiEvt.data1 = generalMidiInstrument-1;
	PutMidiOut( hMidiOut, &midiEvt );
}

void zmidiStartNote( int note, int channel, int volume ) {
	MidiEvent midiEvt;
	midiEvt.time = 0;
	midiEvt.status = NOTEON | channel;
	midiEvt.data1 = note;
	midiEvt.data2 = volume;
	PutMidiOut( hMidiOut, &midiEvt );
}

void zmidiStopNote( int note, int channel ) {
	MidiEvent midiEvt;
	midiEvt.time = 0;
	midiEvt.status = NOTEOFF | channel;
	midiEvt.data1 = note;
	midiEvt.data2 = 0;
	PutMidiOut( hMidiOut, &midiEvt );
}

void zmidiAllNotesOff() {
	for( int channel = 0; channel <= 10; channel++ ) {
		for( int note=0; note<128; note++ ) {
			zmidiStopNote( note, channel );
		}
	}
}

