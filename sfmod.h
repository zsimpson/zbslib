// #include for Zlab code that references Stopflow timing stuff only for that app.

#ifdef STOPFLOW

// this used to include the stopflow sf_time.h to enable the profiling, but now this is running into troubles because of the messed up
// Windows gl headers. So this is all disabled now. If I want to make the profiling work again, this will need to be reworked.

// include directive obfuscated so that broken zlabbuild.pl script does not think this needs to be referenced
// INCLUDE "..\\plug_kintek\\_stopflow\\src\\sf_time.h"

#define SFFAST      // enable for removal of gl push/pop attributes which seem to be *extremely* expensive

#endif

// effectively remove profiling statements
#define SFTIME_RESET()    
#define SFTIME_FINISH()   
#define SFTIME_START(a,b) 
#define SFTIME_END(a)
