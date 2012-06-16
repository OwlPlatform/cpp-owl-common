#include "sensor_aggregator_protocol.hpp"
#include "netbuffer.hpp"
#include "sample_data.hpp"

#include <vector>
#include <algorithm>
#include <string>

std::vector<unsigned char> sensor_aggregator::makeHandshakeMsg() {
  std::vector<unsigned char> buff(4);
  std::string protocol_string = "GRAIL sensor protocol";
  buff.insert(buff.end(), protocol_string.begin(), protocol_string.end());
  //Version number and extension are both currently zero
  buff.push_back(0);
  buff.push_back(0);
  //Insert the length of the protocol string into the buffer
  pushBackVal<uint32_t>(protocol_string.length(), buff, 0);
  return buff;
}

std::vector<unsigned char> sensor_aggregator::makeSampleMsg(SampleData& sample) {
  //Keep track of the total length of the message.
  uint32_t total_length = 0;

  std::vector<unsigned char> buff;

  //Don't count the first four bytes (the message size field) in the total length.
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);
  buff.push_back(0);

  total_length += pushBackVal(sample.physical_layer, buff);
  total_length += pushBackVal(sample.tx_id, buff);
  total_length += pushBackVal(sample.rx_id, buff);
  total_length += pushBackVal(sample.rx_timestamp, buff);
  total_length += pushBackVal(sample.rss, buff);
  if (sample.sense_data.size() > 0) {
    total_length += pushContainer(sample.sense_data, buff);
  }

  //Now go back and insert the message length.
  pushBackVal<uint32_t>(total_length, buff, 0);
  
  return buff;
}

SampleData sensor_aggregator::decodeSampleMsg(std::vector<unsigned char>& buff, unsigned int length) {
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
    if (entire_length + 4 == length) {
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
