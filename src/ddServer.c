#include <string.h>
#include "ddServer.h"

void get_server_info( struct ddAddressInfo* restrict address,
                      const char* ip,
                      const char* port )
{
    if( !address ) return;

    memset( &address->hints, 0, sizeof( address->hints ) );
    address->hints.ai_family = AF_UNSPEC;
    address->hints.ai_socktype = SOCK_STREAM;

    address->status =
        getaddrinfo( ip, port, &address->hints, &address->options );

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    printf( "IP addresses for %s:\n", ip );
#endif

    if( address->status != 0 )
    {
        fprintf( stderr, "getaddrinfo %s\n", gai_strerror( address->status ) );
        return;
    }

    for( struct addrinfo* next_ip = address->options; next_ip != NULL;
         next_ip = next_ip->ai_next )
    {
        void* addr;
        const char* ipver;

        if( next_ip->ai_family == AF_INET )  // IPv4
        {
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)next_ip->ai_addr;
            addr = &( ipv4->sin_addr );
            ipver = "IPv4";
        }
        else  // IPv6
        {
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)next_ip->ai_addr;
            addr = &( ipv6->sin6_addr );
            ipver = "IPv6";
        }

#ifdef VERBOSE
        inet_ntop( next_ip->ai_family, addr, ip_str, sizeof( ip_str ) );
        printf( "  %s: %s\n", ipver, ip_str );
#endif
    }

    freeaddrinfo( address->options );

    return;
}