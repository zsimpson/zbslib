// @ZBS {
//		*MODULE_OWNER_NAME zwildcard
// }

#ifndef ZBITTEXCACHE_H
#define ZBITTEXCACHE_H

int zbitTexCacheComputeChunkSize( int w, int h );
int *zbitTexCacheLoad( struct ZBits *bits, int invert=0 );
int *zbitTexCacheLoadFloatScale( struct ZBits *bits, int invert=0, float scale=1.f );
int *zbitTexCacheLoadDoubleScale( struct ZBits *bits, int invert=0, double scale=1.0 );
void zbitTexCacheFree( int *texCache );
void zbitTexCacheRender( int *texCache );
void zbitTexCacheGetDims( int *texCache, int &w, int &h );

#endif



