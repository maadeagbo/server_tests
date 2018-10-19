#include "TimeInterface.h"

#include <assert.h>

#if PLATFORM == PF_LINUX
#include <time.h>
#include <unistd.h>
//#include <x86intrin.h>

/*
static uint64_t RDTSC()
{
    unsigned int hi, lo;
    // ask the compiler to read the assembler code
    // assign the register holding the lower part of the 64-bit number to low
    // and vice-versa to high
    __asm__ __volatile__( "rdtsc" : "=a"( lo ), "=d"( hi ) );
    return ( (uint64_t)hi << 32 ) | lo;
}

// Calculate cycles per nanosecond
static double SetCyclesPerNanoSec()
{
    // pure c-struct need keyword
    struct timespec beginTS, endTS, diffTS;
    uint64_t _begin = 0, _end = 0;

    clock_gettime( CLOCK_MONOTONIC_RAW, &beginTS );

    _begin = RDTSC();
    for( uint64_t i = 0; i < 1000000; i++ )
        ;
    _end = RDTSC();

    clock_gettime( CLOCK_MONOTONIC_RAW, &endTS );

    // calculate difference in time for "for loop" to run
    diffTS.tv_sec = endTS.tv_sec - beginTS.tv_sec;
    diffTS.tv_nsec = endTS.tv_nsec - beginTS.tv_nsec;

    const uint64_t elaspedNanoSecs =
        diffTS.tv_sec * 1000000000LL + diffTS.tv_nsec;

    return (double)( _end - _begin ) / (double)elaspedNanoSecs;
}

static uint64_t rdtsc_Start()
{
    unsigned int cycles_high, cycles_low;
    __asm__ __volatile__(
        "CPUID\n\t"
        "RDTSC\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        : "=r"( cycles_high ),
          "=r"( cycles_low )::"%rax",
          "%rbx",
          "%rcx",
          "%rdx" );
    return ( (uint64_t)cycles_high << 32 ) | cycles_low;
}

static uint64_t rdtsc_End()
{
    unsigned int cycles_high, cycles_low;
    __asm__ __volatile__(
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "CPUID\n\t"
        : "=r"( cycles_high ),
          "=r"( cycles_low )::"%rax",
          "%rbx",
          "%rcx",
          "%rdx" );
    return ( (uint64_t)cycles_high << 32 ) | cycles_low;
}

*/

uint64_t get_high_res_time()
{
    uint64_t hrTime = 0;
    struct timespec now;

    clock_gettime( CLOCK_MONOTONIC_RAW, &now );
    hrTime = ( now.tv_sec * 1000000000L ) + now.tv_nsec;

    return hrTime;
}
#elif PLATFORM == PF_WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdbool.h>

// return cpu cycles
uint64_t get_high_res_time()
{
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter( &now );
    QueryPerformanceFrequency( &freq );

    return ( (uint64_t)now.QuadPart * 1000000000LL ) / (uint64_t)freq.QuadPart;
}

#endif  // PLATFORM

uint64_t seconds_to_nano( double seconds )
{
    assert( seconds >= 0.0f );

    double nanosecs = seconds * 1000000000.0;
    return (uint64_t)nanosecs;
}

// ( WARNING: can possibly overflow ? )
double nano_to_seconds( uint64_t nanosecs )
{
    uint64_t tempMicro = nanosecs / 1000LL;
    return (double)tempMicro / 1000000.0;
}

uint64_t nano_to_milli( uint64_t nanosecs ) { return nanosecs / 1000000LL; }
