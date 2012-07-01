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

/**
 * @file message_receiver.hpp
 * Defines the MessageReceiver class that simplifies receiving messages in the
 * owl/grail platform format.
 *
 * @author Bernhard Firner
 */

#include "message_receiver.hpp"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <unistd.h>
#include "simple_sockets.hpp"
#include <poll.h>
#include <sys/poll.h>

#include "netbuffer.hpp"

MessageReceiver::MessageReceiver(ClientSocket& s) : sock(s), raw_messages(10000) {
  previous_unfinished = std::vector<unsigned char>(0);
}

bool MessageReceiver::messageAvailable(bool& interrupted) {
  //Try to process data from the previously unfinished buffer, but
  //if there is not enough data available receive from the network.
  //There must be at least 4 bytes to try to process a packet and
  //the size of a piece is stored in its first four bytes.
  uint32_t piece_length = 0;
  if (previous_unfinished.size() >= 4) {
    piece_length = readPrimitive<uint32_t>(previous_unfinished, 0) + 4;
  }
  std::unique_lock<std::mutex> lck(sock_mutex);
  //See if we need more data to complete the current packet
  //True if we do not have enough data to determine a packet size
  //or we do not have enough data for a complete packet.
  if (not interrupted and 
         (previous_unfinished.size() < 4 or
          previous_unfinished.size() < piece_length)) {
    //Wait 10ms for data on the socket.
    if (not sock.inputReady(10)) {
      return false;
    }

    //Receive a message from the network to get more data.
    ssize_t length = sock.receive(raw_messages);
    if ( -1 == length ) {
      //Check for a nonblocking socket
      if (EAGAIN == errno or
          EWOULDBLOCK == errno) {
        //No message available
        return false;
      }
      else {
        //Check the error value to give a more explicit error message.
        std::string err_str(strerror(errno));
        throw std::runtime_error("Error receiving message or the connection was closed: "+err_str);
      }
    }
    else if (0 == length) {
      //0 length indicates end of file (a closed connection)
      throw std::runtime_error("Connection was closed.");
    }
    //Append the message data into the previous_unfinished buffer.
    previous_unfinished.insert(previous_unfinished.end(),
        raw_messages.begin(), raw_messages.begin()+length);
  }

  //Return true if there is enough data to form a packet.
  piece_length = readPrimitive<uint32_t>(previous_unfinished, 0) + 4;
  return piece_length <= previous_unfinished.size();
}

std::vector<unsigned char> MessageReceiver::getNextMessage(bool& interrupted) {
  //Try to process data from the previously unfinished buffer, but
  //if there is not enough data available receive from the network.
  //There must be at least 4 bytes to try to process a packet and
  //the size of a piece is stored in its first four bytes.
  uint32_t piece_length = 0;
  if (previous_unfinished.size() >= 4) {
    piece_length = readPrimitive<uint32_t>(previous_unfinished, 0) + 4;
    //std::cerr<<"Getting piece with length "<<piece_length<<'\n';
  }
  std::unique_lock<std::mutex> lck(sock_mutex);
  //Get the next packet - keep going while the size can't be read or the
  //size was read but the buffer does not have the whole packet
  while (not interrupted and 
         (previous_unfinished.size() < 4 or
          previous_unfinished.size() < piece_length)) {

    //Receive a message from the network to get more data.
    ssize_t length = sock.receive(raw_messages);
    if ( -1 == length ) {
      //Check for a nonblocking socket
      if (EAGAIN == errno or
          EWOULDBLOCK == errno) {
        //Sleep for one millisecond and try again
        usleep(1000);
        continue;
      }
      else {
        //Check the error value to give a more explicit error message.
        std::string err_str(strerror(errno));
        throw std::runtime_error("Error receiving message or the connection was closed: "+err_str);
      }
    }
    else if (0 == length) {
      //0 length indicates end of file (a closed connection)
      throw std::runtime_error("Connection was closed.");
    }
    //Append the message data into the previous_unfinished buffer.
    previous_unfinished.insert(previous_unfinished.end(),
        raw_messages.begin(), raw_messages.begin()+length);

    //Read in the piece length and continue receiving packets until at least
    //that much data has been received. The length bytes themselves take 4 bytes
    if (previous_unfinished.size() >= 4) {
      piece_length = readPrimitive<uint32_t>(previous_unfinished, 0) + 4;
      //std::cerr<<"Continuing next piece with length "<<piece_length<<'\n';
    }
  }
  //If the receive function is interrupted just leave.
  if (interrupted) {
    return std::vector<unsigned char>(0);
  }

  piece_length = readPrimitive<uint32_t>(previous_unfinished, 0) + 4;
  std::vector<unsigned char> new_packet(previous_unfinished.begin(),
      previous_unfinished.begin()+piece_length);
  //Save the unused portion of the buffer. Otherwise there is no more data left over
  //so zero the buffer.
  if ( piece_length < previous_unfinished.size() ) {
    previous_unfinished = std::vector<unsigned char>(previous_unfinished.begin() + piece_length, previous_unfinished.end());
    //std::cerr<<"Saving data for next call:\n";
    //std::for_each(previous_unfinished.begin(), previous_unfinished.end(), [&](unsigned char c) {
        //std::cerr<<c;
        //});
    //std::cerr<<'\n';
  }
  else {
    previous_unfinished.clear();
  }
  return new_packet;
}

