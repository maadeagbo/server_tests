#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "Config.h"

#ifndef MAX_ARGS
#define MAX_ARGS 10
#endif

#ifndef ENUM_VAL
#define ENUM_VAL( x ) 1 << x
#endif  // !ENUM_VAL

enum
{
    ARG_BOOL = ENUM_VAL( 0 ),
    ARG_FLT = ENUM_VAL( 1 ),
    ARG_INT = ENUM_VAL( 2 ),
    ARG_STR = ENUM_VAL( 3 )
};

#undef ENUM_VAL

struct ddArgVal
{
    union {
        float f;
        int32_t i;
        const char* c;
        bool b;
    };
};

struct ddArgNode
{
    uint32_t type_flag;
    const char* description;
    struct ddArgVal val;
};

struct ddArgHandler
{
    char short_id[MAX_ARGS];
    const char* long_id[MAX_ARGS];

    struct ddArgNode args[MAX_ARGS];

    uint32_t args_count;
};

struct ddArgStat
{
    const char* description;
    const char* full_id;
    uint32_t type_flag;
    char short_id;
    struct ddArgVal default_val;
};

void init_arg_handler( struct ddArgHandler* c_restrict handler,
                       const char* c_restrict help_info );

void register_arg( struct ddArgHandler* c_restrict handler,
                   const struct ddArgStat* c_restrict new_arg );

const struct ddArgNode* extract_arg(
    const struct ddArgHandler* c_restrict handler, const char short_id );

void poll_args( struct ddArgHandler* c_restrict handler,
                const uint32_t argc,
                const char* const argv[] );

void print_arg_help_msg( const struct ddArgHandler* c_restrict handler );
