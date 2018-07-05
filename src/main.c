#include <stdio.h>

#include "ddConfig.h"
#include "ddArgHandler.h"
#include "ddServer.h"
#include "ddTime.h"

static struct ddAddressInfo s_io_watchers[BACKLOG];

static double s_time_tracker = 0.0;
static double s_timeout_limit = 0.0;

void read_cb( struct ddLoop* loop );
void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer );

int main( int argc, char const* argv[] )
{
    UNUSED_VAR( s_io_watchers );

    struct ddArgHandler arg_handler;

    init_arg_handler( &arg_handler,
                      "Creates Server instance using provided IP address." );

    struct ddArgStat ip_arg = {
        .description = "IP address to connect to ( default : \"localhost\" )",
        .full_id = "IP",
        .type_flag = ARG_STR,
        .short_id = 'i',
        .default_val = {.c = "localhost"}};

    struct ddArgStat port_arg = {
        .description = "Port to connect to ( default : 4321 )",
        .full_id = "port",
        .type_flag = ARG_STR,
        .short_id = 'p',
        .default_val = {.c = "4321"}};

    struct ddArgStat server_arg = {
        .description = "Set instance as server ( default : false )",
        .full_id = "server",
        .type_flag = ARG_BOOL,
        .short_id = 's',
        .default_val = {.b = false}};

    struct ddArgStat timeout_arg = {
        .description = "Set time to wait before exit ( default : 60 sec )",
        .full_id = "timeout",
        .type_flag = ARG_FLT,
        .short_id = 't',
        .default_val = {.f = 60.f}};

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );
    register_arg( &arg_handler, &server_arg );
    register_arg( &arg_handler, &timeout_arg );

    poll_args( &arg_handler, argc, argv );

    s_timeout_limit = (double)extract_arg( &arg_handler, 't' )->val.f;

    if( extract_arg( &arg_handler, 'h' )->val.b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

#if DD_PLATFORM == DD_WIN32
    dd_server_init_win32();
#endif  // DD_PLATFORM

    struct ddAddressInfo server_addr = {.options = NULL, .selected = NULL};

    const char* ip_addr_str = extract_arg( &arg_handler, 'i' )->val.c;
    const char* port_str = extract_arg( &arg_handler, 'p' )->val.c;
    const bool listen_flag = extract_arg( &arg_handler, 's' )->val.b;

    dd_create_socket( &server_addr, ip_addr_str, port_str, listen_flag );

#ifdef VERBOSE
    dd_server_write_out(
        DDLOG_WARN, "Timeout set to %.5f secs\n", (float)s_timeout_limit );
#endif  // VERBOSE

    if( server_addr.selected == NULL )
    {
        dd_server_write_out( DDLOG_ERROR, "Socket not created\n" );
        return 1;
    }

    if( listen_flag )
    {
        struct ddLoop looper = dd_server_new_loop( read_cb, &server_addr );

        dd_loop_add_timer( &looper, timer_cb, 0.1, true );

        dd_loop_run( &looper );
    }
    else
    {
        struct ddMsgVal msg = {.c = "Test msg"};

        dd_server_send_msg( &server_addr, DDMSG_STR, &msg );

        freeaddrinfo( server_addr.options );
    }

    dd_close_socket( &server_addr.socket_fd );
#ifdef _WIN32
    void dd_server_cleanup_win32();
#endif  // _WIN32

#ifdef VERBOSE
    dd_server_write_out( DDLOG_STATUS, "Closing server/client program\n" );
#endif  // VERBOSE

    return 0;
}

void read_cb( struct ddLoop* loop )
{
    char buff[MAX_MSG_LENGTH];
    struct sockaddr_storage sender = ( struct sockaddr_storage ){0};
    socklen_t addr_len = sizeof( sender );

    ssize_t bytes_read = recvfrom( loop->listener->socket_fd,
                                   buff,
                                   MAX_MSG_LENGTH - 1,
                                   0,
                                   (struct sockaddr*)&sender,
                                   &addr_len );

    if( bytes_read == -1 )
    {
        dd_server_write_out( DDLOG_ERROR, "recvfrom Error\n" );
        dd_loop_break( loop );
        return;
    }

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    struct sockaddr* sender_soc = (struct sockaddr*)&sender;
    void* sender_addr = NULL;

    if( sender_soc->sa_family == AF_INET )
        sender_addr = &( ( (struct sockaddr_in*)sender_soc )->sin_addr );
    else
        sender_addr = &( ( (struct sockaddr_in6*)sender_soc )->sin6_addr );

    dd_server_write_out( DDLOG_STATUS,
                         "Recived %zdB packet from: %s\n",
                         bytes_read,
                         inet_ntop( sender.ss_family,
                                    (struct sockaddr*)sender_addr,
                                    ip_str,
                                    sizeof( ip_str ) ) );
#endif
    buff[bytes_read] = '\0';
    dd_server_write_out( DDLOG_NOTAG, "Data: %s\n", buff );

    s_time_tracker = dd_loop_time_seconds( loop );
}

void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer )
{
    double elapsed = dd_loop_time_seconds( loop ) - s_time_tracker;

    if( elapsed > s_timeout_limit )
    {
        dd_server_write_out( DDLOG_WARN,
                             "Timeout limit reached (time elapsed %.5f)\n",
                             (float)elapsed );
        dd_loop_break( loop );
    }
}