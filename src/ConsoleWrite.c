#include "ConsoleWrite.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if DD_PLATFORM == DD_WIN32
static HANDLE s_hconsole;
#else // DD_PLATFORM == DD_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static struct termios t;

static void unbuffer_stdin()
{
    tcgetattr( STDIN_FILENO, &t ); //get the current terminal I/O structure
    t.c_lflag &= ~ICANON; //Manipulate the flag bits to do what you want it to do
    tcsetattr( STDIN_FILENO, TCSANOW, &t ); //Apply the new settings
}

static void buffer_stdin()
{
    tcgetattr( STDIN_FILENO, &t ); //get the current terminal I/O structure
    t.c_lflag |= ICANON; //Manipulate the flag bits to do what you want it to do
    tcsetattr( STDIN_FILENO, TCSANOW, &t ); //Apply the new settings
}

#endif // DD_PLATFORM

static FILE* s_logfile = 0;
static bool s_log_set = false;

#define IN_BUFF_SIZE 1024
static char input_buffer[IN_BUFF_SIZE];
static char saved_input[IN_BUFF_SIZE];
static bool collect_stdin_flag = false;

static void set_output_color( uint8_t color, const bool flush )
{
#if DD_PLATFORM == DD_WIN32
    SetConsoleTextAttribute( s_hconsole, console_color[color] );
    if( flush ) FlushConsoleInputBuffer( s_hconsole );
#elif DD_PLATFORM == DD_LINUX
    fprintf( stdout, "%s", console_color[color] );
    if( flush ) fflush( stdout );
#endif  // DD_PLATFORM
}

void console_collect_stdin()
{
    if( !collect_stdin_flag )
    {
        //etbuf( stdin, input_buffer ); // turn off buffering for stdin
        fcntl( STDIN_FILENO, F_SETFL, O_NONBLOCK ); // make stdin non-blocking

#if DD_PLATFORM == DD_LINUX
        unbuffer_stdin();
#endif // DD_PLATFORM == DD_LINUX

        collect_stdin_flag = true;
    }
}

void console_set_output_log( const char* c_restrict file_location )
{
#if DD_PLATFORM == DD_WIN32
    fopen_s( &s_logfile, file_location, "w" );
#else
    s_logfile = fopen( file_location, "w" );
#endif
    if( s_logfile )
    {
        s_log_set = true;
        return;
    }

    console_write( LOG_ERROR, "Invalid log file provided. Setting to stdout" );
}

void console_write( const uint32_t log_type,
                    const char* c_restrict fmt_str,
                    ... )
{
    if( !s_log_set )
    {
#if DD_PLATFORM == DD_WIN32
        s_hconsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
        s_logfile = stdout;
        s_log_set = true;
    }

    if( collect_stdin_flag )
    {
        //fgets( input_buffer, IN_BUFF_SIZE, stdin );
        char* ch = input_buffer;
        
        while( ( *ch = getchar() ) != '\n' && *ch != EOF )
        {
            ch++;
            if( ch == (input_buffer + (IN_BUFF_SIZE - 1) ) ) break;
        }

        if( ch != input_buffer )
        {
            *ch = '\0';
            snprintf( saved_input, IN_BUFF_SIZE, "%s", input_buffer );
        }
        
        printf(" ");
    }

    set_output_color( 0, false );

    fprintf( s_logfile, "\r[%10s] ", console_header[log_type] );
    set_output_color( (uint8_t)log_type, false );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( s_logfile, fmt_str, args );
    va_end( args );

    set_output_color( 0, true );

    if( collect_stdin_flag && *saved_input )
    {
        write( STDIN_FILENO, saved_input, strlen( input_buffer ) );
        input_buffer[0] = '\0';
    }
}
