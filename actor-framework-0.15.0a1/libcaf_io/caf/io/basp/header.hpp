/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_IO_BASP_HEADER_HPP
#define CAF_IO_BASP_HEADER_HPP

#include <string>
#include <cstdint>

#include "caf/node_id.hpp"

#include "caf/io/basp/message_type.hpp"

namespace caf {
namespace io {
namespace basp {

/// @addtogroup BASP

/// The header of a Binary Actor System Protocol (BASP) message.
/// A BASP header consists of a routing part, i.e., source and
/// destination, as well as an operation and operation data. Several
/// message types consist of only a header.
struct header {
  message_type operation;
  uint8_t padding1;
  uint8_t padding2;
  uint8_t flags;
  uint32_t payload_len;
  uint64_t operation_data;
  node_id source_node;
  node_id dest_node;
  actor_id source_actor;
  actor_id dest_actor;

  inline header(message_type m_operation, uint8_t m_flags,
                uint32_t m_payload_len, uint64_t m_operation_data,
                node_id m_source_node, node_id m_dest_node,
                actor_id m_source_actor, actor_id m_dest_actor)
      : operation(m_operation),
        flags(m_flags),
        payload_len(m_payload_len),
        operation_data(m_operation_data),
        source_node(std::move(m_source_node)),
        dest_node(std::move(m_dest_node)),
        source_actor(m_source_actor),
        dest_actor(m_dest_actor) {
    // nop
  }

  header() = default;

  /// Identifies a receiver by name rather than ID.
  static const uint8_t named_receiver_flag = 0x01;

  /// Queries whether this header has the given flag.
  inline bool has(uint8_t flag) const {
    return (flags & flag) != 0;
  }
};

/// @relates header
template <class Processor>
void serialize(Processor& proc, header& hdr, const unsigned int) {
  uint8_t pad = 0;
  proc & hdr.operation;
  proc & pad;
  proc & pad;
  proc & hdr.flags;
  proc & hdr.payload_len;
  proc & hdr.operation_data;
  proc & hdr.source_node;
  proc & hdr.dest_node;
  proc & hdr.source_actor;
  proc & hdr.dest_actor;
}

/// @relates header
std::string to_string(const header& hdr);

/// @relates header
bool operator==(const header& lhs, const header& rhs);

/// @relates header
inline bool operator!=(const header& lhs, const header& rhs) {
  return !(lhs == rhs);
}

/// Checks whether given header contains a handshake.
inline bool is_handshake(const header& hdr) {
  return hdr.operation == message_type::server_handshake
      || hdr.operation == message_type::client_handshake;
}

/// Checks wheter given header contains a heartbeat.
inline bool is_heartbeat(const header& hdr) {
  return hdr.operation == message_type::heartbeat;
}

/// Checks whether given BASP header is valid.
/// @relates header
bool valid(const header& hdr);

/// Size of a BASP header in serialized form
constexpr size_t header_size = node_id::serialized_size * 2
                               + sizeof(actor_id) * 2
                               + sizeof(uint32_t) * 2
                               + sizeof(uint64_t);

/// @}

} // namespace basp
} // namespace io
} // namespace caf

#endif // CAF_IO_BASP_HEADER_HPP
