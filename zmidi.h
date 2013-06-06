
#ifndef ZMIDI_H
#define ZMIDI_H

void zmidiOpenOutput( int port );
void zmidiCloseOutput();
void zmidiProgramChange( int channel, int generalMidiInstrument );
void zmidiStartNote( int note, int channel, int volume );
void zmidiStopNote( int note, int channel );
void zmidiAllNotesOff();


const int zmidiMidAb = 68;
const int zmidiMidA  = 69;
const int zmidiMidBb = 70;
const int zmidiMidB  = 71;
const int zmidiMidC  = 72;
const int zmidiMidDb = 73;
const int zmidiMidD  = 74;
const int zmidiMidEb = 75;
const int zmidiMidE  = 76;
const int zmidiMidF  = 77;
const int zmidiMidGb = 78;
const int zmidiMidG  = 79;

#endif