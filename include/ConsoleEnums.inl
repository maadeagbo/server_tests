#ifndef CONSOLE_ENUM
#define CONSOLE_ENUM( a, b, c, d )
#endif

CONSOLE_ENUM( LOG_NOTAG, " ", "\033[0m", FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY ) // white
CONSOLE_ENUM( LOG_STATUS, "status", "\033[0m", FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY ) // white
CONSOLE_ENUM( LOG_ERROR, "error", "\033[31;1;1m", FOREGROUND_RED | FOREGROUND_INTENSITY ) // red
CONSOLE_ENUM( LOG_WARN, "warning", "\033[33;1;1m", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY ) // yellow

#undef CONSOLE_ENUM