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

#ifndef __MESSAGE_RECEIVER_HPP__
#define __MESSAGE_RECEIVER_HPP__

#include <vector>
#include "simple_sockets.hpp"
#include <mutex>

class MessageReceiver {
  private:
    std::mutex sock_mutex;

  public:
    /**
     * Hold a buffer for a unfinished message sent in the last packet that
     * needs a new TCP packet to be processed completely.
     */
    std::vector<unsigned char> previous_unfinished;

    /**
     * Receiving socket.
     */
    ClientSocket& sock;

    /**
     * Create a buffer for receiving messages so that this is not constantly
     * allocating and deallocating memory.
     */
    std::vector<unsigned char> raw_messages;

    /**
     * Constructor that takes in reference to a socket for packet reception.
     * This class does not own the memory for this socket.
     */
    MessageReceiver(ClientSocket& s);

    /**
     * Nonblocking call that checks if a message is ready
     * to be read. If this returns true then the getNextMessage
     * function will return a new packet immediately.
     */
    bool messageAvailable(bool& interrupted);

    /**
     * Blocking call that returns the next message in a vector.
     * This always returns whole single messages regardless of how
     * packets are combined or split in transit.
     * If interrupted becomes true this will return an empty buffer.
     * This is most effective when the ClientSocket is non-blocking.
     */
    std::vector<unsigned char> getNextMessage(bool& interrupted);
};

#endif

