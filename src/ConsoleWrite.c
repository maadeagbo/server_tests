#include "ConsoleWrite.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

static FILE* s_logfile = 0;
static bool s_log_set = false;

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

void console_set_output_log( const char* c_restrict file_location )
{
    s_logfile = fopen( file_location, "w" );
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
        s_logfile = stdout;
        s_log_set = true;
    }

    set_output_color( 0, false );

    fprintf( s_logfile, "[%10s] ", console_header[log_type] );
    set_output_color( log_type, false );

    va_list args;

    va_start( args, fmt_str );
    vfprintf( s_logfile, fmt_str, args );
    va_end( args );

    set_output_color( 0, true );
}
