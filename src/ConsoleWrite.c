#include "ConsoleWrite.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if DD_PLATFORM == DD_WIN32
static HANDLE s_hconsole;

static void unbuffer_stdin() {}
static void buffer_stdin() {}
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

#endif  // DD_PLATFORM == DD_LINUX

static struct sigaction sig_restore_term;

static FILE* s_logfile = 0;
static FILE* s_oldCmds = 0;
static bool s_log_set = false;

#define IN_BUFF_SIZE 1024
#define BUFF_HISTORY_SIZE 50

static char s_buffered_history[IN_BUFF_SIZE * BUFF_HISTORY_SIZE];
static uint32_t s_history_head;
static uint32_t s_history_tail;

static char* s_buffered_str;
static uint32_t s_buffered_str_len;

static char s_final_buff[IN_BUFF_SIZE];
static bool s_collect_stdin_flag = false;

static void sig_handler( int sig )
{
    buffer_stdin();
    fclose( s_oldCmds );
}

static void set_output_color( uint8_t color, const bool flush )
{
#if DD_PLATFORM == DD_WIN32
    SetConsoleTextAttribute( s_hconsole, console_color[color] );
    if( flush ) FlushConsoleInputBuffer( s_hconsole );
#elif DD_PLATFORM == DD_LINUX
    fprintf( stdout, "%s", console_color[color] );
    if( flush ) fflush( stdout );
#endif  // DD_PLATFORM == DD_LINUX
}

void console_restore_stdin() { buffer_stdin(); }
//
static void clear_screen_echo()
{
    fputc( '\r', stdout );
    // include clearing the header-> local_machine:$
    for( uint32_t i = 0; i < s_buffered_str_len + 16; i++ )
        fputc( ' ', stdout );
}

void console_collect_stdin()
{
    if( !s_collect_stdin_flag )
    {
        sig_restore_term.sa_handler = sig_handler;
        sigaction( SIGINT, &sig_restore_term, NULL );

        fcntl( STDIN_FILENO, F_SETFL, O_NONBLOCK );  // make stdin non-blocking
        unbuffer_stdin();

        s_buffered_str = s_buffered_history;

        s_oldCmds = fopen( "server_input.log", "a+" );

        if( s_oldCmds )
        {
            while( fgets( s_buffered_str, IN_BUFF_SIZE, s_oldCmds ) )
            {
                const size_t cmd_length = strlen( s_buffered_str );

                if( cmd_length <= 1 ) continue;

                s_buffered_str[cmd_length - 1] = '\0';  // erase newline

                // update history
                s_history_tail = ( s_history_tail + 1 ) % BUFF_HISTORY_SIZE;

                s_buffered_str =
                    s_buffered_history + ( s_history_tail * IN_BUFF_SIZE );
                s_buffered_str_len = 0;
                s_buffered_str[0] = '\0';

                if( s_history_head == s_history_tail )
                    s_history_head = ( s_history_head + 1 ) % BUFF_HISTORY_SIZE;
            }
        }

        s_collect_stdin_flag = true;
    }

    int ch;

    while( ( ch = getchar() ) != EOF )
    {
        // handle backspace key
        if( ch == 0x7f )
        {
            // update history if necessary
            char* curr_end =
                ( s_buffered_history + s_history_tail * IN_BUFF_SIZE );
            if( s_buffered_str != curr_end )
            {
                snprintf( curr_end, IN_BUFF_SIZE, "%s", s_buffered_str );
                s_buffered_str = curr_end;
            }

            if( s_buffered_str_len > 0 )
            {
                clear_screen_echo();

                s_buffered_str_len--;
                s_buffered_str[s_buffered_str_len] = '\0';
            }
        }
        // handle enter key
        else if( ch == 0xa )
        {
            // update history if necessary
            char* curr_end =
                ( s_buffered_history + s_history_tail * IN_BUFF_SIZE );
            if( s_buffered_str != curr_end )
            {
                snprintf( curr_end, IN_BUFF_SIZE, "%s", s_buffered_str );
                s_buffered_str = curr_end;
            }

            clear_screen_echo();

            snprintf( s_final_buff, IN_BUFF_SIZE, "%s", s_buffered_str );

            // update history
            s_history_tail = ( s_history_tail + 1 ) % BUFF_HISTORY_SIZE;

            s_buffered_str =
                s_buffered_history + ( s_history_tail * IN_BUFF_SIZE );
            s_buffered_str_len = 0;
            s_buffered_str[0] = '\0';

            if( s_history_head == s_history_tail )
                s_history_head = ( s_history_head + 1 ) % BUFF_HISTORY_SIZE;

            // update input log if possible
            if( s_oldCmds ) fprintf( s_oldCmds, "%s\n", s_final_buff );
        }
        // handle up key
        else if( ch == 0x41 )
        {
            if( s_buffered_str !=
                ( s_buffered_history + s_history_head * IN_BUFF_SIZE ) )
            {
                clear_screen_echo();

                const size_t curr_spot =
                    ( s_buffered_str - s_buffered_history ) / IN_BUFF_SIZE;

                const uint32_t next_spot =
                    ( curr_spot > 0 ) ? curr_spot - 1 : BUFF_HISTORY_SIZE - 1;

                s_buffered_str =
                    ( s_buffered_history + next_spot * IN_BUFF_SIZE );
                s_buffered_str_len = strlen( s_buffered_str );
            }
        }
        // handle down key
        else if( ch == 0x42 )
        {
            if( s_buffered_str !=
                ( s_buffered_history + s_history_tail * IN_BUFF_SIZE ) )
            {
                clear_screen_echo();

                const size_t curr_spot =
                    ( s_buffered_str - s_buffered_history ) / IN_BUFF_SIZE;

                const uint32_t next_spot =
                    ( curr_spot + 1 ) % BUFF_HISTORY_SIZE;

                s_buffered_str =
                    ( s_buffered_history + next_spot * IN_BUFF_SIZE );
                s_buffered_str_len = strlen( s_buffered_str );
            }
        }
        else if( ( ch >= ' ' && ch <= 'Z' ) || ( ch >= 'a' && ch <= '~' ) )
        {
            // update history if necessary
            char* curr_end =
                ( s_buffered_history + s_history_tail * IN_BUFF_SIZE );
            if( s_buffered_str != curr_end )
            {
                snprintf( curr_end, IN_BUFF_SIZE, "%s", s_buffered_str );
                s_buffered_str = curr_end;
            }

            // all other input
            if( s_buffered_str_len < ( IN_BUFF_SIZE - 2 ) )
            {
                s_buffered_str[s_buffered_str_len] = ch;
                s_buffered_str_len++;
                s_buffered_str[s_buffered_str_len] = '\0';
            }
        }
        // debug line to figure out hex value of char && history
        /*
        fprintf( stdout,
                 "\rHEAD: %u, TAIL: %u, VALUE: %#x",
                 s_history_head,
                 s_history_tail,
                 ch );
        */
    }

    set_output_color( 4, false );
    fputs( "\rlocal_machine", stdout );

    set_output_color( 0, false );
    fprintf( stdout, ":$ %s", s_buffered_str );
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

    set_output_color( 0, false );

    fprintf( s_logfile, "\r[%10s] ", console_header[log_type] );
    set_output_color( (uint8_t)log_type, false );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( s_logfile, fmt_str, args );
    va_end( args );

    set_output_color( 0, true );
}

void query_input( char* copy_to_buffer, const uint32_t copy_to_buffer_size )
{
    assert( copy_to_buffer && copy_to_buffer_size > 1 );

    snprintf( copy_to_buffer, copy_to_buffer_size, "%s", s_final_buff );

    s_final_buff[0] = '\0';
}