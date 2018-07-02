#ifdef __linux__
#include <unistd.h>
#else
#include <io.h>
#endif // __linux__

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ddServer.h"

#ifdef _WIN32
HANDLE  hConsole;
int32_t console_color_reset;
#endif // _WIN32

#ifndef ENUM_VAL
#define ENUM_VAL( x ) 1 << x
#endif  // !ENUM_VAL

enum
{
	CONSOLE_R = ENUM_VAL( 0 ),
	CONSOLE_Y = ENUM_VAL( 1 ),
	CONSOLE_G = ENUM_VAL( 2 ),
};

#undef ENUM_VAL

static void set_output_color( uint8_t color ) 
{

	//FlushConsoleInputBuffer( hConsole );

	switch (color)
	{
	case CONSOLE_R:
#ifdef _WIN32
		SetConsoleTextAttribute( hConsole, 12 );
#else
		fprintf( stdout, "\033[31;1;1m" );
#endif // _WIN32
		break;
	case CONSOLE_Y:
#ifdef _WIN32
		SetConsoleTextAttribute( hConsole, 14 );
#else
		fprintf( stdout, "\033[33;1;1m" );
#endif // _WIN32
		break;
	case CONSOLE_G:
#ifdef _WIN32
		SetConsoleTextAttribute( hConsole, 10 );
#else
		fprintf( stdout, "\033[32;1;1m" );
#endif // _WIN32
		break;
	default:
#ifdef _WIN32
		SetConsoleTextAttribute( hConsole, 15 );
#else
		fprintf( stdout, "\033[0m" );
#endif // _WIN32
		break;
	}
}

void dd_server_init_win32()
{
	WSADATA wsaData;

	if( WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) != 0 )
	{
		dd_server_write_out( DDLOG_ERROR, "Failed to initialize Win32 WSADATA\n" );
		exit( 1 );
	}

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void dd_server_cleanup_win32()
{
	WSACleanup();
}

void dd_close_socket( int32_t* c_restrict socket )
{
#ifdef EV_SELECT_IS_WINSOCKET
	closesocket( _get_osfhandle(*socket) );
#else
	close( *socket );
#endif // EV_SELECT_IS_WINSOCKET
}

void dd_server_write_out( const uint32_t log_type,
                          const char* c_restrict fmt_str,
                          ... )
{
    FILE* file = stdout;
    const char* type = 0;
	uint8_t color = 0;

    switch( log_type )
    {
        case DDLOG_STATUS:
            type = "status";
            break;
        case DDLOG_ERROR:
            type = "error";
			color = CONSOLE_R;
            break;
        case DDLOG_WARNING:
            type = "warning";
			color = CONSOLE_Y;
            break;
        case DDLOG_NOTAG:
            type = " ";
        default:
            type = " ";
            break;
    }
    fprintf( file, "[%10s] ", type );
	set_output_color( color );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( file, fmt_str, args );
    va_end( args );
	
	set_output_color( 0 );
}

void dd_create_socket( struct ddAddressInfo* c_restrict address,
                       const char* c_restrict ip,
                       const char* c_restrict port,
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

		ddSocket socket_fd = 
#ifdef EV_USE_WSASOCKET
			WSASocketW( next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol, 0, 0, 0 );
#else
			socket( next_ip->ai_family, next_ip->ai_socktype, next_ip->ai_protocol);
#endif
#ifdef EV_SELECT_IS_WINSOCKET
		address->socket_fd = _open_osfhandle( socket_fd, 0 );
#else
		address->socket_fd = socket_fd;
#endif // EV_SELECT_IS_WINSOCKET

        if( socket_fd == -1 )
        {
            dd_server_write_out( DDLOG_WARNING,
                                 "Socket file descriptor creation error\n" );
            continue;
        }

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
#ifdef EV_SELECT_IS_WINSOCKET
		ddSocket socket_fd = _get_osfhandle( address->socket_fd );
#else
		ddSocket socket_fd = address->socket_fd;
#endif // EV_SELECT_IS_WINSOCKET

        int32_t yes = true;
        if( setsockopt( socket_fd,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        (const char*)&yes,
                        sizeof( int32_t ) ) == -1 )
        {
            dd_server_write_out( DDLOG_ERROR, "Socket port reuse\n" );
            return;
        }

        if( bind( socket_fd,
                  address->selected->ai_addr,
                  (int)address->selected->ai_addrlen ) == -1 )
        {
#ifdef __linux__
            close( address->socket_fd );
#elif _WIN32
			closesocket( socket_fd );
#endif
            dd_server_write_out( DDLOG_ERROR, "Socket bind\n" );
            return;
        }

        if( use_tcp )
        {
            if( listen( socket_fd, BACKLOG ) == -1 )
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

void dd_ev_io_watcher( struct ddIODataEV* c_restrict io_data )
{
    ev_io_init( io_data->io_ptr,
                io_data->cb_func,
                io_data->socketfd,
                io_data->io_flags );

    ev_io_start( io_data->loop_ptr, io_data->io_ptr );
}

void dd_ev_timer_watcher( struct ddTimerDataEV* c_restrict timer_data )
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

void dd_ev_signal_watcher( struct ddSignalDataEV* c_restrict signal_data )
{
    ev_signal_init(
        signal_data->signal_ptr, signal_data->cb_func, signal_data->signal );
    ev_signal_start( signal_data->loop_ptr, signal_data->signal_ptr );
}

void dd_server_send_msg( struct ddMsgData* c_restrict msg )
{
    //
}