#include "ConsoleWrite.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if PLATFORM == PF_WIN32

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

static HANDLE s_hconsole_in;
static HANDLE s_hconsole_out;
static DWORD s_mode;
static bool s_handle_set = false;

static void unbuffer_stdin()
{
    if( !s_handle_set )
    {
        s_hconsole_in = GetStdHandle( STD_INPUT_HANDLE );
        s_handle_set = true;

        GetConsoleMode( s_hconsole_in, &s_mode );
    }

    SetConsoleMode( s_hconsole_out,
                    s_mode & ENABLE_PROCESSED_INPUT &
                        ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT ) );
}

static void buffer_stdin()
{
    if( !s_handle_set )
    {
        s_hconsole_in = GetStdHandle( STD_INPUT_HANDLE );
        s_handle_set = true;

        GetConsoleMode( s_hconsole_in, &s_mode );
    }

    SetConsoleMode( s_hconsole_in, s_mode );

    // restore cursor
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = TRUE;
    SetConsoleCursorInfo( s_hconsole_out, &info );
}

static int console_getchar()
{
    // On windows, you must query the console in this manner to get non-blocking
    // stdin
    INPUT_RECORD in_record;
    DWORD records_read = 0;

    GetNumberOfConsoleInputEvents( s_hconsole_in, &records_read );

    if( records_read == 0 ) return -1;

    if( ReadConsoleInput( s_hconsole_in, &in_record, 1, &records_read ) )
    {
        if( records_read > 0 )
        {
            if( in_record.EventType == KEY_EVENT &&
                in_record.Event.KeyEvent.bKeyDown )
            {
                switch( in_record.Event.KeyEvent.wVirtualKeyCode )
                {
                    case VK_BACK:
                        return 0x7f;
                    case VK_RETURN:
                        return 0xa;
                    case VK_UP:
                        return 0x41;
                    case VK_DOWN:
                        return 0x42;
                    default:
                        return in_record.Event.KeyEvent.uChar.AsciiChar;
                }
            }
        }
    }
    else
    {
        char buf[256];
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
            buf,
            ( sizeof( buf ) / sizeof( char ) ),
            NULL );

        console_write( 3, "%s", buf );
    }

    return -1;
}
#else  // PLATFORM == PF_LINUX

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static struct termios s_term;

static void unbuffer_stdin()
{
    fcntl( STDIN_FILENO, F_SETFL, O_NONBLOCK );  // make stdin non-blocking

    tcgetattr( STDIN_FILENO,
               &s_term );  // get the current terminal I/O structure
    s_term.c_lflag &= ~( ECHO | ECHOE | ICANON );  // not canonical mode
    tcsetattr( STDIN_FILENO, TCSANOW, &s_term );   // Apply the new settings
}

static void buffer_stdin()
{
    tcgetattr( STDIN_FILENO, &s_term );
    s_term.c_lflag |= ( ECHO | ECHOE | ICANON );
    tcsetattr( STDIN_FILENO, TCSANOW, &s_term );

    fputs( "\e[?25h", stdout );  // show cursor
    fflush( stdout );
}

static int console_getchar() { return getchar(); }
static struct sigaction sig_restore_term;

#endif  // PLATFORM == PF_LINUX

static FILE* s_logfile = 0;
static FILE* s_oldCmds = 0;
static bool s_log_set = false;

#define IN_BUFF_SIZE 1024
#define BUFF_HISTORY_SIZE 50

static char s_clear_buffer[IN_BUFF_SIZE];

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
#if PLATFORM == PF_WIN32
    SetConsoleTextAttribute( s_hconsole_out, console_color[color] );
    if( flush ) FlushConsoleInputBuffer( s_hconsole_out );
#elif PLATFORM == PF_LINUX
    fprintf( stdout, "%s", console_color[color] );
    if( flush ) fflush( stdout );
#endif  // PLATFORM == PF_LINUX
}

void console_restore_stdin() { buffer_stdin(); }
//

void console_collect_stdin()
{
    if( !s_collect_stdin_flag )
    {
#if PLATFORM == PF_LINUX
        sig_restore_term.sa_handler = sig_handler;
        sigaction( SIGINT, &sig_restore_term, NULL );
#else  // PLATFORM == PF_WIN32
        signal( SIGINT, sig_handler );
#endif

        unbuffer_stdin();

        for( uint32_t i = 0; i < IN_BUFF_SIZE; i++ ) s_clear_buffer[i] = ' ';
        s_clear_buffer[IN_BUFF_SIZE - 1] = '\0';

        s_buffered_str = s_buffered_history;

        s_oldCmds = fopen( "server_input.log", "a+" );

        if( s_oldCmds )
        {
            while( fgets( s_buffered_str, IN_BUFF_SIZE, s_oldCmds ) )
            {
                const size_t cmd_length = strlen( s_buffered_str );

                if( cmd_length <= 1 ) continue;

                s_buffered_str[cmd_length - 1] = '\0';  // strip newline

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
    bool update_line = false;
    uint32_t clear_line = 0;

    while( ( ch = console_getchar() ) != EOF )
    {
        update_line = true;

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
                clear_line = s_buffered_str_len;

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

            clear_line = s_buffered_str_len;

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
                clear_line = s_buffered_str_len;

                const size_t curr_spot =
                    ( s_buffered_str - s_buffered_history ) / IN_BUFF_SIZE;

                const uint32_t next_spot =
                    ( curr_spot > 0 ) ? curr_spot - 1 : BUFF_HISTORY_SIZE - 1;

                s_buffered_str =
                    ( s_buffered_history + next_spot * IN_BUFF_SIZE );
                s_buffered_str_len = (uint32_t)strlen( s_buffered_str );
            }
        }
        // handle down key
        else if( ch == 0x42 )
        {
            if( s_buffered_str !=
                ( s_buffered_history + s_history_tail * IN_BUFF_SIZE ) )
            {
                clear_line = s_buffered_str_len;

                const size_t curr_spot =
                    ( s_buffered_str - s_buffered_history ) / IN_BUFF_SIZE;

                const uint32_t next_spot =
                    ( curr_spot + 1 ) % BUFF_HISTORY_SIZE;

                s_buffered_str =
                    ( s_buffered_history + next_spot * IN_BUFF_SIZE );
                s_buffered_str_len = (uint32_t)strlen( s_buffered_str );
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
                s_buffered_str[s_buffered_str_len] = (char)ch;
                s_buffered_str_len++;
                s_buffered_str[s_buffered_str_len] = '\0';
            }
        }
        // debug line to figure out hex value of char && history
        /*
                console_write( 4, "HEAD: %u, TAIL: %u, VALUE: %#x",
        s_history_head,
                        s_history_tail,
                        ch);
        //*/
    }

    if( update_line )
    {
        const int32_t chars_to_clear = clear_line - s_buffered_str_len;

        set_output_color( 4, false );
        fputs( "\rlocal_machine", stdout );

        set_output_color( 0, true );
        fprintf( stdout, ":$ %s", s_buffered_str );

        if( chars_to_clear > 0 )
        {
            for( int32_t i = 0; i < chars_to_clear; i++ ) fputc( ' ', stdout );
        }
#if PLATFORM == PF_LINUX
        fputs( "\e[?25l", stdout );  // hide cursor
#endif
    }
}

void console_set_output_log( const char* c_restrict file_location )
{
#if PLATFORM == PF_WIN32
    fopen_s( &s_logfile, file_location, "a+" );
#else
    s_logfile = fopen( file_location, "a+" );
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
#if PLATFORM == PF_WIN32
        s_hconsole_out = GetStdHandle( STD_OUTPUT_HANDLE );

        // hide cursor
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo( s_hconsole_out, &info );
#endif
        s_logfile = stdout;
        s_log_set = true;

        if( !s_buffered_str ) s_buffered_str = s_buffered_history;
    }

    set_output_color( 0, false );

    fprintf( s_logfile, "\r[%10s] ", console_header[log_type] );
    set_output_color( (uint8_t)log_type, false );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( s_logfile, fmt_str, args );
    va_end( args );

    // clear overlapping string if present
    if( s_buffered_str_len > 0 )
        fprintf( stdout,
                 "%s",
                 s_clear_buffer + ( IN_BUFF_SIZE - s_buffered_str_len ) );

    set_output_color( 4, false );
    fputs( "\nlocal_machine", stdout );
    set_output_color( 0, true );
    fprintf( stdout, ":$ %s", s_buffered_str );

#if PLATFORM == PF_LINUX
    fputs( "\e[?25l", stdout );  // hide cursor
#endif
}

void query_input( char* copy_to_buffer, const uint32_t copy_to_buffer_size )
{
    assert( copy_to_buffer && copy_to_buffer_size > 1 );

    snprintf( copy_to_buffer, copy_to_buffer_size, "%s", s_final_buff );

    s_final_buff[0] = '\0';
}
