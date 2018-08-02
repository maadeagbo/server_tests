#include "ConsoleWrite.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if DD_PLATFORM == DD_WIN32
static HANDLE s_hconsole;
#else  // DD_PLATFORM == DD_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static struct termios s_term;

static void unbuffer_stdin()
{
    tcgetattr( STDIN_FILENO,
               &s_term );  // get the current terminal I/O structure
    s_term.c_lflag &= ~( ECHO | ECHOE | ICANON );  // not canonical mode
    tcsetattr( STDIN_FILENO, TCSANOW, &s_term );   // Apply the new settings
}

static void buffer_stdin()
{
    tcgetattr( STDIN_FILENO,
               &s_term );  // get the current terminal I/O structure
    s_term.c_lflag |= ( ECHO | ECHOE | ICANON );  // canonical
    tcsetattr( STDIN_FILENO, TCSANOW, &s_term );  // Apply the new settings
}

#endif  // DD_PLATFORM

static FILE* s_logfile = 0;
static bool s_log_set = false;

#define IN_BUFF_SIZE 1024
static char s_input_buff[IN_BUFF_SIZE];

static char s_saved_buff[IN_BUFF_SIZE];
static uint32_t s_saved_size;

static char s_final_buff[IN_BUFF_SIZE];
static bool s_collect_stdin_flag = false;

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
    if( !s_collect_stdin_flag )
    {
        // etbuf( stdin, s_input_buff ); // turn off buffering for stdin
        fcntl( STDIN_FILENO, F_SETFL, O_NONBLOCK );  // make stdin non-blocking

#if DD_PLATFORM == DD_LINUX
        unbuffer_stdin();
#endif  // DD_PLATFORM == DD_LINUX

        s_collect_stdin_flag = true;
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

    if( s_collect_stdin_flag )
    {
        // fgets( s_input_buff, IN_BUFF_SIZE, stdin );
        char ch;
        int32_t buff_size = 0;

        while( ( ch = getchar() ) != '\n' && ch != EOF )
        {
            if( ch == 0x7f )
            {
                buff_size--;
            }
            else
            {
                if( buff_size < 0 ) buff_size = 0;

                // write( STDIN_FILENO, &ch, 1 );

                s_input_buff[buff_size] = ch;
                buff_size++;
                if( buff_size >= ( IN_BUFF_SIZE - 1 ) ) break;
            }
        }

        if( buff_size > 0 )
        {
            s_input_buff[buff_size] = '\0';

            const uint32_t curr_offset = (uint32_t)strlen( s_saved_buff );
            const uint32_t new_buffer_length = curr_offset + buff_size;

            if( new_buffer_length < IN_BUFF_SIZE )
            {
                snprintf( s_saved_buff + curr_offset,
                          IN_BUFF_SIZE - curr_offset,
                          "%s",
                          s_input_buff );
            }
        }
        else if( buff_size < 0 )
        {
            const uint32_t curr_offset = strlen( s_saved_buff );
            const int32_t delete_idx = curr_offset + buff_size;

            if( delete_idx <= 0 )
                s_saved_buff[0] = '\0';
            else
                s_saved_buff[delete_idx] = '\0';
        }

        if( ch == '\n' )
        {
            snprintf( s_final_buff, IN_BUFF_SIZE, "%s", s_saved_buff );

            s_saved_buff[0] = '\0';
        }
    }

    set_output_color( 0, false );

    fprintf( s_logfile, "\r[%10s] ", console_header[log_type] );
    set_output_color( (uint8_t)log_type, false );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( s_logfile, fmt_str, args );
    va_end( args );

    set_output_color( 0, true );

    if( s_collect_stdin_flag && *s_saved_buff )
    {
        write( STDIN_FILENO, s_saved_buff, strlen( s_saved_buff ) );
        // fputs( s_saved_buff, std );
        s_input_buff[0] = '\0';
    }
}
