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

#ifndef CAF_IO_BROKER_SERVANT_HPP
#define CAF_IO_BROKER_SERVANT_HPP

#include "caf/fwd.hpp"
#include "caf/mailbox_element.hpp"

#include "caf/io/abstract_broker.hpp"
#include "caf/io/system_messages.hpp"

namespace caf {
namespace io {

/// Base class for `scribe` and `doorman`.
/// @ingroup Broker
template <class Base, class Handle, class SysMsgType>
class broker_servant : public Base {
public:
  broker_servant(abstract_broker* ptr, Handle x)
      : Base(ptr),
        hdl_(x),
        value_(strong_actor_ptr{}, message_id::make(),
               mailbox_element::forwarding_stack{}, SysMsgType{}) {
    set_hdl(msg(), x);
  }

  Handle hdl() const {
    return hdl_;
  }

protected:
  void detach_from(abstract_broker* ptr) override {
    ptr->erase(hdl_);
  }

  void invoke_mailbox_element(execution_unit* ctx) {
    auto self = this->parent();
    auto pfac = self->proxy_registry_ptr();
    if (pfac)
      ctx->proxy_registry_ptr(pfac);
    auto guard = detail::make_scope_guard([=] {
      if (pfac)
        ctx->proxy_registry_ptr(nullptr);
    });
    self->activate(ctx, value_);
  }

  SysMsgType& msg() {
    return value_.template get_mutable_as<SysMsgType>(0);
  }

  static void set_hdl(new_connection_msg& lhs, Handle& hdl) {
    lhs.source = hdl;
  }

  static void set_hdl(new_data_msg& lhs, Handle& hdl) {
    lhs.handle = hdl;
  }

  Handle hdl_;
  mailbox_element_vals<SysMsgType> value_;
};

} // namespace io
} // namespace caf

#endif // CAF_IO_BROKER_SERVANT_HPP

