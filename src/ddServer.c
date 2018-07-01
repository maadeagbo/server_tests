#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ddServer.h"

void dd_server_write_out( const uint32_t log_type,
                          const char* restrict fmt_str,
                          ... )
{
    FILE* file = stdout;
    const char* type = 0;

    switch( log_type )
    {
        case DDLOG_STATUS:
            type = "status";
            break;
        case DDLOG_ERROR:
            type = "error";
            break;
        case DDLOG_WARNING:
            type = "warning";
            break;
        case DDLOG_NOTAG:
            type = " ";
        default:
            type = " ";
            break;
    }
    fprintf( file, "[%10s]  ", type );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( file, fmt_str, args );
    va_end( args );
}

void dd_create_socket( struct ddAddressInfo* restrict address,
                       const char* restrict ip,
                       const char* restrict port,
                       const bool create_server,
                       const bool use_tcp )
{
    if( !address ) return;

    memset( &address->hints, 0, sizeof( address->hints ) );
    address->hints.ai_family = AF_UNSPEC;
    address->hints.ai_socktype = use_tcp ? SOCK_STREAM : SOCK_DGRAM;

    address->status =
        getaddrinfo( ip, port, &address->hints, &address->options );

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    dd_server_write_out(
        DDLOG_STATUS, "Creating %s socket\n", use_tcp ? "TCP" : "UDP" );

    if( create_server )
        dd_server_write_out( DDLOG_STATUS, "Creating server\n" );

    dd_server_write_out( DDLOG_STATUS, "IP addresses for %s:\n", ip );
#endif

    if( address->status != 0 )
    {
        dd_server_write_out(
            DDLOG_ERROR, "getaddrinfo %s\n", gai_strerror( address->status ) );
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

        dd_server_write_out( DDLOG_NOTAG, "\t%s: %s\n", ipver, ip_str );
#endif

        int32_t socket_fd = socket(
            next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol );

        if( socket_fd == -1 )
        {
            dd_server_write_out( DDLOG_WARNING,
                                 "Socket file descriptor creation error\n" );
            continue;
        }

        address->socket_fd = socket_fd;
        address->selected = next_ip;
        break;
    }

    freeaddrinfo( address->options );

    if( create_server )
    {
        if( address->selected == NULL )
        {
            dd_server_write_out( DDLOG_ERROR, "Server failed to bind\n" );
            return;
        }

        int32_t yes = true;
        if( setsockopt( address->socket_fd,
                        SOL_SOCKET,
                        use_tcp ? SO_REUSEADDR : SO_BROADCAST,
                        &yes,
                        sizeof( int32_t ) ) == -1 )
        {
            dd_server_write_out( DDLOG_ERROR, "Socket port reuse\n" );
            return;
        }

        if( bind( address->socket_fd,
                  address->selected->ai_addr,
                  address->selected->ai_addrlen ) == -1 )
        {
            close( address->socket_fd );
            dd_server_write_out( DDLOG_ERROR, "Socket bind\n" );
            return;
        }

        if( use_tcp )
        {
            if( listen( address->socket_fd, BACKLOG ) == -1 )
            {
                dd_server_write_out( DDLOG_ERROR,
                                     "Server failed to listen on port\n" );
                return;
            }
        }
// fcntl( address->socket_fd, F_SETFL, O_NONBLOCK );

#ifdef VERBOSE
        dd_server_write_out( DDLOG_STATUS,
                             "Server waiting on connections...\n" );
#endif
    }

    return;
}

void dd_ev_io_watcher( struct ddIODataEV* restrict io_data )
{
    ev_io_init( io_data->io_ptr,
                io_data->cb_func,
                io_data->socketfd,
                io_data->io_flags );

    ev_io_start( io_data->loop_ptr, io_data->io_ptr );
}

void dd_ev_timer_watcher( struct ddTimerDataEV* restrict timer_data )
{
    if( timer_data->repeat_flag )
    {
        ev_init( timer_data->timer_ptr, timer_data->cb_func );

        ( *timer_data->timer_ptr ).repeat = 0.1;  // second resolution

        ev_timer_again( timer_data->loop_ptr, timer_data->timer_ptr );
    }
    else
    {
        //
        return;
    }
}

void dd_ev_signal_watcher( struct ddSignalDataEV* restrict signal_data )
{
    ev_signal_init(
        signal_data->signal_ptr, signal_data->cb_func, signal_data->signal );
    ev_signal_start( signal_data->loop_ptr, signal_data->signal_ptr );
}

void dd_server_send_msg( struct ddMsgData* restrict msg )
{
    //
}