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

#ifndef __NETBUFFER_HPP__
#define __NETBUFFER_HPP__

#include <vector>
#include <string>

//Convenience function to check if the system is little endian
bool littleEndian();

//Functions to convert primitive values to and from network
//byte orderings.
//These take in pointers to and size of the primitive.
void toNetworkEndian(unsigned char* in, int size);
void fromNetworkEndian(unsigned char* in, int size);

//Convenience functions to convert 64 bit values to and from
//network and host endian byte orderings.
uint64_t ntohll(uint64_t val);
uint64_t htonll(uint64_t val);

/**
 * A function to read a primitive value from a byte buffer
 * at the specified index.
 * Pulls a value out by reassembling the bytes - only works for primitive types.
 */
template<typename T>
T readPrimitive(const std::vector<unsigned char>& buff, size_t index = 0);

/*
 * Pulls value out of a buffer and into a container.
 * Returns the number of bytes read.
 */
template<typename T>
int readPrimitiveContainer(std::vector<unsigned char>& buff, size_t index, T container);

/*
 * Pulls a sized container (size in uint32_t precedes the data) out of the buff and
 * into container. Returns the number of bytes read.
 */
template<typename T>
int readSizedContainer(std::vector<unsigned char>& buff, size_t index, T container);

//Reassemble a utf16 string from a buffer given the number of characters in the string.
std::u16string readUTF16(std::vector<unsigned char>& buff, size_t start, size_t size);

/**
 * Reassemble a utf16 string in buff at the given index. The first four
 * bytes at index are a uint32_t that give the size of the string, in bytes.
 */
std::u16string readSizedUTF16(std::vector<unsigned char>& buff, int start);

///Structure to simplify reading multiple data types from a buffer.
struct BuffReader {
  private:
    std::vector<unsigned char>::iterator buff_i;
    //Endianness determines if data must be converted after it comes from the network.
    bool little_endian;
    //Remember if the user tried to read more than was in the buffer
    bool _out_of_range;

  public:
    unsigned int cur_index;
    //A reference to a byte buffer
    std::vector<unsigned char>& buff;

    BuffReader(std::vector<unsigned char>& buffer, bool check_endian = true);

    //Return true if read commands tried to go beyond the buffer's size
    bool outOfRange();

    //Discard the specified number of bytes.
    void discard(size_t bytes);

    template<typename T>
    T readPrimitive() {
      T value = (T)0;
      //Treat the given type as an array of bytes and push the bytes
      //into it one byte at a time
      unsigned char* out = (unsigned char*)&value;
      if ( cur_index + sizeof(T) <= buff.size() ) {
        std::copy(buff_i, buff_i + sizeof(T), out);
        cur_index += sizeof(T);
        std::advance(buff_i, sizeof(T));
        //Convert to host endian from network endian
        fromNetworkEndian(out, sizeof(T));
      }
      else {
        _out_of_range = true;
      }
      return value;
    }

    template<typename T>
    void convertPrimitive(T& value) {
      value = this->readPrimitive<T>();
    }

    /*
     * Pulls value out of the buffer and into a container.
     * The number of elements in the container determines the number of
     * elements read.
     */
    template<typename T>
    void readPrimitiveContainer(T& container) {
      size_t val_size = sizeof(typename T::value_type);
      //Read in a T::value_type for each position in the container
      for (auto I = container.begin(); (cur_index+val_size <= buff.size()) and I != container.end(); ++I) {
        //Read in the T::value_type
        *I = this->readPrimitive<typename T::value_type>();
      }
    }

    /**
     * Pulls value out of the buffer and into a container.
     * The number of elements to read is read as a uint32_t in the buffer.
     */
    template<typename T>
    std::vector<T> readSizedVector() {
      size_t val_size = sizeof(T);
      uint32_t size = this->readPrimitive<uint32_t>() / val_size;
      std::vector<T> vect;
      //Read in a T::value_type for each position in the container
      for (size_t index = 0; (cur_index+val_size <= buff.size()) and index < size; ++index) {
        //Read in the next T
        T val = this->readPrimitive<T>();
        vect.push_back(val);
      }
      return vect;
    }

    ///Reassemble a utf16 string given the number of characters the string contains.
    std::u16string readUTF16(size_t size);

    /**
     * Reassemble a utf16 string. The next 4 bytes of the buffer should
     * be the size (in bytes) of the string.
     */
    std::u16string readSizedUTF16();
};

/**
 * Push a container onto the given buffer. The first four bytes pushed
 * are a uint32_t that indicate the size of the container, in
 * bytes. Return the total number of bytes pushed back.
 */
template<typename T>
uint32_t pushSizedContainer(T container, std::vector<unsigned char>& buff);

/**
 * Push a UTF16 string onto the given buffer
 * Return the total number of bytes pushed back.
 */
uint32_t pushBackUTF16(std::vector<unsigned char>& buff, const std::u16string& str);

/**
 * Push a UTF16 string onto the given buffer. The first four bytes pushed
 * are a uint32_t that indicate the size of the string, in bytes.
 * Return the total number of bytes pushed back.
 */
uint32_t pushBackSizedUTF16(std::vector<unsigned char>& buff, const std::u16string& str);

/**
 * Push a value into the character buffer at the specified location
 * Return the total number of bytes pushed back.
 */
template<typename T>
uint32_t pushBackVal(T val, std::vector<unsigned char>& buff, size_t index);

/**
 * Push a value into the character buffer
 * Return the total number of bytes pushed back.
 */
template<typename T>
uint32_t pushBackVal(T val, std::vector<unsigned char>& buff);

/*******************************************************************************
 * Template function definitions.
 ******************************************************************************/

template<typename T>
T readPrimitive(const std::vector<unsigned char>& buff, size_t index) {
  T value = 0;
  //Treat the given type as an array of bytes and push the bytes
  //into it one byte at a time
  unsigned char* out = (unsigned char*)&value;
  size_t max_shift = sizeof(T) - 1;
  if ( (index + max_shift) < buff.size() ) {
    for (size_t shift = 0; shift <= max_shift; ++shift) {
      out[shift] = buff[index + shift];
    }
  }
  //Convert to host endian from network endian
  fromNetworkEndian(out, sizeof(T));
  return value;
}

template<typename T>
int readPrimitiveContainer(std::vector<unsigned char>& buff, size_t index, T container) {
  int total_read = 0;
  int max_shift = sizeof(*container.begin()) - 1;
  for (auto I = container.begin(); I != container.end(); ++I) {
    for (int shift = 0; shift <= max_shift; ++shift) {
      *I |= (decltype(*I))buff[index + shift] << ( 8 * (max_shift - shift));
    }
    total_read += sizeof(*I);
    index += sizeof(*I);
  }
  return total_read;
}

template<typename T>
int readSizedContainer(std::vector<unsigned char>& buff, size_t index, T container) {
  int max_shift = sizeof(typename T::value_type);
  uint32_t size = readPrimitive<uint32_t>(buff, index) / max_shift;
  index += sizeof(size);
  container = T(size);
  return readPrimitiveContainer(buff, index, container);
}

template<typename T>
uint32_t pushSizedContainer(T container, std::vector<unsigned char>& buff) {
  uint32_t size = 0;
  uint32_t container_len = container.size() * sizeof(typename T::value_type);
  size += pushBackVal(container_len, buff);
  //Push back each value in the container individually
  for (auto I = container.begin(); I != container.end(); ++I ) {
    size += pushBackVal<typename T::value_type>(*I, buff);
  }
  return size;
}

template<typename T>
uint32_t pushContainer(T container, std::vector<unsigned char>& buff) {
  uint32_t size = 0;
  //Push back each value in the container individually
  for (auto I = container.begin(); I != container.end(); ++I ) {
    size += pushBackVal<typename T::value_type>(*I, buff);
  }
  return size;
}

/**
 * Push a value into the character buffer at the specified location
 * Return the total number of bytes pushed back.
 */
template<typename T>
uint32_t pushBackVal(T val, std::vector<unsigned char>& buff, size_t index) {
  //Treat the given value as an array of bytes and push the bytes
  //into the buff one at a time
  unsigned char* in = (unsigned char*)&val;
  //Change the order of the bytes if this machine architecture does not
  //store data in network byte order (big endian).
  //
  toNetworkEndian(in, sizeof(T));
  std::copy(in, in+sizeof(T), buff.begin()+index);
  return sizeof(T);
}

/**
 * Push a value into the character buffer
 * Return the total number of bytes pushed back.
 */
template<typename T>
uint32_t pushBackVal(T val, std::vector<unsigned char>& buff) {
  //Treat the given value as an array of bytes and push the bytes
  //into the buff one at a time
  unsigned char* in = (unsigned char*)&val;
  //Change the order of the bytes if this machine architecture does not
  //store data in network byte order (big endian).
  toNetworkEndian(in, sizeof(T));
  for (size_t shift = 0; shift < sizeof(T); ++shift) {
    buff.push_back(in[shift]);
  }
  return sizeof(T);
}

#endif

