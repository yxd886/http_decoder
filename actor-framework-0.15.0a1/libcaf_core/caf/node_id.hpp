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

#ifndef CAF_NODE_ID_HPP
#define CAF_NODE_ID_HPP

#include <array>
#include <string>
#include <cstdint>
#include <functional>

#include "caf/fwd.hpp"
#include "caf/config.hpp"
#include "caf/ref_counted.hpp"
#include "caf/intrusive_ptr.hpp"

#include "caf/detail/comparable.hpp"

namespace caf {

/// Objects of this type identify an invalid `node_id`.
/// @relates node_id
struct invalid_node_id_t {
  constexpr invalid_node_id_t() {
    // nop
  }
};

/// Identifies an invalid `node_id`.
/// @relates node_id
constexpr invalid_node_id_t invalid_node_id = invalid_node_id_t{};

/// A node ID consists of a host ID and process ID. The host ID identifies
/// the physical machine in the network, whereas the process ID identifies
/// the running system-level process on that machine.
class node_id {
public:
  ~node_id();

  node_id() = default;

  node_id(const node_id&) = default;

  node_id(const invalid_node_id_t&);

  node_id& operator=(const node_id&) = default;

  node_id& operator=(const invalid_node_id_t&);

  /// A 160 bit hash (20 bytes).
  static constexpr size_t host_id_size = 20;

  /// The size of a `node_id` in serialized form.
  static constexpr size_t serialized_size = host_id_size + sizeof(uint32_t);

  /// Represents a 160 bit hash.
  using host_id_type = std::array<uint8_t, host_id_size>;

  /// Creates a node ID from `process_id` and `hash`.
  /// @param process_id System-wide unique process identifier.
  /// @param hash Unique node id as hexadecimal string representation.
  node_id(uint32_t process_id, const std::string& hash);

  /// Creates a node ID from `process_id` and `hash`.
  /// @param process_id System-wide unique process identifier.
  /// @param node_id Unique node id.
  node_id(uint32_t process_id, const host_id_type& node_id);

  /// Identifies the running process.
  /// @returns A system-wide unique process identifier.
  uint32_t process_id() const;

  /// Identifies the host system.
  /// @returns A hash build from the MAC address of the first network device
  ///      and the UUID of the root partition (mounted in "/" or "C:").
  const host_id_type& host_id() const;

  /// Queries whether this node is not default-constructed.
  explicit operator bool() const;

  void swap(node_id&);

  /// @cond PRIVATE

  // A reference counted container for host ID and process ID.
  class data : public ref_counted {
  public:
    // for singleton API
    void stop();

    // for singleton API
    inline void dispose() {
      deref();
    }

    // for singleton API
    inline void initialize() {
      // nop
    }

    static intrusive_ptr<data> create_singleton();

    int compare(const node_id& other) const;

    ~data();

    data();

    data(uint32_t procid, host_id_type hid);

    data(uint32_t procid, const std::string& hash);

    uint32_t pid_;

    host_id_type host_;
  };

  // "inherited" from comparable<node_id>
  int compare(const node_id& other) const;

  // "inherited" from comparable<node_id, invalid_node_id_t>
  int compare(const invalid_node_id_t&) const;

  explicit node_id(intrusive_ptr<data> dataptr);

  friend void serialize(serializer& sink, node_id& x, const unsigned int);

  friend void serialize(deserializer& source, node_id& x, const unsigned int);

  void from_string(const std::string& str);

  /// @endcond

private:
  intrusive_ptr<data> data_;
};

inline bool operator==(const node_id& lhs, const node_id& rhs) {
  return lhs.compare(rhs) == 0;
}

inline bool operator==(const node_id& lhs, invalid_node_id_t) {
  return ! static_cast<bool>(lhs);
}

inline bool operator!=(const node_id& lhs, const node_id& rhs) {
  return ! (lhs == rhs);
}

inline bool operator!=(const node_id& lhs, invalid_node_id_t) {
  return static_cast<bool>(lhs);
}

inline bool operator<(const node_id& lhs, const node_id& rhs) {
  return lhs.compare(rhs) < 0;
}

/// @relates node_id
std::string to_string(const node_id& x);

} // namespace caf

namespace std{

template<>
struct hash<caf::node_id> {
  size_t operator()(const caf::node_id& nid) const {
    if (nid == caf::invalid_node_id)
      return 0;
    // xor the first few bytes from the node ID and the process ID
    auto x = static_cast<size_t>(nid.process_id());
    auto y = *(reinterpret_cast<const size_t*>(&nid.host_id()));
    return x ^ y;
  }
};

} // namespace std

#endif // CAF_NODE_ID_HPP
