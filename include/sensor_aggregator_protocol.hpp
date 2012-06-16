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

#ifndef __SENSOR_AGGREGATOR_PROTOCOL_HPP__
#define __SENSOR_AGGREGATOR_PROTOCOL_HPP__

#include <array>
#include <vector>

#include "sample_data.hpp"

namespace sensor_aggregator {
  std::vector<unsigned char> makeHandshakeMsg();

  std::vector<unsigned char> makeSampleMsg(SampleData& sample);

  SampleData decodeSampleMsg(std::vector<unsigned char>& buff, unsigned int length);
}

#endif
