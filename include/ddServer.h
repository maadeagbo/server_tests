#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ddConfig.h"

#include <sys/types.h>
#include <errno.h>
#if DD_PLATFORM == DD_LINUX
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif  // DD_PLATFORM

#if DD_PLATFORM == DD_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif  // DD_PLATFORM

#ifndef BACKLOG
#define BACKLOG 10
#endif

#ifndef MAX_TAG_LENGTH
#define MAX_TAG_LENGTH 64
#endif

#ifndef MAX_MSG_LENGTH
#define MAX_MSG_LENGTH 1024 - MAX_TAG_LENGTH
#endif

#ifndef MAX_ACTIVE_TIMERS
#define MAX_ACTIVE_TIMERS 10
#endif

#ifndef ENUM_VAL
#define ENUM_VAL( x ) 1 << x
#endif  // !ENUM_VAL

#if DD_PLATFORM == DD_LINUX
#define ddSocket int32_t
#elif DD_PLATFORM == DD_WIN32
#define ddSocket SOCKET
#endif  // DD_PLATFORM

enum
{
    DDLOG_STATUS = ENUM_VAL( 0 ),
    DDLOG_ERROR = ENUM_VAL( 1 ),
    DDLOG_WARN = ENUM_VAL( 2 ),
    DDLOG_NOTAG = ENUM_VAL( 3 ),
};

struct ddLoop;
struct ddServerTimer;

typedef void ( *dd_loop_cb )( struct ddLoop* );
typedef void ( *dd_timer_cb )( struct ddLoop*, struct ddServerTimer* );

struct ddAddressInfo
{
    struct addrinfo hints;
    struct addrinfo* options;
    struct addrinfo* selected;
    int32_t status;
    ddSocket socket_fd;
};

struct ddServerTimer
{
    uint64_t tick_rate;
    bool repeat;
};

struct ddLoop
{
    int64_t start_time;
    int64_t active_time;
    int32_t timers_count;

    dd_loop_cb callback;

    struct ddAddressInfo* listener;

    struct ddServerTimer timers[MAX_ACTIVE_TIMERS];
    uint64_t timer_update[MAX_ACTIVE_TIMERS];
    dd_timer_cb timer_cbs[MAX_ACTIVE_TIMERS];

    bool active;
};

struct ddMsgData
{
    char msg[MAX_MSG_LENGTH];
    char tag[MAX_TAG_LENGTH];
    struct ddAddressInfo* recipient;
    uint16_t msg_size;
};

void dd_server_init_win32();

void dd_server_cleanup_win32();

void dd_close_socket( ddSocket* c_restrict socket );

void dd_create_socket( struct ddAddressInfo* c_restrict address,
                       const char* c_restrict ip,
                       const char* c_restrict port,
                       const bool create_server );

void dd_server_write_out( const uint32_t log_type,
                          const char* c_restrict fmt_str,
                          ... );

void dd_server_send_msg( struct ddMsgData* c_restrict msg );

struct ddLoop dd_server_new_loop( dd_loop_cb loop_cb,
                                  struct ddAddressInfo* listener );

void dd_loop_add_timer( struct ddLoop* c_restrict loop,
                        dd_timer_cb timer_cb,
                        double seconds,
                        bool repeat );

int64_t dd_loop_time_nano( struct ddLoop* c_restrict loop );

double dd_loop_time_seconds( struct ddLoop* c_restrict loop );

void dd_loop_break( struct ddLoop* loop );

void dd_loop_run( struct ddLoop* loop );
