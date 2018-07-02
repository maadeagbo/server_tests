#pragma once

#define EV_STANDALONE 1

#ifdef _WIN32
#define EV_SELECT_IS_WINSOCKET 1 /* configure libev for windows select */
#endif                           // __linux__

#include <sys/types.h>
#include <errno.h>
#ifdef __linux__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif  //__linux__

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif  // _WIN32

#include "ev.h"

typedef void ddev_func_io( struct ev_loop*, ev_io*, int );
typedef void ddev_func_timer( struct ev_loop*, ev_timer*, int );
typedef void ddev_func_signal( struct ev_loop*, ev_signal*, int );
