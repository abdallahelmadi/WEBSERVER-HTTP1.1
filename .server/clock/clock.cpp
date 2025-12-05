#include <clock.hpp>

clock_tt startClock(void) throw() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

clock_tt endClock(void) throw() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

clock_tt calculateTime(clock_tt start, clock_tt end) throw() {
  return end - start;
}