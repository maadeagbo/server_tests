#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "ddServer.h"

void create_socket( struct ddAddressInfo* restrict address,
                    const char* ip,
                    const char* port,
                    const bool create_server )
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

    // find useable soscket for port
    for( struct addrinfo* next_ip = address->options; next_ip != NULL;
         next_ip = next_ip->ai_next )
    {
#ifdef VERBOSE
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

        inet_ntop( next_ip->ai_family, addr, ip_str, sizeof( ip_str ) );
        printf( "  %s: %s\n", ipver, ip_str );
#endif

        int32_t yes = true;
        int32_t socket_fd = socket(
            next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol );

        if( socket_fd == -1 )
        {
            fprintf( stderr, "Skip::Socket file descriptor creation error" );
            continue;
        }

        if( setsockopt( socket_fd,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        &yes,
                        sizeof( int32_t ) ) == -1 )
        {
            fprintf( stderr, "Skip::Socket port reuse" );
            continue;
        }

        if( bind( socket_fd, next_ip->ai_addr, next_ip->ai_addrlen ) == -1 )
        {
            close( socket_fd );
            fprintf( stderr, "Skip::Socket bind" );
            continue;
        }

        address->socket_fd = socket_fd;
        address->selected = next_ip;
        break;
    }

    freeaddrinfo( address->options );

    return;
}