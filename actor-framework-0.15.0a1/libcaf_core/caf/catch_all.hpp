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

#ifndef CAF_CATCH_ALL_HPP
#define CAF_CATCH_ALL_HPP

#include <functional>
#include <type_traits>

namespace caf {

template <class F>
struct catch_all {
  using fun_type = std::function<result<message> (message_view&)>;

  F handler;

  catch_all(catch_all&&) = default;

  template <class T>
  catch_all(T&& x) : handler(std::forward<T>(x)) {
    // nop
  }

  static_assert(std::is_convertible<F, fun_type>::value,
                "catch-all handler must have signature "
                "result<message> (message_view&)");

  fun_type lift() const {
    return handler;
  }
};

template <class T>
struct is_catch_all : std::false_type {};

template <class T>
struct is_catch_all<catch_all<T>> : std::true_type {};

} // namespace caf

#endif // CAF_CATCH_ALL_HPP
