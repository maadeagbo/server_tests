#pragma once

#include <stdint.h>
#include "ddConfig.h"

#if DD_PLATFORM == DD_WIN32

const WORD console_color[] = 
{
#define CONSOLE_ENUM( a, b, c, d ) d,
#include "ConsoleEnums.inl"
};

#elif DD_PLATFORM == DD_LINUX

const char* const console_color[] = 
{
#define CONSOLE_ENUM( a, b, c, d ) c,
#include "ConsoleEnums.inl"
};

#endif

enum
{
#define CONSOLE_ENUM( a, b, c, d ) a,
#include "ConsoleEnums.inl"
};

const char* const console_header[] = 
{
#define CONSOLE_ENUM( a, b, c, d ) a,
#include "ConsoleEnums.inl"
};

void console_set_output_log( const char* c_restrict file_location );

void console_write( const uint32_t log_type,
                    const char* c_restrict fmt_str,
                    ... );