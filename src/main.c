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

    register_arg( &arg_handler, &ip_arg );
    register_arg( &arg_handler, &port_arg );

    poll_args( &arg_handler, argc, argv );

    if( extract_arg( &arg_handler, 'h' )->val.b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

    struct ddAddressInfo server_addr;

    get_server_info( &server_addr,
                     extract_arg( &arg_handler, 'i' )->val.c,
                     extract_arg( &arg_handler, 'p' )->val.c );

    return 0;
}
