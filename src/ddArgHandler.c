#include "ddArgHandler.h"
#include <stdlib.h>
#include <stdio.h>

void init_arg_handler( struct ddArgHandler* restrict handler,
                       const char* restrict help_info )
{
    if( !handler ) return;

    *handler = ( struct ddArgHandler ){.args_count = 0};

    handler->help_str = help_info;

    for( uint32_t i = 0; i < MAX_ARGS; i++ )
        handler->args[i] = ( struct ddArgNode ){0};

    register_arg( handler, "help", 'h', ARG_STR );
}

static void set_arg( struct ddArgNode* restrict arg,
                     const char* restrict value )
{
    if( !arg ) return;

    switch( arg->type_flag )
    {
        case ARG_BOOL:
            arg->b = *value ? true : false;
            break;
        case ARG_FLT:
            arg->f = strtof( value, NULL );
            break;
        case ARG_INT:
            arg->i = strtod( value, NULL );
            break;
        case ARG_STR:
            arg->c = value;
            break;
        default:
            break;
    }
}

void poll_args( struct ddArgHandler* restrict handler,
                const uint32_t argc,
                const char* const argv[restrict] )
{
    if( !handler || handler->args_count == 0 ) return;

    struct ddArgNode* curr_arg = NULL;

    for( uint32_t i = 0; i < argc; i++ )
    {
        const char* next_arg = argv[i];
        if( *next_arg == '-' )
        {
            curr_arg = NULL;
            next_arg++;

            while( *next_arg )
            {
                for( uint32_t j = 0; j < handler->args_count; j++ )
                    if( *next_arg == handler->short_id[j] )
                    {
                        curr_arg = &handler->args[j];
                        set_arg( curr_arg, "1" );
                    }

                next_arg++;
            }
        }
        else
            set_arg( curr_arg, next_arg );
    }
}

void register_arg( struct ddArgHandler* restrict handler,
                   const char* restrict full_id,
                   const char short_id,
                   const uint32_t type_flag )
{
    if( !handler || !full_id || handler->args_count == MAX_ARGS ) return;

    handler->long_id[handler->args_count] = full_id;
    handler->short_id[handler->args_count] = short_id;
    handler->args[handler->args_count].type_flag = type_flag;

    handler->args_count++;
}

const struct ddArgNode* extract_arg(
    const struct ddArgHandler* restrict handler, const char short_id )
{
    if( !handler ) return NULL;

    for( uint32_t i = 0; i < handler->args_count; i++ )
        if( handler->short_id[i] == short_id ) return &handler->args[i];

    return NULL;
}

static const char* print_type( const struct ddArgNode* restrict arg )
{
    if( !arg ) return "";

    switch( arg->type_flag )
    {
        case ARG_BOOL:
            return "bool";
        case ARG_FLT:
            return "float";
        case ARG_INT:
            return "int";
        case ARG_STR:
            return "string";
        default:
            return "unassigned";
    }
}

void print_arg_help_msg( const struct ddArgHandler* restrict handler )
{
    if( !handler ) return;

    if( handler->help_str ) printf( "%s\n", handler->help_str );

    for( uint32_t i = 0; i < handler->args_count; i++ )
    {
        printf( "  -%c\t %s\t", handler->short_id[i], handler->long_id[i] );
        printf( "(%s)", print_type( &handler->args[i] ) );
        printf( "\n" );
    }
}