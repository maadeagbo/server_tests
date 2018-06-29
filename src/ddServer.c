#include <string.h>
#include "ddServer.h"

void get_server_info( struct ddAddressInfo* restrict address,
                      const char* ip,
                      const char* port )
{
    if( !address ) return;

    char ip_str[INET6_ADDRSTRLEN];

    memset( &address->hints, 0, sizeof( address->hints ) );
    address->hints.ai_family = AF_UNSPEC;
    address->hints.ai_socktype = SOCK_STREAM;

    address->status =
        getaddrinfo( ip, port, &address->hints, &address->options );

    if( address->status != 0 )
    {
        fprintf( stderr, "getaddrinfo %s\n", gai_strerror( address->status ) );
        return;
    }

    freeaddrinfo( address->options );

    return;
}