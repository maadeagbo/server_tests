#include <stdio.h>

#include "ddConfig.h"
#include "ArgHandler.h"
#include "ConsoleWrite.h"
#include "ServerInterface.h"
#include "TimeInterface.h"

static struct sockaddr_storage s_clients[BACKLOG];
static uint32_t s_num_clients;

static double s_time_tracker = 0.0;
static double s_timeout_limit = 0.0;

void read_cb( struct ddLoop* loop );
void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer );

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

    struct ddArgStat msg_arg = {
        .description = "Sets msg to be sent over udp ( default : \"empty\" )",
        .full_id = "msg",
        .type_flag = ARG_STR,
        .short_id = 'm',
        .default_val = {.c = "empty"}};

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );
    register_arg( &arg_handler, &server_arg );
    register_arg( &arg_handler, &timeout_arg );
    register_arg( &arg_handler, &msg_arg );

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
    console_write(
        LOG_WARN, "Timeout set to %.5f secs\n", (float)s_timeout_limit );
#endif  // VERBOSE

    if( server_addr.selected == NULL )
    {
        console_write( LOG_ERROR, "Socket not created\n" );
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
        struct ddMsgVal msg = {.c = extract_arg( &arg_handler, 'm' )->val.c};

        dd_server_send_msg( &server_addr, DDMSG_STR, &msg );

        freeaddrinfo( server_addr.options );
    }

    dd_close_socket( &server_addr.socket_fd );
#ifdef _WIN32
    void dd_server_cleanup_win32();
#endif  // _WIN32

#ifdef VERBOSE
    console_write( LOG_STATUS, "Closing server/client program\n" );
#endif  // VERBOSE

    return 0;
}

void read_cb( struct ddLoop* loop )
{
    struct ddRecvMsg data = {
        .bytes_read = 0,
    };

    dd_server_recieve_msg( loop->listener, &data );

    if( data.bytes_read == -1 )
        dd_loop_break( loop );
    else
    {
        if( s_num_clients < BACKLOG )
        {
            s_clients[s_num_clients] = data.sender;
            s_num_clients++;
        }

        console_write( LOG_NOTAG, "Data: %s\n", data.msg );

        s_time_tracker = dd_loop_time_seconds( loop );
    }
}

void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer )
{
	UNUSED_VAR( timer );

    double elapsed = dd_loop_time_seconds( loop ) - s_time_tracker;

    if( elapsed > s_timeout_limit )
    {
        console_write( LOG_WARN,
                       "Timeout limit reached (time elapsed %.5f)\n",
                       (float)elapsed );
		
		struct ddMsgVal msg = { .c = "Closing connection" };

		struct addrinfo send = (struct addrinfo){0};

		struct ddAddressInfo client_info = {
			.socket_fd = loop->listener->socket_fd,
		};
		client_info.selected = &send;

		for (uint32_t i = 0; i < s_num_clients; i++)
		{
			client_info.selected->ai_addr = (struct sockaddr*)&s_clients[i];
			client_info.selected->ai_addrlen = sizeof( s_clients[i] );

			dd_server_send_msg( &client_info, DDMSG_STR, &msg);
		}
        dd_loop_break( loop );
    }
}