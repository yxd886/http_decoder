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

#ifndef CAF_UNIT_HPP
#define CAF_UNIT_HPP

#include <string>

#include "caf/detail/comparable.hpp"

namespace caf {

/// Unit is analogous to `void`, but can be safely returned, stored, etc.
/// to enable higher-order abstraction without cluttering code with
/// exceptions for `void` (which can't be stored, for example).
struct unit_t : detail::comparable<unit_t> {
  constexpr unit_t() {
    // nop
  }

  constexpr unit_t(const unit_t&) {
    // nop
  }

  template <class T>
  explicit constexpr unit_t(T&&) {
    // nop
  }

  static constexpr int compare(const unit_t&) {
    return 0;
  }
};

static constexpr unit_t unit = unit_t{};

/// @relates unit_t
template <class Processor>
void serialize(Processor&, const unit_t&, const unsigned int) {
  // nop
}

/// @relates unit_t
template <class T>
std::string to_string(const unit_t&) {
  return "<unit>";
}

template <class T>
struct lift_void {
  using type = T;
};

template <>
struct lift_void<void> {
  using type = unit_t;
};

template <class T>
struct unlift_void {
  using type = T;
};

template <>
struct unlift_void<unit_t> {
  using type = void;
};

} // namespace caf

#endif // CAF_UNIT_HPP
