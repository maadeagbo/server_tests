#pragma once

#include <stdint.h>

#include "ddConfig.h"

/* Interface for retrieving time information w/ nano-second granularity */

uint64_t get_high_res_time();
uint64_t seconds_to_nano( double seconds );
uint64_t nano_to_milli( uint64_t nanosecs );
double nano_to_seconds( uint64_t nanosecs );
