#pragma once
#include <sys/time.h>

typedef long long clock_tt;

clock_tt startClock(void) throw();
clock_tt endClock(void) throw();
clock_tt calculateTime(clock_tt start, clock_tt end) throw();