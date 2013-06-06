// For use in the master files:

// @ZBS {
//		*MODULE_NAME 
//		+DESCRIPTION {
//		}
//		+EXAMPLE {
//		}
//		*MODULE_DEPENDS
//		*SDK_DEPENDS
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES
//		*OPTIONAL_FILES
//		*VERSION 1.5
//		+HISTORY {
//			1.2 KLD fixed a bug on setting a key that had previously been deleted
//			1.3 ZBS changed memcmp to strncmp to fix a bug that made set read potenitally outside of bounds (causing Bounds Checker to complain)
//			1.4 ZBS changed hash function to code borrowed from Bob Jenkins
//		}
//		+TODO {
//			Finish Unix port.  Need to implement sleep with select
//		}
//		*SELF_TEST yes console
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
// ZBSLIB includes:

//For use in the non-master files:

// @ZBS {
//		*MODULE_OWNER_NAME zhashtable
// }


