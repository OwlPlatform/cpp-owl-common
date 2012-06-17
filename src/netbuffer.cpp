/*
 * netbuffer.{cpp,hpp} - transform data to and from network format
 * Source version: October 11, 2010
 * Copyright (c) 2010 Bernhard Firner
 * All rights reserved.
 *
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
 * This module simplifies sending and reading
 * data over a big endian messaging system (like the internet)
 * It reads data from a character buffer or encodes it into the buffer.
 ******************************************************************************/

#include "netbuffer.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <arpa/inet.h>

//endian.h contains non-standard endian conversions for
//64 bit numbers. These can be used to define htonll
//and htonll functions.
#include <endian.h>

bool littleEndian() {
  uint32_t endian_test = 1;
  return ( *(unsigned char*)(&endian_test) == 1 );
}

#ifndef be64toh
//be64toh is not defined on some systems
uint64_t be64toh(uint64_t val) {
  //If the host is big endian just return.
  if (not littleEndian()) {
    return val;
  }

  uint64_t retval = val;
  uint8_t* val_p = (uint8_t*)&retval;
  //Swap the bytes to convert endianness
  std::reverse(val_p, val_p + 8);
  return retval;
}
#endif

#ifndef htobe64
//htobe64 is not defined on some systems
uint64_t htobe64(uint64_t val) {
  //If the host is big endian just return.
  if (not littleEndian()) {
    return val;
  }

  uint64_t retval = val;
  uint8_t* val_p = (uint8_t*)&retval;
  //Swap the bytes to convert endianness
  std::reverse(val_p, val_p + 8);
  return retval;
}
#endif

uint64_t ntohll(uint64_t val) {
  return be64toh(val);
}

uint64_t htonll(uint64_t val) {
  return htobe64(val);
}

void toNetworkEndian(unsigned char* in, int size) {
  //Network endian is big endian so only convert if this is a little endian system.
  if (littleEndian()) {
    uint16_t* p16;
    uint32_t* p32;
    uint64_t* p64;
    //Convert based upon the number of bytes in the data.
    switch (size) {
      case 2:
        p16 = (uint16_t*)in;
        *p16 = htons(*p16);
        break;
      case 4:
        p32 = (uint32_t*)in;
        *p32 = htonl(*p32);
        break;
      case 8:
        p64 = (uint64_t*)in;
        *p64 = htonll(*p64);
        break;
      case 16:
        p64 = (uint64_t*)in;
        *p64 = htonll(*p64);
        p64 = (uint64_t*)(in+8);
        *p64 = htonll(*p64);
        break;
      default:
        //Don't do anything for other byte sizes
        break;
    }
  }
}

void fromNetworkEndian(unsigned char* in, int size) {
  //Network endian is big endian so only convert if this is a little endian system.
  if (littleEndian()) {
    uint16_t* p16;
    uint32_t* p32;
    uint64_t* p64;
    switch (size) {
      case 2:
        p16 = (uint16_t*)in;
        *p16 = ntohs(*p16);
        break;
      case 4:
        p32 = (uint32_t*)in;
        *p32 = ntohl(*p32);
        break;
      case 8:
        p64 = (uint64_t*)in;
        *p64 = ntohll(*p64);
        break;
      case 16:
        p64 = (uint64_t*)in;
        *p64 = ntohll(*p64);
        p64 = (uint64_t*)(in+8);
        *p64 = ntohll(*p64);
        break;
      default:
        //Don't do anything for other byte sizes
        break;
    }
  }
}

//Reassemble a utf16 string
std::u16string readUTF16(std::vector<unsigned char>& buff, size_t start, size_t size) {
  std::u16string str;
  //Return an empty string if the specified size is larger than the buffer
  if ( (buff.size() - start) < size*sizeof(char16_t)) {
    return str;
  }
  for (size_t index = 0; index*sizeof(char16_t) < size and start + index*sizeof(char16_t) <= buff.size(); ++index) {
    str.push_back( readPrimitive<char16_t>(buff, start + (index*sizeof(char16_t))) );
  }
  return str;
}

std::u16string readSizedUTF16(std::vector<unsigned char>& buff, int start) {
  uint32_t size = readPrimitive<uint32_t>(buff, start) / sizeof(char16_t);
  start += sizeof(size);
  return readUTF16(buff, start, size);
}

BuffReader::BuffReader(std::vector<unsigned char>& buffer, bool check_endian) : buff(buffer) {
  _out_of_range = false;
  cur_index = 0;
  buff_i = buff.begin();
  if (check_endian) {
    uint32_t endian_test = 1;
    little_endian = ( *(unsigned char*)(&endian_test) == 1 );
  } else {
    little_endian = false;
  }
}

bool BuffReader::outOfRange() {
  return _out_of_range;
}

void BuffReader::discard(size_t bytes) {
  cur_index += bytes;
  buff_i += bytes;
  if ( cur_index > buff.size() ) {
    cur_index = buff.size();
  }
}

//Reassemble a utf16 string
std::u16string BuffReader::readUTF16(size_t size) {
  std::u16string str;
  for (size_t index = 0; index < size; ++index) {
    str.push_back( this->readPrimitive<char16_t>() );
  }
  return str;
}
std::u16string BuffReader::readSizedUTF16() {
  uint32_t size = this->readPrimitive<uint32_t>() / sizeof(char16_t);
  std::u16string str;
  for (size_t index = 0; index < size; ++index) {
    str.push_back( this->readPrimitive<char16_t>() );
  }
  return str;
}

/**
 * Push a UTF16 string onto the given buffer
 * Return the total number of bytes pushed back.
 */
uint32_t pushBackUTF16(std::vector<unsigned char>& buff, const std::u16string& str) {
  uint32_t size = 0;
  //Push back each character in the string individually
  for (auto I = str.begin(); I != str.end(); ++I ) {
    size += pushBackVal(*I, buff);
  }
  return size;
}

uint32_t pushBackSizedUTF16(std::vector<unsigned char>& buff, const std::u16string& str) {
  uint32_t size = 0;
  uint32_t string_len = str.length() * sizeof(char16_t);
  size += pushBackVal(string_len, buff);
  //Push back each character in the string individually
  for (auto I = str.begin(); I != str.end(); ++I ) {
    size += pushBackVal(*I, buff);
  }
  return size;
}

