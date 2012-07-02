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
 * @file aggregator_solver_protocol.cpp
 * Defines messages between aggregators and solvers in the
 * owl/grail platform format.
 *
 * @author Bernhard Firner
 */

#include "aggregator_solver_protocol.hpp"
#include "netbuffer.hpp"
#include "sample_data.hpp"

#include <algorithm>
#include <string>
#include <iterator>
#include <array>
#include <vector>

#include <iostream>

using namespace aggregator_solver;

std::vector<unsigned char> aggregator_solver::makeHandshakeMsg() {
  std::vector<unsigned char> buff(4);
  std::string protocol_string = "GRAIL solver protocol";
  buff.insert(buff.end(), protocol_string.begin(), protocol_string.end());
  //Version number and extension are both currently zero
  buff.push_back(0);
  buff.push_back(0);
  //Insert the length of the protocol string into the buffer
  pushBackVal<uint32_t>(protocol_string.length(), buff, 0);
  return buff;
}

std::vector<unsigned char>&& aggregator_solver::makeCertMsg();

std::vector<unsigned char>&& aggregator_solver::ackCertMsg();

std::vector<unsigned char> aggregator_solver::makeSubscribeReqMsg(Subscription& rules) {
  //The total length starts out at one for the message type field.
  //Each rule will add additional length.
  uint32_t total_length = 1;

  std::vector<unsigned char> buff;

  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back((unsigned char)subscription_request);

  //Push back the rules
  {
    //First push the number of rules onto the buffer
    uint32_t num_rules = rules.size();
    total_length += pushBackVal(num_rules, buff);

    //For each rule push back the physical layer, the number of
    //tx rules and the tx rules, the number of boxes and the boxes,
    //and the number of FSM/TR rules and the FSM/TR rules.
    for (auto rule = rules.begin(); rule != rules.end(); ++rule) {
      //Push the physical layer
      total_length += pushBackVal(rule->physical_layer, buff);

      //Push the transmitter information
      uint32_t num_txers = rule->txers.size();
      total_length += pushBackVal(num_txers, buff);
      //Each transmitter is made of an ID and a MASK
      for (auto txer = rule->txers.begin(); txer != rule->txers.end(); ++txer) {
        total_length += pushBackVal(txer->base_id, buff);
        total_length += pushBackVal(txer->mask, buff);
      }

      //Finish the rule with the update interval
      total_length += pushBackVal(rule->update_interval, buff);
    }
  }

  //Store the message length in the first four bytes
  pushBackVal<uint32_t>(total_length, buff, 0);

  return buff;
}

Subscription aggregator_solver::decodeSubscribeMsg(std::vector<unsigned char>& buff, unsigned int length) {
  Subscription rules;

  //Only process the message if its type matches one of the subscription ids
  //and if the message is large enough to be valid.
  if ( length > 4 ) {

    BuffReader reader(buff);
    uint32_t entire_length = reader.readPrimitive<uint32_t>();
    MessageID msg_type = MessageID(reader.readPrimitive<uint8_t>());
    if (entire_length + 4 == length and
        (subscription_request == msg_type or
         subscription_response == msg_type)) {

      uint32_t num_rules = reader.readPrimitive<uint32_t>();
      for (size_t rule_no = 0; rule_no < num_rules; ++rule_no) {
        Rule rule;
        rule.physical_layer = reader.readPrimitive<unsigned char>();
        //Read in the txer ids and masks
        uint32_t num_txers = reader.readPrimitive<uint32_t>();
        for (size_t txer_no = 0; txer_no < num_txers; ++txer_no) {
          Transmitter t;
          t.base_id = reader.readPrimitive<uint128_t>();
          t.mask = reader.readPrimitive<uint128_t>();
          rule.txers.push_back(t);
        }

        rule.update_interval = reader.readPrimitive<decltype(rule.update_interval)>();

        rules.push_back(rule);

      }
    }
  }

  return rules;
}

std::vector<unsigned char> aggregator_solver::makeSampleMsg(SampleData& sample) {
  //The total length starts out at one for the message type field.
  //Each rule will add additional length.
  uint32_t total_length = 1;

  std::vector<unsigned char> buff;

  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back((unsigned char)server_sample);

  total_length += pushBackVal(sample.physical_layer, buff);
  total_length += pushBackVal(sample.tx_id, buff);
  total_length += pushBackVal(sample.rx_id, buff);
  total_length += pushBackVal(sample.rx_timestamp, buff);
  total_length += pushBackVal(sample.rss, buff);
  if (sample.sense_data.size() > 0) {
    total_length += pushContainer(sample.sense_data, buff);
  }

  pushBackVal<uint32_t>(total_length, buff, 0);
  
  return buff;
}

SampleData aggregator_solver::decodeSampleMsg(std::vector<unsigned char>& buff, unsigned int length) {
  SampleData sample;
  //Assume that the sample is invalid until we manage to get data out of buff
  sample.valid = false;

  //Only process the message if its type matches the server_sample id
  //and the length is sane.
  if ( length > 4 ) {

    //Make a new buffer reader for this message and put the data into the
    //sample structure.
    BuffReader reader(buff);
    uint32_t entire_length = reader.readPrimitive<uint32_t>();
    MessageID msg_type = MessageID(reader.readPrimitive<uint8_t>());
    if (entire_length + 4 == length and
        server_sample == msg_type) {
      //Since we found data to read into the sample, mark it as valid.
      sample.valid = true;
      sample.physical_layer = reader.readPrimitive<decltype(sample.physical_layer)>();
      sample.tx_id = reader.readPrimitive<decltype(sample.tx_id)>();
      sample.rx_id = reader.readPrimitive<decltype(sample.rx_id)>();
      sample.rx_timestamp = reader.readPrimitive<decltype(sample.rx_timestamp)>();
      sample.rss = reader.readPrimitive<decltype(sample.rss)>();
      //Get the size of the sense data
      unsigned int left = length - reader.cur_index;
      sample.sense_data = std::vector<unsigned char>(left);
      reader.readPrimitiveContainer(sample.sense_data);
    }
  }

  return sample;
}


