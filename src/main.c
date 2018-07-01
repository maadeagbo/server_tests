#include <stdio.h>
#include "ddConfig.h"
#include "ddArgHandler.h"
#include "ddServer.h"

int main( int argc, char const* argv[] )
{
    struct ddArgHandler arg_handler;
    init_arg_handler( &arg_handler,
                      "Creates Server instance using provided IP address." );

    struct ddArgStat ip_arg = {
        .description = "IP address to connect to (default : \"localhost\")",
        .full_id = "IP",
        .type_flag = ARG_STR,
        .short_id = 'i',
        .default_val = {.c = "localhost"}};

    struct ddArgStat port_arg = {
        .description = "Port to connect to (default : 4321)",
        .full_id = "port",
        .type_flag = ARG_STR,
        .short_id = 'p',
        .default_val = {.c = "4321"}};

    struct ddArgStat server_arg = {
        .description = "Set instance as server (default : false)",
        .full_id = "server",
        .type_flag = ARG_BOOL,
        .short_id = 's',
        .default_val = {.b = true}};

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );
    register_arg( &arg_handler, &server_arg );

    poll_args( &arg_handler, argc, argv );

    if( extract_arg( &arg_handler, 'h' )->val.b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

    struct ddAddressInfo server_addr;

    create_socket( &server_addr,
                   extract_arg( &arg_handler, 'i' )->val.c,
                   extract_arg( &arg_handler, 'p' )->val.c );
    
    if( server_addr.socket_fd < 0 )
    {
        fprintf( stderr, "Fatal Error::Socket not created" );
        return 1;
    }
    
    if( extract_arg( &arg_handler, 's' )->val.b )
    {
        if( server_addr.selected == NULL )
        {
            fprintf( stderr, "Fatal Error::Server failed to bind" );
            return 1;
        }

        if( listen( server_addr.socket_fd, BACKLOG ) == -1 )
        {
            fprintf( stderr, "Fatal Error::Server failed to listen on port" );
            return 1;
        }

#ifdef VERBOSE
        printf( "Server waiting on connections...\n" );
#endif
    }

    return 0;
}
