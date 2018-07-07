#include "ServerInterface.h"
#include "ConsoleWrite.h"
#include "TimeInterface.h"

#if DD_PLATFORM == DD_LINUX
#include <unistd.h>
#elif DD_PLATFORM == DD_WIN32
#include <io.h>
#endif  // DD_PLATFORM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if DD_PLATFORM == DD_WIN32

void dd_server_init_win32()
{
    WSADATA wsaData;

    if( WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) != 0 )
    {
        console_write( LOG_ERROR, "Failed to initialize Win32 WSADATA\n" );
        exit( 1 );
    }

    s_hconsole = GetStdHandle( STD_OUTPUT_HANDLE );
}

void dd_server_cleanup_win32() { WSACleanup(); }
#endif  // DD_PLATFORM

void dd_close_socket( ddSocket* c_restrict socket )
{
#if DD_PLATFORM == DD_WIN32
    closesocket( *socket );
#elif DD_PLATFORM == DD_LINUX
    close( *socket );
#endif  // DD_PLATFORM
}

void dd_create_socket( struct ddAddressInfo* c_restrict address,
                       const char* c_restrict ip,
                       const char* c_restrict port,
                       const bool create_server )
{
    if( !address ) return;

    memset( &address->hints, 0, sizeof( address->hints ) );
    address->hints.ai_family = AF_UNSPEC;
    address->hints.ai_socktype = SOCK_DGRAM;

    address->status =
        getaddrinfo( ip, port, &address->hints, &address->options );

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    console_write( LOG_STATUS, "Creating UDP socket on port %s\n", port );

    if( create_server )
        console_write( LOG_STATUS, "Attempting to create server\n" );

    console_write( LOG_STATUS, "IP addresses for %s:\n", ip );
#endif

    if( address->status != 0 )
    {
        console_write(
            LOG_ERROR, "getaddrinfo %s\n", gai_strerror( address->status ) );
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

        console_write( LOG_NOTAG, "\t%s: %s\n", ipver, ip_str );
#endif

        ddSocket socket_fd = socket(
            next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol );

#if DD_PLATFORM == DD_LINUX
        fcntl( socket_fd, F_SETFL, O_NONBLOCK );
#endif  // DD_PLATFORM

        if( socket_fd == -1 )
        {
            console_write( LOG_WARN,
                           "Socket file descriptor creation error\n" );
            continue;
        }

        address->socket_fd = socket_fd;
        address->selected = next_ip;
        break;
    }

    if( create_server )
    {
        if( address->selected == NULL )
        {
            console_write( LOG_ERROR, "Server failed to bind\n" );
            return;
        }

        int32_t yes = true;
        if( setsockopt( address->socket_fd,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        (const char*)&yes,
                        sizeof( int32_t ) ) == -1 )
        {
            console_write( LOG_ERROR, "Socket port reuse\n" );
            return;
        }

        if( bind( address->socket_fd,
                  address->selected->ai_addr,
                  (int)address->selected->ai_addrlen ) == -1 )
        {
            dd_close_socket( &address->socket_fd );

            console_write( LOG_ERROR, "Socket bind\n" );
            return;
        }

        freeaddrinfo( address->options );

#ifdef VERBOSE
        console_write( LOG_STATUS, "Server waiting on data...\n" );
#endif
    }

    return;
}

void dd_server_send_msg( const struct ddAddressInfo* c_restrict recipient,
                         const uint32_t msg_type,
                         const struct ddMsgVal* c_restrict msg )
{
    size_t msg_length = 0;
    int32_t bytes_sent = 0;
    char output[MAX_MSG_LENGTH];

    switch( msg_type )
    {
        case DDMSG_STR:
            msg_length = strnlen( msg->c, MAX_MSG_LENGTH - 1 );
            snprintf( output, msg_length + 1, "%s", msg->c );
            break;
        default:
            console_write( LOG_ERROR, "Message type unrecognized\n" );
            return;
    }

    if( ( bytes_sent = sendto( recipient->socket_fd,
                               output,
                               (int)msg_length,
                               0,
                               recipient->selected->ai_addr,
                               (int)recipient->selected->ai_addrlen ) ) == -1 )
    {
        console_write( LOG_ERROR, "sendto Failure\n" );
        return;
    }

    console_write(
        LOG_NOTAG, "Sent %zuB out of %uB\n", bytes_sent, msg_length );
}

void dd_server_recieve_msg( const struct ddAddressInfo* c_restrict listener,
                            struct ddRecvMsg* c_restrict msg_data )
{
    msg_data->sender = ( struct sockaddr_storage ){0};
    msg_data->addr_len = sizeof( msg_data->sender );

    msg_data->bytes_read = recvfrom( listener->socket_fd,
                                     msg_data->msg,
                                     MAX_MSG_LENGTH - 1,
                                     0,
                                     (struct sockaddr*)&( msg_data->sender ),
                                     &( msg_data->addr_len ) );

    if( msg_data->bytes_read == -1 )
    {
        console_write( LOG_ERROR, "recvfrom Error\n" );
        return;
    }

    msg_data->msg[msg_data->bytes_read] = '\0';

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    struct sockaddr* sender_soc = (struct sockaddr*)&msg_data->sender;
    void* sender_addr = NULL;

    if( sender_soc->sa_family == AF_INET )
        sender_addr = &( ( (struct sockaddr_in*)sender_soc )->sin_addr );
    else
        sender_addr = &( ( (struct sockaddr_in6*)sender_soc )->sin6_addr );

    console_write( LOG_STATUS,
                   "Recived %zdB packet from: %s\n",
                   msg_data->bytes_read,
                   inet_ntop( msg_data->sender.ss_family,
                              (struct sockaddr*)sender_addr,
                              ip_str,
                              sizeof( ip_str ) ) );
#endif
}

struct ddLoop dd_server_new_loop( dd_loop_cb loop_cb,
                                  struct ddAddressInfo* listener )
{
    return ( struct ddLoop ){
        .start_time = 0,
        .active_time = 0,
        .timers_count = 0,
        .listener = listener,
        .callback = loop_cb,
        .active = true,
    };
}

void dd_loop_add_timer( struct ddLoop* c_restrict loop,
                        dd_timer_cb timer_cb,
                        double seconds,
                        bool repeat )
{
    if( loop->timers_count >= MAX_ACTIVE_TIMERS )
    {
        console_write( LOG_ERROR, "Loop timer limit reached. Abort add\n" );
        return;
    }

    loop->timer_cbs[loop->timers_count] = timer_cb;

    loop->timer_update[loop->timers_count] = 0;

    loop->timers[loop->timers_count] = ( struct ddServerTimer ){
        .tick_rate = seconds_to_nano( seconds ), .repeat = repeat,
    };

    loop->timers_count++;
}

int64_t dd_loop_time_nano( struct ddLoop* c_restrict loop )
{
    return loop->active_time - loop->start_time;
}

double dd_loop_time_seconds( struct ddLoop* c_restrict loop )
{
    return nano_to_seconds( loop->active_time - loop->start_time );
}

void dd_loop_break( struct ddLoop* loop ) { loop->active = false; }
void dd_loop_run( struct ddLoop* loop )
{
    fd_set master;
    fd_set read_fd;
    FD_ZERO( &master );
    FD_ZERO( &read_fd );

    int32_t fdmax = (int)loop->listener->socket_fd;  // ignored on windows lol
    FD_SET( fdmax, &master );

    loop->start_time = loop->active_time = get_high_res_time();

    for( uint32_t i = 0; i < loop->timers_count; i++ )
        loop->timer_update[i] = loop->start_time;

    while( loop->active )
    {
        // usec == 1e6 sec
        struct timeval select_timeout = {
            .tv_sec = 0, .tv_usec = 100,
        };

        read_fd = master;

        int32_t rc = select( fdmax + 1, &read_fd, NULL, NULL, &select_timeout );

        if( rc == -1 )
        {
            console_write( LOG_ERROR, "Select error\n" );
            break;
        }

        // look for data
        if( rc == 1 ) loop->callback( loop );

        if( !loop->active ) return;

        // check timers
        uint32_t timer_idx = 0;
        while( timer_idx < loop->timers_count )
        {
            const uint64_t delta =
                loop->active_time - loop->timer_update[timer_idx];

            struct ddServerTimer* timer = &loop->timers[timer_idx];

            if( timer->tick_rate < delta )
            {
                loop->timer_cbs[timer_idx]( loop, timer );

                if( !loop->active ) return;

                if( timer->repeat )
                    loop->timer_update[timer_idx] = loop->active_time;
                else
                {
                    loop->timers_count--;

                    loop->timers[timer_idx] = loop->timers[loop->timers_count];
                    continue;
                }
            }

            timer_idx++;
        }

        loop->active_time = get_high_res_time();
    }
}
