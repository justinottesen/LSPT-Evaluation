#include "TimeUtil.h"

#include <bits/types/struct_timeval.h>
#include <sys/time.h>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

std::string current_time(bool path) {
  timeval time_value{};
  gettimeofday(&time_value, nullptr);
  tm time_struct{};
  localtime_r(&time_value.tv_sec, &time_struct);
  std::ostringstream output_stream;
  output_stream << std::put_time(&time_struct, "%F %T") << "." << std::setw(3) << std::setfill('0')
                << time_value.tv_usec / 1000;
  std::string date_str = output_stream.str();
  if (path) {
    std::ranges::replace(date_str, ' ', '_');
    std::ranges::replace(date_str, ':', '-');
  }
  return date_str;
}

std::string program_time(bool path) {
  static const std::string time_str = current_time();
  if (path) {
    std::string copy = time_str;
    std::ranges::replace(copy, ' ', '_');
    std::ranges::replace(copy, ':', '-');
    return copy;
  }
  return time_str;
}
