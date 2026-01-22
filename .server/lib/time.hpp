#pragma once
#include <sys/time.h>
#include <sstream>
#include <string>

class time {
  public:
    static long long clock(void) throw() {
      struct timeval tv;
      gettimeofday(&tv, 0);
      return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    }
    static long long calcl(long long start, long long end) throw() {
      return end - start;
    }
    static std::string calcs(long long start, long long end) throw() {
      std::stringstream ss;
      std::string buffer;
      ss << end - start;
      ss >> buffer;
      return buffer;
    }
};