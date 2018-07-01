#pragma once

#define EV_STANDALONE 1

#ifdef _WIN32
#define EV_SELECT_IS_WINSOCKET 1 /* configure libev for windows select */
#endif                           // __linux__

#include "ev.h"

typedef void ddev_func_io( struct ev_loop*, ev_io*, int );
typedef void ddev_func_timer( struct ev_loop*, ev_timer*, int );
typedef void ddev_func_signal( struct ev_loop*, ev_signal*, int );
