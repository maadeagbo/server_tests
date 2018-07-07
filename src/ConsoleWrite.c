#include "ConsoleWrite.h"

#include <stdlib.h>

void console_write( const uint32_t log_type,
                    const char* c_restrict fmt_str,
                    ... )
{
    FILE* file = stdout;
    const char* type = 0;

    set_output_color( 0 );

    switch( log_type )
    {
        case LOG_STATUS:
            type = console_header[log_type];
            break;
        case LOG_ERROR:
            type = console_header[log_type];
            break;
        case LOG_WARN:
            type = console_header[log_type];
            break;
        default:
            type =  console_header[LOG_NOTAG];
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
