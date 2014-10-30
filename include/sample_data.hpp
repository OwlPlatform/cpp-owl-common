/*
 * Copyright (c) 2012 Bernhard Firner and Rutgers University
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 * or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/*******************************************************************************
 * Common definitions for parsing sample data from an aggregator.
 ******************************************************************************/
#ifndef __SAMPLE_DATA_HPP__
#define __SAMPLE_DATA_HPP__

#include <iostream>
#include <vector>
#include <string>

//Get the time in milliseconds
uint64_t msecTime();

struct uint128_t {
  uint64_t upper;
  uint64_t lower;

  //Default constructor
  uint128_t() = default;
  //Constructor that just takes in an unsigned integer.
  uint128_t(uint64_t val);
  uint128_t(const uint128_t& val);
  template <typename T>
  uint128_t& operator=(const T& val) {
    upper = 0;
    lower = val;
    return *this;
  }
} __attribute__((packed));


//Define an equality operator for the 128 bit type
bool operator==(const uint128_t& a, const uint128_t& b);

//Define a print operator for the 128 bit type
std::ostream& operator<<(std::ostream& os, const uint128_t& val);
std::istream& operator>>(std::istream& is, uint128_t& val);

//Define a binary & operator.
uint128_t operator&(const uint128_t& a, const uint128_t& b);

//Define a less than operator
bool operator<(const uint128_t& a, const uint128_t& b);

//To string functions for the 128 bit integer type
std::string to_string(uint128_t val);
std::u16string to_u16string(uint128_t val);

typedef uint128_t TransmitterID;
typedef uint128_t ReceiverID;
typedef int64_t Timestamp;

//Sample data with details about a packet from a transmitter to a receiver.
struct SampleData {
  unsigned char physical_layer;
  TransmitterID tx_id;
  ReceiverID rx_id;
  Timestamp rx_timestamp;
  float rss;
  std::vector<unsigned char> sense_data;
  bool valid;
};

//Device position
struct DevicePosition {
  unsigned char physical_layer;
  TransmitterID device_id;
  float x;
  float y;
  float z;
  std::u16string region_uri;
  bool valid;
};

#endif


