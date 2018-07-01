#include <stdio.h>
#include "ddConfig.h"
#include "ddArgHandler.h"
#include "ddServer.h"

ev_io io_watchers[BACKLOG];
ev_timer timer_watcher;
ev_signal sig_watcher;

ev_tstamp idle_time = 0.0;
ev_tstamp timeout_limit = 0.0;

static void io_callback( EV_P_ ev_io* io_w, int r_events );

static void timer_callback( struct ev_loop* t_loop,
                            ev_timer* timer_w,
                            int r_events );

static void sigint_callback( struct ev_loop* sig_loop,
                             ev_signal* sig_w,
                             int r_events );

int main( int argc, char const* argv[] )
{
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

    struct ddArgStat tcp_arg = {
        .description = "Select TCP or UDP connection ( default : UDP )",
        .full_id = "tcp",
        .type_flag = ARG_BOOL,
        .short_id = 'u',
        .default_val = {.b = false}};

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );
    register_arg( &arg_handler, &server_arg );
    register_arg( &arg_handler, &timeout_arg );
    register_arg( &arg_handler, &tcp_arg );

    poll_args( &arg_handler, argc, argv );

    timeout_limit = extract_arg( &arg_handler, 't' )->val.f;

    if( extract_arg( &arg_handler, 'h' )->val.b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

    struct ddAddressInfo server_addr;

    const char* ip_addr_str = extract_arg( &arg_handler, 'i' )->val.c;
    const char* port_str = extract_arg( &arg_handler, 'p' )->val.c;
    const bool listen_flag = extract_arg( &arg_handler, 's' )->val.b;
    const bool tcp_flag = extract_arg( &arg_handler, 'u' )->val.b;

    dd_create_socket(
        &server_addr, ip_addr_str, port_str, listen_flag, tcp_flag );

    if( server_addr.socket_fd < 0 )
    {
        dd_server_write_out( DDLOG_ERROR, "Socket not created\n" );
        return 1;
    }

    struct ev_loop* loop = EV_DEFAULT;

    if( listen_flag )
    {
        // first watcher is always the primary one
        struct ddIODataEV io_info = {
            .socketfd = server_addr.socket_fd,
            .io_flags = EV_READ | EV_WRITE,
            .io_ptr = io_watchers,
            .cb_func = io_callback,
            .loop_ptr = loop,
        };

        dd_ev_io_watcher( &io_info );

        struct ddTimerDataEV timer_info = {
            .interval = 0.1,
            .timer_ptr = &timer_watcher,
            .cb_func = timer_callback,
            .loop_ptr = loop,
            .repeat_flag = true,
        };

        dd_ev_timer_watcher( &timer_info );

        struct ddSignalDataEV sig_info = {
            .signal = SIGINT,
            .signal_ptr = &sig_watcher,
            .cb_func = sigint_callback,
            .loop_ptr = loop,
        };

        dd_ev_signal_watcher( &sig_info );

        ev_run( loop, 0 );
    }
    else
    {
        // parse message
    }

    return 0;
}

static void io_callback( EV_P_ ev_io* io_w, int r_events )
{
    if( io_w == io_watchers ) return;

    if( r_events & EV_READ )
    {
        idle_time = 0.0;
        dd_server_write_out( DDLOG_STATUS, "Got data\n" );
    }
}

static void timer_callback( struct ev_loop* t_loop,
                            ev_timer* timer_w,
                            int r_events )
{
    idle_time += timer_w->repeat;

    if( idle_time > timeout_limit )
    {
        dd_server_write_out( DDLOG_STATUS,
                             "Timeout limit reached. Closing connection.\n" );
        ev_break( t_loop, EVBREAK_ALL );
    }
}

static void sigint_callback( struct ev_loop* sig_loop,
                             ev_signal* sig_w,
                             int r_events )
{
    dd_server_write_out( DDLOG_STATUS, "FORCE_CLOSE. Closing connection.\n" );
    ev_break( sig_loop, EVBREAK_ALL );
}