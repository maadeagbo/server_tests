#include <stdio.h>
#include <string.h>

#include "ddConfig.h"
#include "ArgHandler.h"
#include "ConsoleWrite.h"
#include "ServerInterface.h"
#include "TimeInterface.h"

#define IP_LENGTH INET6_ADDRSTRLEN

static struct ddAddressInfo s_clients[BACKLOG];
static char s_client_ips[BACKLOG][IP_LENGTH];
static char s_client_ports[BACKLOG][10];
static uint32_t s_num_clients;

static void read_cb( struct ddLoop* loop );
static void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer );

static char input_msg[MAX_MSG_LENGTH];

int main( int argc, char const* argv[] )
{
	// set up input to program
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

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );

	// get arguments passed in
    poll_args( &arg_handler, argc, argv );

    if( extract_arg( &arg_handler, 'h' )->val.b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

#if DD_PLATFORM == DD_WIN32
    dd_server_init_win32();
#endif  // DD_PLATFORM == DD_WIN32

	// set up local server
    struct ddAddressInfo server_addr = {.options = NULL, .selected = NULL};

    const char* ip_addr_str = extract_arg( &arg_handler, 'i' )->val.c;
    const char* port_str = extract_arg( &arg_handler, 'p' )->val.c;

    dd_create_socket( &server_addr, ip_addr_str, port_str, true );

    if( server_addr.selected == NULL )
    {
        console_write( LOG_ERROR, "Socket not created\n" );
        return 1;
    }

    struct ddLoop looper = dd_server_new_loop( read_cb, &server_addr );

	// add timed callback for processing messages
    dd_loop_add_timer( &looper, timer_cb, 0.1, true );

    dd_loop_run( &looper );

	// cleanup resources
    dd_close_socket( &server_addr.socket_fd );
    dd_close_clients( s_clients, s_num_clients );

#ifdef DD_PLATFORM == DD_WIN32
    void dd_server_cleanup_win32();
#endif  // DD_PLATFORM == DD_WIN32

#ifdef VERBOSE
    console_write( LOG_STATUS, "Closing server/client program\n" );
#endif  // VERBOSE

    return 0;
}

static void read_cb( struct ddLoop* loop )
{
	// buffer for message retrieval
    struct ddRecvMsg data = {
        .bytes_read = 0,
    };

    dd_server_recieve_msg( loop->listener, &data );

    if( data.bytes_read == -1 )
        dd_loop_break( loop ); // server read error
    else
    {
		// add 1st responder to messaging list
        if( s_num_clients == 0 )
        {
            const bool success = dd_create_socket2( &s_clients[s_num_clients],
                                                    &data.sender,
                                                    loop->listener->port_num );
            if( success ) s_num_clients++;
        }

        console_write( LOG_NOTAG, "Data recieved: %s\n", data.msg );
    }
}

static void timer_cb( struct ddLoop* loop, struct ddServerTimer* timer )
{
    UNUSED_VAR( timer );

    query_input( input_msg, MAX_MSG_LENGTH );

    if( *input_msg )
    {
        if( strcmp( input_msg, "exit" ) == 0 ) dd_loop_break( loop );
        // process new ip to send messages to
        else if( input_msg[0] == '@' )
        {
            char* port_ptr = strchr( input_msg + 1, '#' );

            if( port_ptr )
            {
                const size_t ip_len = port_ptr - input_msg;

                snprintf( s_client_ips[s_num_clients],
                          ip_len < IP_LENGTH ? ip_len : IP_LENGTH,
                          "%s",
                          input_msg + 1 );
                snprintf(
                    s_client_ports[s_num_clients], 10, "%s", port_ptr + 1 );

				// remove whitespace ( can lead to failed connections )
				char* whitespace = strchr( s_client_ips[s_num_clients], ' ' );
				if( whitespace ) *whitespace = '\0';

				whitespace = strchr( s_client_ports[s_num_clients], ' ' );
				if( whitespace ) *whitespace = '\0';

				// connection attempt
                dd_create_socket( &s_clients[s_num_clients],
                                  s_client_ips[s_num_clients],
                                  s_client_ports[s_num_clients],
                                  false );

                if( s_clients[s_num_clients].selected == NULL )
                    console_write(
                        LOG_ERROR,
                        "Connection un-established-> IP: %s PORT: %s\n",
                        s_client_ips[s_num_clients],
                        s_client_ports[s_num_clients] );
                else
                    s_num_clients++;
            }
        }
        else
        {
			// send message to all connections
            struct ddMsgVal msg = {
                .c = input_msg,
            };

            for( uint32_t i = 0; i < s_num_clients; i++ )
                dd_server_send_msg( &s_clients[i], DDMSG_STR, &msg );

            input_msg[0] = '\0';
        }
    }
}