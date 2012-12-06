/*******************************************************************************
 * Common definitions for parsing sample data.
 ******************************************************************************/

#include "sys/time.h"
#include "sample_data.hpp"

//Get the time in milliseconds
uint64_t msecTime() {
  //Set this to the real timestamp, milliseconds since 1970
  timeval tval;
  gettimeofday(&tval, NULL);
  //Cast to uint64_t to prevent rounding problems on 32 bit systems
  return (uint64_t)tval.tv_sec*(uint64_t)1000 + tval.tv_usec/1000;
}

bool operator==(const uint128_t& a, const uint128_t& b) {
  return a.upper == b.upper and a.lower == b.lower;
}

std::ostream& operator<<(std::ostream& os, const uint128_t& val) {
  //The packed struct seems to have problems with the stream operators
  //so pull out the upper and lower values into temporary variables before
  //applying the << operator.
  uint64_t upper = val.upper;
  uint64_t lower = val.lower;
  //Save current format flags
  std::ios_base::fmtflags flags = os.flags();
  return os<<"0x"<<std::hex<<upper<<lower<<std::dec;
  //Reset format flags
  os.flags(flags);
  //TODO FIXME This assumes that the number is actually only 64 bits long.
  //return os<<lower<<std::dec;
}

//TODO FIXME This assumes that the input number is actually only 64 bits long.
std::istream& operator>>(std::istream& is, uint128_t& val) {
  //The packed nature of the structure seems to cause errors with the
  //stream operator so make a temporary variable to read data into.
  uint64_t newval;
  is >> newval;
  val.lower = newval;
  return is;
  //return is>>val.lower;
}

uint128_t operator&(const uint128_t& a, const uint128_t& b) {
  uint128_t c;
  c.upper = a.upper & b.upper;
  c.lower = a.lower & b.lower;
  return c;
}

bool operator<(const uint128_t& a, const uint128_t& b) {
  return (a.upper < b.upper or (a.upper == b.upper and a.lower < b.lower));
}

uint128_t::uint128_t(const uint128_t& val) {
  this->upper = val.upper;
  this->lower = val.lower;
}

uint128_t::uint128_t(uint64_t val) {
  this->upper = 0;
  this->lower = val;
}

std::string to_string(uint128_t val) {
  return std::to_string(val.lower);
}

std::u16string to_u16string(uint128_t val) {
  std::string str = to_string(val);
  return std::u16string(str.begin(), str.end());
}

