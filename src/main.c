#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "Config.h"
#include "ArgHandler.h"
#include "ConsoleWrite.h"
#include "ServerInterface.h"
#include "TimeInterface.h"

#define IP_LENGTH INET6_ADDRSTRLEN

static struct ServerAddressInfo s_clients[BACKLOG];
static uint32_t s_num_clients;

static void read_cb( struct ServerLoop* loop );
static void time_cb( struct ServerLoop* loop, struct ServerTimer* timer );

static char input_msg[MAX_MSG_SIZE];

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

#if PLATFORM == PF_WIN32
    server_init_win32();
#endif  // PLATFORM == PF_WIN32

    // set up local server
    struct ServerAddressInfo server_addr = {.options = NULL, .selected = NULL};

    const char* ip_addr_str = extract_arg( &arg_handler, 'i' )->val.c;
    const char* port_str = extract_arg( &arg_handler, 'p' )->val.c;

    server_create_socket( &server_addr, ip_addr_str, port_str, true );

    if( server_addr.selected == NULL )
    {
        console_write( LOG_ERROR, "Socket not created" );
        return 1;
    }

    struct ServerLoop looper = server_server_new_loop( read_cb, &server_addr );

    // add timed callback for processing messages
    server_loop_add_timer( &looper, time_cb, 0.1, true );

    server_loop_run( &looper );

    // cleanup resources
    server_close_socket( &server_addr.socket_fd );
    server_close_clients( s_clients, s_num_clients );

#if PLATFORM == PF_WIN32
    void server_cleanup_win32();
#endif  // PLATFORM == PF_WIN32

#ifdef VERBOSE
    console_write( LOG_STATUS, "Closing server/client program" );
#endif  // VERBOSE

    return 0;
}

static void translate_msg( const char* const c_restrict msg )
{
    assert( msg );

    uint64_t tag;
    memcpy( &tag, msg, sizeof( uint64_t ) );
    uint32_t mask = ( 1 << 4 ) - 1;

    enum ConsoleOutType console_type = tag & mask;

    console_write( console_type, "%s", msg + sizeof( tag ) );
}

static void read_cb( struct ServerLoop* loop )
{
    // buffer for message retrieval
    struct ServerRecvMsg data = {
        .bytes_read = 0,
    };

    server_server_recieve_msg( loop->listener, &data );

    if( data.bytes_read == -1 )
        server_loop_break( loop );  // server read error
    else
        translate_msg( data.msg );
}

static void time_cb( struct ServerLoop* loop, struct ServerTimer* timer )
{
    UNUSED_VAR( timer );

    query_input( input_msg, MAX_MSG_SIZE );

    if( *input_msg )
    {
        if( strcmp( input_msg, "exit" ) == 0 ) server_loop_break( loop );
        // process new ip to send messages to
        else if( input_msg[0] == '@' )
        {
            char* port_ptr = strchr( input_msg + 1, '#' );

            if( port_ptr )
            {
                const size_t ip_len = port_ptr - input_msg;

                snprintf( s_clients[s_num_clients].ip_raw,
                          ip_len < IP_LENGTH ? ip_len : IP_LENGTH,
                          "%s",
                          input_msg + 1 );
                snprintf(
                    s_clients[s_num_clients].port_raw, 10, "%s", port_ptr + 1 );

                // remove whitespace ( can lead to failed connections )
                char* whitespace =
                    strchr( s_clients[s_num_clients].ip_raw, ' ' );
                if( whitespace ) *whitespace = '\0';

                whitespace = strchr( s_clients[s_num_clients].port_raw, ' ' );
                if( whitespace ) *whitespace = '\0';

                for( uint32_t i = 0; i < s_num_clients; i++ )
                    if( strcmp( s_clients[i].ip_raw,
                                s_clients[s_num_clients].ip_raw ) == 0 )
                    {
                        console_write( LOG_WARN,
                                       "Connection attempt ignored-> IP: %s",
                                       s_clients[s_num_clients].ip_raw );
                        return;
                    }

                // connection attempt
                server_create_socket( &s_clients[s_num_clients],
                                      s_clients[s_num_clients].ip_raw,
                                      s_clients[s_num_clients].port_raw,
                                      false );

                if( s_clients[s_num_clients].selected == NULL )
                    console_write(
                        LOG_ERROR,
                        "Connection un-established-> IP: %s PORT: %s",
                        s_clients[s_num_clients].ip_raw,
                        s_clients[s_num_clients].port_raw );
                else
                    s_num_clients++;
            }
        }
        else
        {
            for( uint32_t i = 0; i < s_num_clients; i++ )
                server_server_send_msg( &s_clients[i], input_msg );

            input_msg[0] = '\0';
        }
    }
}
