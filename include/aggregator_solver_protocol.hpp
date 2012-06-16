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
 * @file aggregator_solver_protocol.hpp
 * Defines the protocol interface between an aggregator and solver.
 *
 * @author Bernhard Firner
 */

#ifndef __AGGREGATOR_SOLVER_PROTOCOL_HPP__
#define __AGGREGATOR_SOLVER_PROTOCOL_HPP__

#include <array>
#include <string>
#include <vector>
#include <iostream>

#include "sample_data.hpp"

namespace aggregator_solver {

  enum MessageID {keep_alive            = 0,
                  certificate           = 1,
                  ack_certificate       = 2, //There is no message for certificate denial
                  subscription_request  = 3,
                  subscription_response = 4,
                  device_position       = 5,
                  server_sample         = 6,
                  buffer_overrun        = 7};

  //Requests for transmitters are made up of a base ID and a mask (to specify ranges)
  struct Transmitter {
    uint128_t base_id;
    uint128_t mask;
  };

  struct Rule {
    unsigned char physical_layer;
    //uint32_t num_txers; --Implicit with the vector but sent in the buffer
    std::vector<Transmitter> txers;
    //The desired update interval, in milliseconds
    uint64_t update_interval;
  };

  typedef std::vector<Rule> Subscription;

  std::vector<unsigned char> makeHandshakeMsg();

  std::vector<unsigned char>&& makeCertMsg();

  std::vector<unsigned char>&& ackCertMsg();

  std::vector<unsigned char> makeSubscribeReqMsg(Subscription& sub);

  Subscription decodeSubscribeMsg(std::vector<unsigned char>& buff, unsigned int length);

  std::vector<unsigned char> makeSampleMsg(SampleData& sample);

  SampleData decodeSampleMsg(std::vector<unsigned char>& buff, unsigned int length);

};

#endif

