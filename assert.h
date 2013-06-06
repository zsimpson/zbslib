// This is a duplicate of assert.h put into the local directories
// to overload the normal behavior.  Assert normally is wrapped
// by a DEBUG macro so that it is in active in release.  I don't
// like this behavior so this over rides it.

//#pragma message("Using local assert")

#ifndef ZBSLIB_ASSERT_H
#define ZBSLIB_ASSERT_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
	
void __assert_fail( const char *ass, const char *file, unsigned int line, const char *func);
#define assert(exp) (void)( (exp) || (__assert_fail(#exp, __FILE__, __LINE__,0), 0) )

#else

void __cdecl _assert(const void *, const void *, unsigned);
#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )

#endif

#ifdef  __cplusplus
}
#endif




//////////////


//extern void __assert_fail (__const char *__assertion, __const char *__file,
//		   unsigned int __line, __const char *__function)

//extern void __assert (const char *__assertion, const char *__file, int __line)
//     __THROW __attribute__ ((__noreturn__));


#endif


