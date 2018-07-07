#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ddConfig.h"

#include <sys/types.h>
#include <errno.h>
#if DD_PLATFORM == DD_LINUX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif  // DD_PLATFORM

#if DD_PLATFORM == DD_WIN32

#pragma warning( push )
#pragma warning( disable : 4201 )  // unnamed union
#pragma warning( disable : 4204 )  // struct initializer

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
    uint64_t start_time;
    uint64_t active_time;
    uint32_t timers_count;

    dd_loop_cb callback;

    struct ddAddressInfo* listener;

    struct ddServerTimer timers[MAX_ACTIVE_TIMERS];
    uint64_t timer_update[MAX_ACTIVE_TIMERS];
    dd_timer_cb timer_cbs[MAX_ACTIVE_TIMERS];

    bool active;
};

enum
{
    DDMSG_FLOAT1 = ENUM_VAL( 0 ),
    DDMSG_FLOAT2 = ENUM_VAL( 1 ),
    DDMSG_FLOAT3 = ENUM_VAL( 2 ),
    DDMSG_FLOAT4 = ENUM_VAL( 3 ),

    DDMSG_INT1 = ENUM_VAL( 4 ),
    DDMSG_INT2 = ENUM_VAL( 5 ),
    DDMSG_INT3 = ENUM_VAL( 6 ),
    DDMSG_INT4 = ENUM_VAL( 7 ),

    DDMSG_STR = ENUM_VAL( 8 ),
    DDMSG_BOOL = ENUM_VAL( 9 ),
};

struct ddMsgVal
{
    union {
        float f[4];
        int32_t i[4];
        const char* c;
        bool b;
    };
};

struct ddRecvMsg
{
    char msg[MAX_MSG_LENGTH];
    int32_t bytes_read;
    struct sockaddr_storage sender;
    socklen_t addr_len;
};

void dd_server_init_win32();

void dd_server_cleanup_win32();

void dd_close_socket( ddSocket* c_restrict socket );

void dd_create_socket( struct ddAddressInfo* c_restrict address,
                       const char* c_restrict ip,
                       const char* c_restrict port,
                       const bool create_server );

void dd_server_send_msg( const struct ddAddressInfo* c_restrict recipient,
                         const uint32_t msg_type,
                         const struct ddMsgVal* c_restrict msg );

void dd_server_recieve_msg( const struct ddAddressInfo* c_restrict listener,
                            struct ddRecvMsg* c_restrict msg_data );

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
