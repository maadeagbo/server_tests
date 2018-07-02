#define EV_STANDALONE 1

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif  // __GNUC__

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4706 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4133 )
#pragma warning( disable : 4127 )
#endif  // _WIN32

#include "ddEVEmbed.h"

#include "ev.c"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif  // __GNUC__

#ifdef _WIN32
#pragma warning( pop )
#endif  // _WIN32
