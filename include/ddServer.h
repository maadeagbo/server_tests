#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ddConfig.h"
#include "ddEVEmbed.h"

#ifndef BACKLOG
#define BACKLOG 10
#endif

#ifndef MAX_TAG_LENGTH
#define MAX_TAG_LENGTH 64
#endif

#ifndef MAX_MSG_LENGTH
#define MAX_MSG_LENGTH 1024 - MAX_TAG_LENGTH
#endif

#ifndef ENUM_VAL
#define ENUM_VAL( x ) 1 << x
#endif  // !ENUM_VAL

#ifdef __linux__
#define ddSocket int32_t
#elif _WIN32
#define ddSocket SOCKET
#endif // __linux__


enum
{
    DDLOG_STATUS = ENUM_VAL( 0 ),
    DDLOG_ERROR = ENUM_VAL( 1 ),
    DDLOG_WARNING = ENUM_VAL( 2 ),
    DDLOG_NOTAG = ENUM_VAL( 3 ),
};

#undef ENUM_VAL

struct ddAddressInfo
{
    struct addrinfo hints;
    struct addrinfo* options;
    struct addrinfo* selected;
    int32_t status;
	int32_t socket_fd;
};

struct ddIODataEV
{
	int32_t socketfd;
    int32_t io_flags;
    ev_io* io_ptr;
    ddev_func_io* cb_func;
    struct ev_loop* loop_ptr;
};

struct ddTimerDataEV
{
    double interval;
    ev_timer* timer_ptr;
    ddev_func_timer* cb_func;
    struct ev_loop* loop_ptr;
    bool repeat_flag;
};

struct ddSignalDataEV
{
    int32_t signal;
    ev_signal* signal_ptr;
    ddev_func_signal* cb_func;
    struct ev_loop* loop_ptr;
};

struct ddMsgData
{
	int32_t socketfd;
    uint16_t msg_size;
    uint16_t tag_size;
    char msg[MAX_MSG_LENGTH];
    char tag[MAX_TAG_LENGTH];
};

void dd_server_init_win32();

void dd_server_cleanup_win32();

void dd_close_socket( int32_t* c_restrict socket );

void dd_create_socket( struct ddAddressInfo* c_restrict address,
                       const char* c_restrict ip,
                       const char* c_restrict port,
                       const bool create_server,
                       const bool use_tcp );

void dd_server_write_out( const uint32_t log_type,
                          const char* c_restrict fmt_str,
                          ... );

void dd_ev_io_watcher( struct ddIODataEV* c_restrict io_data );

void dd_ev_timer_watcher( struct ddTimerDataEV* c_restrict timer_data );

void dd_ev_signal_watcher( struct ddSignalDataEV* c_restrict signal_data );

void dd_server_send_msg( struct ddMsgData* c_restrict msg );
