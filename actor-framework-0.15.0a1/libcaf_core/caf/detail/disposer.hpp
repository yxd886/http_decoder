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

#ifndef CAF_DETAIL_DISPOSER_HPP
#define CAF_DETAIL_DISPOSER_HPP

#include <type_traits>

#include "caf/memory_managed.hpp"

namespace caf {
namespace detail {

class disposer {
public:
  inline void operator()(memory_managed* ptr) const {
    ptr->request_deletion(false);
  }

  template <class T>
  typename std::enable_if<! std::is_base_of<memory_managed, T>::value>::type
  operator()(T* ptr) const {
    delete ptr;
  }
};

} // namespace detail
} // namespace caf

#endif // CAF_DETAIL_DISPOSER_HPP

