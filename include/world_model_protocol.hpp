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
 * This file contains functions and definitions for the
 * interface between solvers and the world model and between
 * the world model and clients in the application layer.
 */


#ifndef __WORLD_MODEL_PROTOCOL_HPP__
#define __WORLD_MODEL_PROTOCOL_HPP__

#include <cstdint>
#include <climits>

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace world_model {

  //Special characters besides the period (.) are not permitted in URIs.
  typedef std::u16string URI;
  typedef std::vector<uint8_t> Buffer;

  //The type that grail uses for time values.
  typedef int64_t grail_time;
  const grail_time MAX_GRAIL_TIME = INT64_MAX;

  //Each object URI has a set of attributes associated with it
  struct Attribute {
    //The description of this attribute. The attribute name indicates the data type.
    std::u16string name;
    //The creation date and the date when this attribute's data expires.
    grail_time creation_date;
    //The time when this attribute was expired. Equal to 0 if it is not expired.
    grail_time expiration_date;
    std::u16string origin;
    //Data in the field.
    Buffer data;
  };

  //Each object URI has a set of attributes associated with it
  struct AliasedAttribute {
    //The alias for the description of this attribute.
    //The attribute name indicates the data type.
    uint32_t name_alias;
    //The creation date and the date when this attribute's data expires.
    grail_time creation_date;
    grail_time expiration_date;
    uint32_t origin_alias;
    //Data in the field.
    Buffer data;
  };

  typedef std::map<URI, std::vector<Attribute>> WorldState;

  struct WorldData {
    //The URI of an object in the world model.
    URI object_uri;
    //Attributes of this URI
    std::vector<Attribute> attributes;
  };

  struct AliasedWorldData {
    //The URI of an object in the world model.
    URI object_uri;
    //Attributes of this URI
    std::vector<AliasedAttribute> attributes;
  };

  grail_time getGRAILTime();

  namespace client {
    enum class MessageID : uint8_t {keep_alive        = 0,
                                    snapshot_request  = 1,
                                    range_request     = 2,
                                    stream_request    = 3,
                                    attribute_alias   = 4,
                                    origin_alias      = 5,
                                    request_complete  = 6,
                                    cancel_request    = 7,
                                    data_response     = 8,
                                    uri_search        = 9,
                                    uri_response      = 10,
                                    origin_preference = 11};

    //It is a waste of bandwidth to repeatedly send large strings over the
    //network so some strings are aliased with a number.
    struct AliasType {
      //The alias used to refer to this type.
      uint32_t alias;
      std::u16string type;
    };

    struct Request {
      URI object_uri;
      std::vector<URI> attributes;
      //Start time of the request
      grail_time start;
      //The stop time or time period
      grail_time stop_period;
    };

    /**
     * Make a handshake message.
     */
    std::vector<unsigned char> makeHandshakeMsg();

    /*
     * Make a keep alive message to test if a connection should be kept active.
     */
    Buffer makeKeepAlive();

    /**
     * There are three ways to query the world model.
     * First, a client can request a snapshot of the world model state.
     * This returns the state of the world model after all of the results
     * in a given time period. The world model server will return 1 URI
     * at a time and will finish by sending a request_complete message.
     *
     * Second, the client can request all data with creation date's in a
     * time range. The world model server will return 1 URI and one attribute
     * at a time and will finish by sending a request_complete message.
     * Changes will be returned in creation time order.
     *
     * Third, the client can request a data stream that forwards new
     * world model data as it arrives. An update interval can be specified
     * to limit updates to a specified time interval.
     *
     * Tickets are used to identify requests in the response messages and
     * to cancel requests.
     *
     * TODO FIXME There should be two kinds of ranges, one with every
     * message in the range and one with holes when all requested attributes
     * are not present and unexpired.
     */
    Buffer makeSnapshotRequest(const Request& request, uint32_t ticket);
    Buffer makeRangeRequest(const Request& request, uint32_t ticket);
    Buffer makeStreamRequest(const Request& request, uint32_t ticket);

    std::tuple<client::Request, uint32_t> decodeSnapshotRequest(Buffer& buff);
    std::tuple<client::Request, uint32_t> decodeRangeRequest(Buffer& buff);
    std::tuple<client::Request, uint32_t> decodeStreamRequest(Buffer& buff);

    /**
     * Each attribute has a UTF16 big endian name and alias.
     * Sending these repeatedly would be a waste of bandwidth so
     * aliases will be used to refer to the strings. Any time the
     * world model would send an attribute with a name or origin
     * that is previously unseen by a client an attribute alias
     * or origin alias message will be sent to define aliases for
     * the new attribute names and origins.
     */
    Buffer makeAttrAliasMsg(const std::vector<AliasType>& attribute_aliases);
    Buffer makeOriginAliasMsg(const std::vector<AliasType>& origin_aliases);

    std::vector<AliasType> decodeAttrAliasMsg(Buffer& buff);
    std::vector<AliasType> decodeOriginAliasMsg(Buffer& buff);

    /**
     * After sending all of the data for a snapshot request or range request
     * the world model will send a request complete message to indicate
     * that no more data will be sent for the request.
     */
    Buffer makeRequestComplete(uint32_t ticket_number);
    uint32_t decodeRequestComplete(Buffer& buff);

    /**
     * The cancel request message cancels an ongoing streaming request.
     * After cancellation a request complete message is sent.
     */
    Buffer makeCancelRequest(uint32_t ticket_number);
    uint32_t decodeCancelRequest(Buffer& buff);

    /**
     * Data for any request is sent in the same format.
     */
    Buffer makeDataMessage(const AliasedWorldData& wd, uint32_t ticket);
    std::tuple<AliasedWorldData, uint32_t> decodeDataMessage(Buffer& buff);

    /**
     * Search for any URIs matching a regular expression string.
     */
    Buffer makeURISearch(const URI& uri);
    URI decodeURISearch(Buffer& buff);

    Buffer makeURISearchResponse(const std::vector<URI>& uris);
    std::vector<URI> decodeURISearchResponse(Buffer& buff);

    /**
     * A message used by a client to specify preferences for
     * different origins. This message uses origin strings rather
     * than aliases to allow clients to specify origins before
     * any data has been requested or received. Each origin name
     * is accompanied by an integer value representing the user's
     * preference for that origin with higher values representing
     * higher preference. If an attribute is available from multiple
     * origins only those values from the highest valued group of
     * origins are returned. For example, if location is available
     * from origin's A, B, and C with respective desirabilities of
     * 1, 1, and 0 then the location values from both A and B are
     * returned because one is not more desired than the other.
     * If only A and C are available then only A's data will be
     * returned. If only C is available then C's data will be returned.
     * 1 is the default preference value. If an origin has
     * a preference value less than zero then its data will never
     * be returned.
     * These preferences only apply to streaming and snapshot
     * requests. Range requests return all matching information,
     * regardless of origin.
     */
    Buffer makeOriginPreference(const std::vector<std::pair<std::u16string, int32_t>>& weights);
    std::vector<std::pair<std::u16string, int32_t>> decodeOriginPreference(Buffer& buff);

  }//End namespace world_model::client

  namespace solver {
    enum class MessageID : uint8_t {keep_alive       = 0,
                                    type_announce    = 1,
                                    start_on_demand  = 2,
                                    stop_on_demand   = 3,
                                    solver_data      = 4,
                                    create_uri       = 5,
                                    expire_uri       = 6,
                                    delete_uri       = 7,
                                    expire_attribute = 8,
                                    delete_attribute = 9 };

    //Solver with data and target data. The alias specifies the data format.
    struct SolutionData {
      //Alias that specifies the solution's name
      uint32_t type_alias;
      //The time when this attribute data was created
      grail_time time;
      //The URI to modify
      URI target;
      //Raw data
      std::vector<uint8_t> data;
    };

    /**
     * The AliasType of the solver->world model protocol is different than
     * in the client->world model because it keeps track of on_demand information.
     * A on_demand type is not stored in the world model and data is only sent
     * from the solver to the world model if that data type was requested
     * by a client.
     */
    struct AliasType {
      //The alias used to refer to this type.
      uint32_t alias;
      std::u16string type;
      bool on_demand;
    };

    /**
     * Make a handshake message.
     */
    std::vector<unsigned char> makeHandshakeMsg();

    /*
     * Make a keep alive message to test if a connection should be kept active.
     */
    Buffer makeKeepAlive();

    /**
     * Before a solver submits new kinds of data to the world model it must first
     * identify itself and the types of data it can create.
     *
     * TODO FIXME Should have an autoexpire field in the type announce message so
     * that solvers can set values to autoexpire if they are not updated within
     * some time frame
     */
    Buffer makeTypeAnnounceMsg(const std::vector<AliasType>& type_alias, const std::u16string& origin);
    std::pair<std::vector<AliasType>, std::u16string> decodeTypeAnnounceMsg(Buffer& buff);

    Buffer makeStartOnDemand(const std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>& aliases);
    std::vector<std::tuple<uint32_t, std::vector<std::u16string>>> decodeStartOnDemand(Buffer& buff);

    Buffer makeStopOnDemand(const std::vector<std::tuple<uint32_t, std::vector<std::u16string>>>& aliases);
    std::vector<std::tuple<uint32_t, std::vector<std::u16string>>> decodeStopOnDemand(Buffer& buff);

    /**
     * Data sent from the solver to modify an attribute in the world model.
     * The world model uses the previously announced origin and type aliases
     * when decoding this message.
     */
    Buffer makeSolutionMsg(bool create_uris, const std::vector<SolutionData>& solutions);
    std::tuple<bool, std::vector<SolutionData>> decodeSolutionMsg(Buffer& buff);

    /**
     * Solvers may also create new URIs in the world model.
     */
    Buffer makeCreateURI(const URI& new_uri, grail_time creation, std::u16string origin);
    std::tuple<URI, grail_time, std::u16string> decodeCreateURI(Buffer& buff);

    /**
     * Expiring a URI or attribute will set the expiration time of the URI.
     * Once a URI or attribute is expired it is not in the world model at the
     * current time.
     */
    Buffer makeExpireURI(const URI& uri, grail_time expiration, std::u16string origin);
    std::tuple<URI, grail_time, std::u16string> decodeExpireURI(Buffer& buff);

    Buffer makeExpireAttribute(const URI& uri, std::u16string attribute, std::u16string origin, grail_time expiration);
    std::tuple<URI, std::u16string, grail_time, std::u16string> decodeExpireAttribute(Buffer& buff);

    /**
     * Deleting a URI or attribute will remove it from the world model
     * for any time period.
     */
    Buffer makeDeleteURI(const URI& uri, std::u16string origin);
    std::pair<URI, std::u16string> decodeDeleteURI(Buffer& buff);

    Buffer makeDeleteAttribute(const URI& uri, std::u16string attribute, std::u16string origin);
    std::tuple<URI, std::u16string, std::u16string> decodeDeleteAttribute(Buffer& buff);

  }//End namespace world_model::solver
}//End namespace world_model

#endif

