#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "Config.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#if PLATFORM == PF_LINUX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif  // PLATFORM

#if PLATFORM == PF_WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#endif  // PLATFORM

#ifndef MAX_TAG_SIZE
#define MAX_TAG_SIZE sizeof( uint64_t )
#endif

#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 1024 - MAX_TAG_SIZE
#endif

#ifndef MAX_ACTIVE_TIMERS
#define MAX_ACTIVE_TIMERS 10
#endif

#ifndef BACKLOG
#define BACKLOG 10
#endif

#ifndef ENUM_VAL
#define ENUM_VAL( x ) 1 << x
#endif  // !ENUM_VAL

#if PLATFORM == PF_LINUX
#define ServerSocket int32_t
#elif PLATFORM == PF_WIN32
#define ServerSocket SOCKET
#endif  // PLATFORM

struct ServerLoop;
struct ServerTimer;

typedef void ( *loop_cb )( struct ServerLoop* );
typedef void ( *timer_cb )( struct ServerLoop*, struct ServerTimer* );

struct ServerAddressInfo
{
    char ip_raw[INET6_ADDRSTRLEN];
    char port_raw[INET6_ADDRSTRLEN];
    struct addrinfo hints;
    struct addrinfo* options;
    struct addrinfo* selected;
    int32_t status;
    int32_t port_num;
    ServerSocket socket_fd;
};

struct ServerTimer
{
    uint64_t tick_rate;
    bool repeat;
};

struct ServerLoop
{
    uint64_t start_time;
    uint64_t active_time;
    uint32_t timers_count;

    loop_cb callback;

    struct ServerAddressInfo* listener;

    struct ServerTimer timers[MAX_ACTIVE_TIMERS];
    uint64_t timer_update[MAX_ACTIVE_TIMERS];
    timer_cb timer_cbs[MAX_ACTIVE_TIMERS];

    bool active;
};

struct ServerRecvMsg
{
    char msg[MAX_MSG_SIZE];
    int32_t bytes_read;
    struct sockaddr_storage sender;
    socklen_t addr_len;
};

void server_init_win32();

void server_cleanup_win32();

void server_close_socket( ServerSocket* c_restrict socket );

void server_close_clients( struct ServerAddressInfo* c_restrict clients,
                           const uint32_t count );

void server_create_socket( struct ServerAddressInfo* c_restrict address,
                           const char* const c_restrict ip,
                           const char* const c_restrict port,
                           const bool create_server );

bool server_create_socket2( struct ServerAddressInfo* c_restrict address,
                            struct sockaddr_storage* client,
                            const uint32_t port );

void server_server_send_msg( const struct ServerAddressInfo* c_restrict
                                 recipient,
                             const char* const c_restrict msg );

void server_server_recieve_msg( const struct ServerAddressInfo* c_restrict
                                    listener,
                                struct ServerRecvMsg* msg_data );

struct ServerLoop server_server_new_loop( loop_cb loop_cb,
                                          struct ServerAddressInfo* listener );

void server_loop_add_timer( struct ServerLoop* c_restrict loop,
                            timer_cb timer_cb,
                            double seconds,
                            bool repeat );

int64_t server_loop_time_nano( struct ServerLoop* c_restrict loop );

double server_loop_time_seconds( struct ServerLoop* c_restrict loop );

void server_loop_break( struct ServerLoop* loop );

void server_loop_run( struct ServerLoop* loop );
