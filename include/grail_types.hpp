/*******************************************************************************
 * This file defines types that exist in the world model.
 * Endian differences stop packed structs from being read in directly from a
 * raw byte buffer to the struct.
 ******************************************************************************/

#ifndef __GRAIL_TYPES_HPP__
#define __GRAIL_TYPES_HPP__

#include "sample_data.hpp"
#include "netbuffer.hpp"
#include <functional>
#include <iostream>
#include <vector>

namespace grail_types {

  //A transmitter entry holds a physical layer and id.
  struct transmitter {
    uint8_t phy;
    uint128_t id;
  } __attribute__((packed));

  bool operator<(const transmitter& a, const transmitter& b);
  bool operator==(const transmitter& a, const transmitter& b);

  std::ostream& operator<<(std::ostream& out, const transmitter& t);

  transmitter readTransmitterFromBuffer(BuffReader& reader);
  transmitter readTransmitter(const std::vector<unsigned char>& buff);
  //Write the transmitter to a buffer and return the bytes written
  uint32_t writeTransmitter(transmitter t, std::vector<unsigned char>& buff);

  template<typename T>
    std::vector<T> unpackGRAILVector(std::function<T (BuffReader&)> read_fun, std::vector<unsigned char>& buff) {
      BuffReader reader(buff);
      std::vector<T> results;
      uint32_t total = reader.readPrimitive<uint32_t>();
      for (;0 < total; --total) {
        results.push_back(read_fun(reader));
      }
      return results;
    }

};

#endif

