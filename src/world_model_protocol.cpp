/**
 * This file contains the implementation of the world model protocol
 * between solvers and the world model and between clients and the world model.
 */

#include <array>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <chrono>

#include "world_model_protocol.hpp"
#include "netbuffer.hpp"

using namespace world_model;
using namespace std::chrono;

using std::pair;
using std::u16string;
using std::vector;


//Grail time is milliseconds since 1970
grail_time world_model::getGRAILTime() {
  //Using time since the epoch in milliseconds as grail time
  milliseconds t = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  return t.count();
}

/******************************************************************************
 * The following functions comprise the client <-> world model interface.
 *****************************************************************************/
std::vector<unsigned char> client::makeHandshakeMsg() {
  std::vector<unsigned char> buff(4);
  std::string protocol_string = "GRAIL client protocol";
  buff.insert(buff.end(), protocol_string.begin(), protocol_string.end());
  //Version number and extension are both currently zero
  buff.push_back(0);
  buff.push_back(0);
  //Insert the length of the protocol string into the buffer
  pushBackVal<uint32_t>(protocol_string.size(), buff, 0);
  return buff;
}

Buffer client::makeKeepAlive() {
  Buffer buff;
  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 1;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::keep_alive, buff);
  return buff;
}

Buffer client::makeSnapshotRequest(const client::Request& request, uint32_t ticket) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::snapshot_request, buff);

  //Push back the ticket number
  total_length += pushBackVal(ticket, buff);

  //Push back the length and content of the query URI
  total_length += pushBackSizedUTF16(buff, request.object_uri);

  //Push back the number of attribute strings
  total_length += pushBackVal((uint32_t)request.attributes.size(), buff);

  //Push back each attribute string
  for (auto attr = request.attributes.begin(); attr != request.attributes.end(); ++attr) {
    total_length += pushBackSizedUTF16(buff, *attr);
  }
  total_length += pushBackVal(request.start, buff);
  total_length += pushBackVal(request.stop_period, buff);

  //Push back the actual length into the buffer.
  pushBackVal(total_length, buff, 0);
  return buff;
}

Buffer client::makeRangeRequest(const client::Request& request, uint32_t ticket) {
  Buffer buff = makeSnapshotRequest(request, ticket);
  buff[4] = (uint8_t)MessageID::range_request;
  return buff;
}

Buffer client::makeStreamRequest(const client::Request& request, uint32_t ticket) {
  Buffer buff = makeSnapshotRequest(request, ticket);
  buff[4] = (uint8_t)MessageID::stream_request;
  return buff;
}

std::tuple<client::Request, uint32_t> client::decodeSnapshotRequest(Buffer& buff) {
  BuffReader reader(buff);
  client::Request request;
  uint32_t ticket = 0;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  //Check to make sure this is a valid type specification message.
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::snapshot_request) {
    //Read in the ticket number, object URI, and object attributes
    ticket = reader.readPrimitive<uint32_t>();
    request.object_uri = reader.readSizedUTF16();
    uint32_t total_attributes = reader.readPrimitive<uint32_t>();

    for (size_t i = 0; i < total_attributes; ++i) {
      URI attr = reader.readSizedUTF16();
      request.attributes.push_back(attr);
    }
    request.start = reader.readPrimitive<grail_time>();
    request.stop_period = reader.readPrimitive<grail_time>();
  }
  if (reader.outOfRange()) {
    return std::make_tuple(client::Request(), ticket);
  }

  return std::make_tuple(request, ticket);
}

std::tuple<client::Request, uint32_t> client::decodeRangeRequest(Buffer& buff) {
  //Reuse the snapshot request message code
  if (buff.size() >= 5 and
      MessageID::range_request == MessageID(buff[4])) {
    buff[4] = (uint8_t)MessageID::snapshot_request;
    return decodeSnapshotRequest(buff);
  }
  //Otherwise fail
  Request empty;
  return std::make_tuple(empty, (uint32_t)0);
}

std::tuple<client::Request, uint32_t> client::decodeStreamRequest(Buffer& buff) {
  //Reuse the snapshot request message code
  if (buff.size() >= 5 and
      MessageID::stream_request == MessageID(buff[4])) {
    buff[4] = (uint8_t)MessageID::snapshot_request;
    return decodeSnapshotRequest(buff);
  }
  //Otherwise fail
  Request empty;
  return std::make_tuple(empty, (uint32_t)0);
}

Buffer client::makeAttrAliasMsg(const vector<client::AliasType>& attribute_aliases) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::attribute_alias, buff);

  //Push back the number of aliases
  total_length += pushBackVal<uint32_t>((uint32_t)attribute_aliases.size(), buff);

  //Push back each alias string
  for (auto alias = attribute_aliases.begin(); alias != attribute_aliases.end(); ++alias) {
    total_length += pushBackVal<uint32_t>(alias->alias, buff);
    total_length += pushBackSizedUTF16(buff, alias->type);
  }

  //Push back the actual length into the buffer.
  pushBackVal(total_length, buff, 0);
  return buff;
}

Buffer client::makeOriginAliasMsg(const vector<client::AliasType>& origin_aliases) {
  //Reuse the attribute alias message code
  Buffer buff = makeAttrAliasMsg(origin_aliases);
  //Change the message type
  buff[4] = (uint8_t)MessageID::origin_alias;
  return buff;
}

vector<client::AliasType> client::decodeAttrAliasMsg(Buffer& buff) {
  BuffReader reader(buff);
  vector<client::AliasType> aliases;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  //Check to make sure this is a valid message.
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::attribute_alias) {
    uint32_t total_attributes = reader.readPrimitive<uint32_t>();

    for (size_t i = 0; i < total_attributes; ++i) {
      uint32_t alias = reader.readPrimitive<uint32_t>();
      u16string type = reader.readSizedUTF16();
      aliases.push_back(client::AliasType{alias, type});
    }
  }
  if (reader.outOfRange()) {
    return vector<client::AliasType>();
  }

  return aliases;
}

vector<client::AliasType> client::decodeOriginAliasMsg(Buffer& buff) {
  //Reuse the attribute alias message code
  if (buff.size() > 5 and
      MessageID::origin_alias == MessageID(buff[4])) {
    buff[4] = (uint8_t)MessageID::attribute_alias;
    return decodeAttrAliasMsg(buff);
  }
  //Otherwise fail
  return vector<client::AliasType>();
}

Buffer client::makeRequestComplete(uint32_t ticket_number) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 5;
  //Push back space for the total length.
  pushBackVal(total_length, buff);

  //Push back the message type.
  pushBackVal(MessageID::request_complete, buff);

  //Push back the ticket number
  pushBackVal(ticket_number, buff);

  return buff;
}

uint32_t client::decodeRequestComplete(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::request_complete == msg_type) {
    //Return the ticket number
    return reader.readPrimitive<uint32_t>();
  }

  //Return 0 on failure
  return 0;
}

Buffer client::makeCancelRequest(uint32_t ticket_number) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 5;
  //Push back space for the total length.
  pushBackVal(total_length, buff);

  //Push back the message type.
  pushBackVal(MessageID::cancel_request, buff);

  //Push back the ticket number
  pushBackVal(ticket_number, buff);

  return buff;
}

uint32_t client::decodeCancelRequest(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::cancel_request == msg_type) {
    //Return the ticket number
    return reader.readPrimitive<uint32_t>();
  }

  //Return 0 on failure
  return 0;
}

Buffer client::makeDataMessage(const AliasedWorldData& wd, uint32_t ticket) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::data_response, buff);

  //Push back the length and content of the world object's URI
  total_length += pushBackSizedUTF16(buff, wd.object_uri);

  //Push back the number of attributes
  total_length += pushBackVal<uint32_t>(ticket, buff);

  //Push back the number of attributes
  total_length += pushBackVal<uint32_t>((uint32_t)wd.attributes.size(), buff);

  //Push back each attribute
  for (auto attr = wd.attributes.begin(); attr != wd.attributes.end(); ++attr) {
    total_length += pushBackVal<uint32_t>(attr->name_alias, buff);
    total_length += pushBackVal<grail_time>(attr->creation_date, buff);
    total_length += pushBackVal<grail_time>(attr->expiration_date, buff);
    total_length += pushBackVal<uint32_t>(attr->origin_alias, buff);
    total_length += pushSizedContainer(attr->data, buff);
  }

  //Push back the actual length into the buffer.
  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<AliasedWorldData, uint32_t> client::decodeDataMessage(Buffer& buff) {
  BuffReader reader(buff);
  AliasedWorldData wd;
  uint32_t ticket_number = 0;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::data_response == msg_type) {
    wd.object_uri = reader.readSizedUTF16();
    ticket_number = reader.readPrimitive<uint32_t>();

    uint32_t total_attributes = reader.readPrimitive<uint32_t>();

    for (; total_attributes > 0; --total_attributes) {
      AliasedAttribute aa;
      aa.name_alias = reader.readPrimitive<uint32_t>();
      aa.creation_date = reader.readPrimitive<grail_time>();
      aa.expiration_date = reader.readPrimitive<grail_time>();
      aa.origin_alias = reader.readPrimitive<uint32_t>();
      aa.data = reader.readSizedVector<uint8_t>();
      wd.attributes.push_back(aa);
    }
  }
  //If we went out of range then the attribute count was wrong
  if (reader.outOfRange()) {
    return std::make_tuple(AliasedWorldData(), (uint32_t)0);
  }


  return std::make_tuple(wd, ticket_number);
}

Buffer client::makeURISearch(const URI& uri) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::uri_search, buff);

  //Push back the query string
  total_length += pushBackUTF16(buff, uri);

  pushBackVal(total_length, buff, 0);
  return buff;
}

URI client::decodeURISearch(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::uri_search == msg_type) {
    u16string uri = reader.readUTF16((buff.size() - reader.cur_index)/2);
    return uri;
  }

  //Return on empty string upon failure.
  return u"";
}

Buffer client::makeURISearchResponse(const std::vector<URI>& uris) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::uri_response, buff);
  for (auto uri = uris.begin(); uri != uris.end(); ++uri) {
    total_length += pushBackSizedUTF16(buff, *uri);
  }

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::vector<URI> client::decodeURISearchResponse(Buffer& buff) {
  BuffReader reader(buff);
  std::vector<URI> uris;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::uri_response == msg_type) {
    uint32_t total_uris = reader.readPrimitive<uint32_t>();
    for (; total_uris > 0; --total_uris) {
      URI uri = reader.readSizedUTF16();
      uris.push_back(uri);
    }
  }
  //If we went out of range then the total URI count was invalid
  if (reader.outOfRange()) {
    return std::vector<URI>();
  }

  return uris;
}

Buffer client::makeOriginPreference(const std::vector<pair<std::u16string, int32_t>>& weights) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::origin_preference, buff);
  for (auto w = weights.begin(); w != weights.end(); ++w) {
    total_length += pushBackSizedUTF16(buff, w->first);
    total_length += pushBackVal<int32_t>(w->second, buff);
  }

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::vector<pair<std::u16string, int32_t>> client::decodeOriginPreference(Buffer& buff) {
  BuffReader reader(buff);
  std::vector<pair<std::u16string, int32_t>> weights;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();
  if (buff.size() == total_length + 4 and
      MessageID::origin_preference == msg_type) {
    while (reader.cur_index < buff.size()) {
      weights.push_back(std::make_pair(reader.readSizedUTF16(), reader.readPrimitive<int32_t>()));
    }
  }
  //If we went out of range then the string sizes were invalid
  if (reader.outOfRange()) {
    return std::vector<pair<std::u16string, int32_t>>();
  }

  return weights;
}

/******************************************************************************
 * The following functions comprise the solver <-> world model interface.
 *****************************************************************************/

std::vector<unsigned char> solver::makeHandshakeMsg() {
  std::vector<unsigned char> buff(4);
  std::string protocol_string = "GRAIL world model protocol";
  buff.insert(buff.end(), protocol_string.begin(), protocol_string.end());
  //Version number and extension are both currently zero
  buff.push_back(0);
  buff.push_back(0);
  //Insert the length of the protocol string into the buffer
  pushBackVal<uint32_t>(protocol_string.size(), buff, 0);
  return buff;
}

Buffer solver::makeKeepAlive() {
  Buffer buff;
  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 1;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::keep_alive, buff);
  return buff;
}

/**
 * Before a solver submits new kinds of data to the world model it must first
 * identify itself and the types of data it can create.
 */
Buffer solver::makeTypeAnnounceMsg(const vector<solver::AliasType>& type_alias, const u16string& origin) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::type_announce, buff);

  //Push back the number of aliases
  total_length += pushBackVal((uint32_t)type_alias.size(), buff);

  //Push back each alias string
  for (auto alias = type_alias.begin(); alias != type_alias.end(); ++alias) {
    total_length += pushBackVal(alias->alias, buff);
    total_length += pushBackSizedUTF16(buff, alias->type);
    total_length += pushBackVal((uint8_t)(alias->on_demand ? 1 : 0), buff);
  }

  total_length += pushBackUTF16(buff, origin);

  //Push back the actual length into the buffer.
  pushBackVal(total_length, buff, 0);
  return buff;
}

pair<std::vector<solver::AliasType>, u16string> solver::decodeTypeAnnounceMsg(Buffer& buff) {
  BuffReader reader(buff);
  vector<AliasType> aliases;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  //Check to make sure this is a valid message.
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::type_announce) {
    uint32_t total_aliases = reader.readPrimitive<uint32_t>();

    for (size_t i = 0; i < total_aliases; ++i) {
      uint32_t alias = reader.readPrimitive<uint32_t>();
      u16string type = reader.readSizedUTF16();
      uint8_t on_demand = reader.readPrimitive<uint8_t>();
      aliases.push_back(AliasType{alias, type, on_demand != 0});
    }
  }
  u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);

  //If we went out of range then alias count was incorrect
  if (reader.outOfRange()) {
    return std::make_pair(std::vector<AliasType>(), u"");
  }

  return std::make_pair(aliases, origin);
}

Buffer solver::makeStartOnDemand(const std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>& aliases) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::start_on_demand, buff);

  //Push back the total number of aliases
  total_length += pushBackVal<uint32_t>(aliases.size(), buff);
  //Push back the aliases
  for (auto alias = aliases.begin(); alias != aliases.end(); ++alias) {
    total_length += pushBackVal<uint32_t>(std::get<0>(*alias), buff);
    //Push back the total number of requests for this type.
    total_length += pushBackVal<uint32_t>(std::get<1>(*alias).size(), buff);
    for (auto attr = std::get<1>(*alias).begin(); attr != std::get<1>(*alias).end(); ++attr) {
      total_length += pushBackSizedUTF16(buff, *attr);
    }
  }

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::vector<std::tuple<uint32_t, std::vector<std::u16string>>> solver::decodeStartOnDemand(Buffer& buff) {
  BuffReader reader(buff);
  std::vector<std::tuple<uint32_t, std::vector<std::u16string>>> aliases;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  //Check to make sure this is a valid message.
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::start_on_demand) {

    uint32_t total_aliases = reader.readPrimitive<uint32_t>();
    for (uint32_t alias = 0; alias < total_aliases; ++alias) {
      uint32_t cur_alias = reader.readPrimitive<uint32_t>();
      uint32_t total_attribs = reader.readPrimitive<uint32_t>();
      std::vector<std::u16string> attribs;
      for (uint32_t attr = 0; attr < total_attribs; ++attr) {
        attribs.push_back(reader.readSizedUTF16());
      }
      aliases.push_back(std::make_tuple(cur_alias, attribs));
    }
  }
  //If we went out of range then the alias count was incorrect
  if (reader.outOfRange()) {
    return std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>();
  }

  return aliases;
}

Buffer solver::makeStopOnDemand(const std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>& aliases) {
  Buffer buff = makeStartOnDemand(aliases);
  buff[4] = (uint8_t)MessageID::stop_on_demand;
  return buff;
}

std::vector<std::tuple<uint32_t, std::vector<std::u16string>>> solver::decodeStopOnDemand(Buffer& buff) {
  //Check to make sure this is a valid message.
  BuffReader reader(buff);
  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::stop_on_demand) {

    buff[4] = (uint8_t)MessageID::start_on_demand;
    return decodeStartOnDemand(buff);
  }
  //Otherwise return an empty vector
  return std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>();
}

/**
 * Data sent from the solver to modify an attribute in the world model.
 * The world model uses the previously announced origin and type aliases
 * when decoding this message.
 */
Buffer solver::makeSolutionMsg(bool create_uris, const std::vector<SolutionData>& solutions) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;
  //Push back space in the buffer for the total length.
  pushBackVal(total_length, buff);
  //Push back the message type.
  total_length += pushBackVal(MessageID::solver_data, buff);

  total_length += pushBackVal<uint8_t>(create_uris ? 1 : 0, buff);
  //Push back the number of solutions
  total_length += pushBackVal<uint32_t>(solutions.size(), buff);

  //Push back each solution
  for (auto soln = solutions.begin(); soln != solutions.end(); ++soln) {
    total_length += pushBackVal(soln->type_alias, buff);
    total_length += pushBackVal(soln->time, buff);
    total_length += pushBackSizedUTF16(buff, soln->target);
    total_length += pushSizedContainer(soln->data, buff);
  }

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<bool, std::vector<solver::SolutionData>> solver::decodeSolutionMsg(Buffer& buff) {
  BuffReader reader(buff);
  bool create_uris = false;
  vector<solver::SolutionData> solutions;

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  //Check to make sure this is a valid message.
  if ( buff.size() == (total_length + 4) and
       msg_type == MessageID::solver_data) {
    create_uris = reader.readPrimitive<uint8_t>() == 1;
    uint32_t num_solns = reader.readPrimitive<uint32_t>();
    for (; num_solns > 0; --num_solns) {
      SolutionData sd;
      reader.convertPrimitive(sd.type_alias);
      reader.convertPrimitive(sd.time);
      sd.target = reader.readSizedUTF16();
      sd.data = reader.readSizedVector<uint8_t>();

      solutions.push_back(sd);
    }
  }
  //If we went out of range then the number of solutions is incorrect
  if (reader.outOfRange()) {
    return std::make_pair(false, vector<solver::SolutionData>());
  }
  return std::make_pair(create_uris, solutions);
}

/**
 * Solvers may also create new URIs in the world model.
 */
Buffer solver::makeCreateURI(const URI& new_uri, grail_time creation, std::u16string origin) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::create_uri, buff);

  //Push back the query string
  total_length += pushBackSizedUTF16(buff, new_uri);
  total_length += pushBackVal(creation, buff);
  total_length += pushBackUTF16(buff, origin);

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<URI, grail_time, std::u16string> solver::decodeCreateURI(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::create_uri == msg_type) {
    //Return the new URI and its origin
    u16string new_uri = reader.readSizedUTF16();
    grail_time created = reader.readPrimitive<grail_time>();
    u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);
    //If we went out of range then the URI is invalid
    if (reader.outOfRange()) {
      return std::make_tuple(u"", 0, u"");
    }
    return std::make_tuple(new_uri, created, origin);
  }

  //Return empty strings on failure.
  return std::make_tuple(u"", 0, u"");
}

Buffer solver::makeExpireURI(const URI& uri, grail_time expiration, std::u16string origin) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::expire_uri, buff);

  //Push back the URI, expiration time, and origin
  total_length += pushBackSizedUTF16(buff, uri);
  total_length += pushBackVal(expiration, buff);
  total_length += pushBackUTF16(buff, origin);

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<URI, grail_time, std::u16string> solver::decodeExpireURI(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::expire_uri == msg_type) {
    //Return the new URI and its origin
    u16string uri = reader.readSizedUTF16();
    grail_time expired = reader.readPrimitive<grail_time>();
    u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);
    //If we went out of range then the URI is invalid
    if (reader.outOfRange()) {
      return std::make_tuple(u"", 0, u"");
    }
    return std::make_tuple(uri, expired, origin);
  }

  //Return empty strings on failure.
  return std::make_tuple(u"", 0, u"");
}

Buffer solver::makeExpireAttribute(const URI& uri, std::u16string attribute, std::u16string origin, grail_time expiration) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::expire_attribute, buff);

  //Push back the URI, expiration time, and origin
  total_length += pushBackSizedUTF16(buff, uri);
  total_length += pushBackSizedUTF16(buff, attribute);
  total_length += pushBackVal(expiration, buff);
  total_length += pushBackUTF16(buff, origin);

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<URI, std::u16string, grail_time, std::u16string> solver::decodeExpireAttribute(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::expire_attribute == msg_type) {
    //Return the new URI and its origin
    u16string uri = reader.readSizedUTF16();
    u16string attribute = reader.readSizedUTF16();
    grail_time expired = reader.readPrimitive<grail_time>();
    u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);
    //If we went out of range then the URI is invalid
    if (reader.outOfRange()) {
      return std::make_tuple(u"", u"", 0, u"");
    }
    return std::make_tuple(uri, attribute, expired, origin);
  }

  //Return empty strings on failure.
  return std::make_tuple(u"", u"", 0, u"");
}

Buffer solver::makeDeleteURI(const URI& uri, std::u16string origin) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::delete_uri, buff);

  //Push back the query string
  total_length += pushBackSizedUTF16(buff, uri);
  total_length += pushBackUTF16(buff, origin);

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::pair<URI, std::u16string> solver::decodeDeleteURI(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::delete_uri == msg_type) {
    //Return the new URI and its origin
    u16string uri = reader.readSizedUTF16();
    u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);
    //If we went out of range then the URI is invalid
    if (reader.outOfRange()) {
      return std::make_pair(u"", u"");
    }
    return std::make_pair(uri, origin);
  }

  //Return empty strings on failure.
  return std::make_pair(u"", u"");
}

Buffer solver::makeDeleteAttribute(const URI& uri, std::u16string attribute, std::u16string origin) {
  Buffer buff;

  //Keep track of the total length of the message (in bytes)
  uint32_t total_length = 0;

  //Push back the total length and message type
  pushBackVal(total_length, buff);
  total_length += pushBackVal(MessageID::delete_attribute, buff);

  //Push back the query string
  total_length += pushBackSizedUTF16(buff, uri);
  total_length += pushBackSizedUTF16(buff, attribute);
  total_length += pushBackUTF16(buff, origin);

  pushBackVal(total_length, buff, 0);
  return buff;
}

std::tuple<URI, std::u16string, std::u16string> solver::decodeDeleteAttribute(Buffer& buff) {
  BuffReader reader(buff);

  uint32_t total_length = reader.readPrimitive<uint32_t>();
  MessageID msg_type = reader.readPrimitive<MessageID>();

  if (buff.size() == total_length + 4 and
      MessageID::delete_attribute == msg_type) {
    //Return the new URI and its origin
    u16string uri = reader.readSizedUTF16();
    u16string attribute = reader.readSizedUTF16();
    u16string origin = reader.readUTF16((buff.size() - reader.cur_index)/2);
    //If we went out of range then the strings are invalid
    if (reader.outOfRange()) {
      return std::make_tuple(u"", u"", u"");
    }
    return std::make_tuple(uri, attribute, origin);
  }

  //Return empty strings on failure.
  return std::make_tuple(u"", u"", u"");
}

