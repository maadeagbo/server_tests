#include "ddConfig.h"
#include "ddArgHandler.h"
#include "ddServer.h"

int main( int argc, char const* argv[] )
{
    struct ddArgHandler arg_handler;
    init_arg_handler( &arg_handler,
                      "Creates Server instance using provided IP address."
                      "Default address is localhost" );

    register_arg( &arg_handler, "IP", 'i', ARG_STR );

    poll_args( &arg_handler, argc, argv );

    if( extract_arg( &arg_handler, 'h' )->b )
    {
        print_arg_help_msg( &arg_handler );
        return 0;
    }

    struct ddAddressInfo server_addr;

    get_server_info( &server_addr );

    return 0;
}
