#include "ServerInterface.h"
#include "ConsoleWrite.h"
#include "TimeInterface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if PLATFORM == PF_WIN32

void server_init_win32()
{
    // windows server api requires initialization
    WSADATA wsaData;

    if( WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) != 0 )
    {
        console_write( LOG_ERROR, "Failed to initialize Win32 WSADATA\n" );
        exit( 1 );
    }
}

void server_cleanup_win32()
{
    // windows server api requires cleanup
    WSACleanup();
}

#endif  // PLATFORM

void server_close_socket( ServerSocket* c_restrict socket )
{
#if PLATFORM == PF_WIN32
    closesocket( *socket );
#elif PLATFORM == PF_LINUX
    close( *socket );
#endif  // PLATFORM
}

void server_close_clients( struct ServerAddressInfo* c_restrict clients,
                           const uint32_t count )
{
    for( uint32_t i = 0; i < count; i++ )
        server_close_socket( &clients[i].socket_fd );
}

static bool create_socket_base( struct ServerAddressInfo* c_restrict address,
                                const char* const c_restrict ip,
                                const char* const c_restrict port )
{
    if( !address || !ip || !port ) return false;

    // udp-type socket struct
    memset( &address->hints, 0, sizeof( address->hints ) );
    address->hints.ai_family = AF_UNSPEC;
    address->hints.ai_socktype = SOCK_DGRAM;

    address->status =
        getaddrinfo( ip, port, &address->hints, &address->options );

#ifdef VERBOSE
    char ip_str[INET6_ADDRSTRLEN];

    console_write( LOG_STATUS, "Creating UDP socket on port %s\n", port );

    console_write( LOG_STATUS, "IP addresses for %s:\n", ip );
#endif

    if( address->status != 0 )
    {
        console_write(
            LOG_ERROR, "getaddrinfo %s\n", gai_strerror( address->status ) );
        return false;
    }

    // find useable socket for port
    address->port_num = strtol( port, NULL, 10 );

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
        // create socket
        ServerSocket socket_fd = socket(
            next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol );

#if PLATFORM == PF_LINUX
        fcntl( socket_fd, F_SETFL, O_NONBLOCK );  // make non-blocking
#elif PLATFORM == PF_WIN32
        unsigned long mode = 1;
        const bool success =
            ioctlsocket( socket_fd, FIONBIO, &mode ) == 0;  // make non-blocking

        if( !success )
            console_write( LOG_WARN,
                           "Socket could not be made non-blocking\n" );
#endif  // PLATFORM == PF_WIN32

        if( socket_fd == -1 )
        {
            console_write( LOG_WARN,
                           "Socket file descriptor creation error\n" );
            continue;
        }

        // set return socket
        address->socket_fd = socket_fd;
        address->selected = next_ip;  // can be deleted if socket_fd is copied

        return true;
    }
    return false;
}

void server_create_socket( struct ServerAddressInfo* c_restrict address,
                           const char* const c_restrict ip,
                           const char* const c_restrict port,
                           const bool create_server )
{
    const bool success = create_socket_base( address, ip, port );

    if( !success ) return;

    if( create_server )
    {
#ifdef VERBOSE
        console_write( LOG_STATUS, "Attempting to create server\n" );
#endif  // VERBOSE

        if( address->selected == NULL )
        {
            console_write( LOG_ERROR, "Server failed to bind\n" );
            return;
        }

        // free port if blocked from use
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

        // bind socket to port
        if( bind( address->socket_fd,
                  address->selected->ai_addr,
                  (int)address->selected->ai_addrlen ) == -1 )
        {
            server_close_socket( &address->socket_fd );

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

bool server_create_socket2( struct ServerAddressInfo* c_restrict address,
                            struct sockaddr_storage* client,
                            const uint32_t port )
{
    // convert port and ip address to string format for socket creation
    char port_str[10];
    snprintf( port_str, sizeof( port_str ), "%d", port );

    char ip_str[INET6_ADDRSTRLEN];

    struct sockaddr* client_soc = (struct sockaddr*)client;
    void* addr = NULL;

    if( client_soc->sa_family == AF_INET )
        addr = &( (struct sockaddr_in*)client_soc )->sin_addr;  // IPv4
    else
        addr = &( (struct sockaddr_in6*)client_soc )->sin6_addr;  // IPv6

    inet_ntop(
        client->ss_family, (struct sockaddr*)addr, ip_str, sizeof( ip_str ) );

    return create_socket_base( address, ip_str, port_str );
}

void server_server_send_msg( const struct ServerAddressInfo* c_restrict
                                 recipient,
                             const char* const c_restrict msg )
{
    size_t msg_length = 0;
    int32_t bytes_sent = 0;
    char output[MAX_MSG_SIZE];

    // format message for compression (if implemented)
    msg_length = strnlen( msg, MAX_MSG_SIZE - 1 );
    snprintf( output, msg_length + 1, "%s", msg );

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

#ifdef VERBOSE
    void* addr;
    struct addrinfo* recvr = recipient->selected;
    char ip_str[INET6_ADDRSTRLEN];

    if( recvr->ai_family == AF_INET )
        addr = &( ( (struct sockaddr_in*)recvr )->sin_addr );  // IPv4
    else
        addr = &( ( (struct sockaddr_in6*)recvr )->sin6_addr );  // IPv6

    inet_ntop(
        recvr->ai_family, (struct sockaddr*)addr, ip_str, sizeof( ip_str ) );

    console_write( LOG_NOTAG,
                   "Sent %zuB out of %uB to %s on port %u\n",
                   bytes_sent,
                   msg_length,
                   ip_str,
                   recipient->port_num );
#endif
}

void server_server_recieve_msg( const struct ServerAddressInfo* c_restrict
                                    listener,
                                struct ServerRecvMsg* msg_data )
{
    msg_data->sender = ( struct sockaddr_storage ){0};
    msg_data->addr_len = sizeof( msg_data->sender );

    msg_data->bytes_read = recvfrom( listener->socket_fd,
                                     msg_data->msg,
                                     MAX_MSG_SIZE - 1,
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

struct ServerLoop server_server_new_loop( loop_cb loop_cb,
                                          struct ServerAddressInfo* listener )
{
    return ( struct ServerLoop ){
        .start_time = 0,
        .active_time = 0,
        .timers_count = 0,
        .listener = listener,
        .callback = loop_cb,
        .active = true,
    };
}

void server_loop_add_timer( struct ServerLoop* c_restrict loop,
                            timer_cb timer_cb,
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

    loop->timers[loop->timers_count] = ( struct ServerTimer ){
        .tick_rate = seconds_to_nano( seconds ),
        .repeat = repeat,
    };

    loop->timers_count++;
}

int64_t server_loop_time_nano( struct ServerLoop* c_restrict loop )
{
    return loop->active_time - loop->start_time;
}

double server_loop_time_seconds( struct ServerLoop* c_restrict loop )
{
    return nano_to_seconds( loop->active_time - loop->start_time );
}

void server_loop_break( struct ServerLoop* loop )
{
    loop->active = false;
    console_restore_stdin();
}

void server_loop_run( struct ServerLoop* loop )
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
        console_collect_stdin();

        // usec == 1e-6 sec
        struct timeval select_timeout = {
            .tv_sec = 0,
            .tv_usec = 1000,
        };

        read_fd = master;

        int32_t rc = select( fdmax + 1, &read_fd, NULL, NULL, &select_timeout );

        if( rc == -1 )
        {
            console_write( LOG_ERROR, "Select error\n" );
            break;
        }

        // process data thru callback
        if( rc == 1 ) loop->callback( loop );

        if( !loop->active ) return;

        // check timers
        uint32_t timer_idx = 0;
        while( timer_idx < loop->timers_count )
        {
            const uint64_t delta =
                loop->active_time - loop->timer_update[timer_idx];

            struct ServerTimer* timer = &loop->timers[timer_idx];

            // update timers
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
