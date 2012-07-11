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
 * @file temporarily_unavailable.hpp
 * Exceptions thrown by simple sockets.
 *
 * @author Bernhard Firner
 */

#ifndef __TEMPORARILY_UNAVAILABLE_HPP__
#define __TEMPORARILY_UNAVAILABLE_HPP__

#include <exception>


/**
 * Exception thrown when the socket is temporarily unavailable after polling
 * for ready to send.
 */
class temporarily_unavailable : public std::runtime_error {
  public:
    temporarily_unavailable() : std::runtime_error("Error sending data over socket: Resource temporarily unavailable") {};
};

#endif

