#define EV_STANDALONE 1

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif  // __GNUC__

#ifdef MSVC
#pragma warning( push )
//#pragma warning( disable : 4723 )
#endif  // MSVC

#ifdef _WIN32
#include "evwrap.h"
#endif  // __linux__
#include "ev.c"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif  // __GNUC__

#ifdef MSVC
#pragma warning( pop )
#endif  // MSVC
