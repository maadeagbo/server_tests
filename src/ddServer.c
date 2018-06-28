#include <string.h>
#include "ddServer.h"

void print_hello( const char* restrict extra_str )
{
    printf( "Hello World...from %s\n", extra_str );
}

void get_server_info( struct ddAddressInfo* restrict address )
{
    char ip_str[INET6_ADDRSTRLEN];

    return;
}