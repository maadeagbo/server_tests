#include "ArgHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct ddArgStat h_arg;

void init_arg_handler( struct ddArgHandler* c_restrict handler,
                       const char* c_restrict help_info )
{
    if( !handler ) return;

    *handler = ( struct ddArgHandler ){.args_count = 0};

    for( uint32_t i = 0; i < MAX_ARGS; i++ )
        handler->args[i] = ( struct ddArgNode ){0};

    h_arg = ( struct ddArgStat ){.description = help_info,  //
                                 .full_id = "help",         //
                                 .type_flag = ARG_BOOL,
                                 .short_id = 'h'};

    register_arg( handler, &h_arg );
}

static void set_arg( struct ddArgNode* c_restrict arg,
                     const char* c_restrict value )
{
    if( !arg ) return;

    switch( arg->type_flag )
    {
        case ARG_BOOL:
            arg->val.b = true;
            break;
        case ARG_FLT:
            arg->val.f = strtof( value, NULL );
            break;
        case ARG_INT:
            arg->val.i = strtol( value, NULL, 10 );
            break;
        case ARG_STR:
            arg->val.c = value;
            break;
        default:
            break;
    }
}

void poll_args( struct ddArgHandler* c_restrict handler,
                const uint32_t argc,
                const char* const argv[] )
{
    if( !handler || handler->args_count == 0 ) return;

    struct ddArgNode* curr_arg = NULL;

    for( uint32_t i = 0; i < argc; i++ )
    {
        const char* next_arg = argv[i];
        if( *next_arg == '-' )
        {
            curr_arg = NULL;  // used if current argument has modifier/input
            next_arg++;

            if( *next_arg == '-' )  // long-form argument
            {
                next_arg++;
                for( uint32_t j = 0; j < handler->args_count; j++ )
                    if( strcmp( next_arg, handler->long_id[j] ) == 0 )
                    {
                        curr_arg = &handler->args[j];
                        set_arg( curr_arg, "1" );  // defaults boolean to true
                        break;
                    }
            }
            else  // short-form argument
                while( *next_arg )
                {
                    for( uint32_t j = 0; j < handler->args_count; j++ )
                        if( *next_arg == handler->short_id[j] )
                        {
                            curr_arg = &handler->args[j];
                            set_arg( curr_arg, "1" );  // sets boolean to true
                        }

                    next_arg++;
                }
        }
        else
            set_arg( curr_arg, next_arg );
    }
}

void register_arg( struct ddArgHandler* c_restrict handler,
                   const struct ddArgStat* c_restrict new_arg )
{
    if( !handler || !new_arg->full_id || handler->args_count == MAX_ARGS )
        return;

    handler->long_id[handler->args_count] = new_arg->full_id;
    handler->short_id[handler->args_count] = new_arg->short_id;
    handler->args[handler->args_count].type_flag = new_arg->type_flag;
    handler->args[handler->args_count].description = new_arg->description;
    handler->args[handler->args_count].val = new_arg->default_val;

    handler->args_count++;
}

const struct ddArgNode* extract_arg(
    const struct ddArgHandler* c_restrict handler, const char short_id )
{
    if( !handler ) return NULL;

    for( uint32_t i = 0; i < handler->args_count; i++ )
        if( handler->short_id[i] == short_id ) return &handler->args[i];

    return NULL;
}

static const char* print_type( const struct ddArgNode* c_restrict arg )
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

void print_arg_help_msg( const struct ddArgHandler* c_restrict handler )
{
    if( !handler ) return;

    // help description always 1st
    printf( "%s\n", handler->args[0].description );

    for( uint32_t i = 0; i < handler->args_count; i++ )
    {
        printf( "  -%-4c --%-10s", handler->short_id[i], handler->long_id[i] );
        printf( ">%-10s", print_type( &handler->args[i] ) );

        if( handler->short_id[i] != 'h' )
            printf( "%s", handler->args[i].description );

        printf( "\n" );
    }
}