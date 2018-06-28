#include <stdint.h>
#include <stdbool.h>

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

struct ddArgNode
{
    uint32_t type_flag;
    union {
        float f;
        int32_t i;
        const char* c;
        bool b;
    };
};

struct ddArgHandler
{
    char short_id[MAX_ARGS];
    const char* long_id[MAX_ARGS];

    const char* help_str;

    struct ddArgNode args[MAX_ARGS];

    uint32_t args_count;
};

void init_arg_handler( struct ddArgHandler* restrict handler,
                       const char* restrict help_info );

void register_arg( struct ddArgHandler* restrict handler,
                   const char* restrict full_id,
                   const char short_id,
                   const uint32_t type_flag );

const struct ddArgNode* extract_arg(
    const struct ddArgHandler* restrict handler, const char short_id );

void poll_args( struct ddArgHandler* restrict handler,
                const uint32_t argc,
                const char* const argv[restrict] );

void print_arg_help_msg( const struct ddArgHandler* restrict handler );
