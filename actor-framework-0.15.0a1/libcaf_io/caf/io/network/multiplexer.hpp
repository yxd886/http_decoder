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

#ifndef CAF_IO_NETWORK_MULTIPLEXER_HPP
#define CAF_IO_NETWORK_MULTIPLEXER_HPP

#include <string>
#include <thread>
#include <functional>

#include "caf/extend.hpp"
#include "caf/expected.hpp"
#include "caf/resumable.hpp"
#include "caf/make_counted.hpp"
#include "caf/execution_unit.hpp"

#include "caf/io/fwd.hpp"
#include "caf/io/accept_handle.hpp"
#include "caf/io/connection_handle.hpp"

#include "caf/io/network/protocol.hpp"
#include "caf/io/network/native_socket.hpp"

namespace boost {
namespace asio {
class io_service;
} // namespace asio
} // namespace boost

namespace caf {
namespace io {
namespace network {

/// Low-level backend for IO multiplexing.
class multiplexer : public execution_unit {
public:
  explicit multiplexer(actor_system* sys);

  /// Tries to connect to `host` on given `port` and returns an unbound
  /// connection handle on success.
  /// @threadsafe
  virtual expected<connection_handle>
  new_tcp_scribe(const std::string& host, uint16_t port) = 0;

  /// Assigns an unbound scribe identified by `hdl` to `ptr`.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual expected<void>
  assign_tcp_scribe(abstract_broker* ptr, connection_handle hdl) = 0;

  /// Creates a new TCP doorman from a native socket handle.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual connection_handle add_tcp_scribe(abstract_broker* ptr,
                                           native_socket fd) = 0;

  /// Tries to connect to `host` on `port` and returns a
  /// new scribe managing the connection on success.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual expected<connection_handle>
  add_tcp_scribe(abstract_broker*, const std::string& host, uint16_t port) = 0;

  /// Tries to create an unbound TCP doorman bound to `port`, optionally
  /// accepting only connections from IP address `in`.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual expected<std::pair<accept_handle, uint16_t>>
  new_tcp_doorman(uint16_t port, const char* in = nullptr,
                  bool reuse_addr = false) = 0;

  /// Assigns an unbound doorman identified by `hdl` to `ptr`.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual expected<void>
  assign_tcp_doorman(abstract_broker* ptr, accept_handle hdl) = 0;

  /// Creates a new TCP doorman from a native socket handle.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual accept_handle add_tcp_doorman(abstract_broker* ptr,
                                        native_socket fd) = 0;

  /// Tries to create a new TCP doorman running on port `p`, optionally
  /// accepting only connections from IP address `in`.
  /// @warning Do not call from outside the multiplexer's event loop.
  virtual expected<std::pair<accept_handle, uint16_t>>
  add_tcp_doorman(abstract_broker* ptr, uint16_t port, const char* in = nullptr,
                  bool reuse_addr = false) = 0;

  /// Simple wrapper for runnables
  class runnable : public resumable, public ref_counted {
  public:
    subtype_t subtype() const override;
    void intrusive_ptr_add_ref_impl() override;
    void intrusive_ptr_release_impl() override;
  };

  /// Makes sure the multipler does not exit its event loop until
  /// the destructor of `supervisor` has been called.
  class supervisor {
  public:
    virtual ~supervisor();
  };

  using supervisor_ptr = std::unique_ptr<supervisor>;

  /// Creates a supervisor to keep the event loop running.
  virtual supervisor_ptr make_supervisor() = 0;

  /// Creates an instance using the networking backend compiled with CAF.
  static std::unique_ptr<multiplexer> make(actor_system& sys);

  /// Runs the multiplexers event loop.
  virtual void run() = 0;

  /// Invokes @p fun in the multiplexer's event loop, calling `fun()`
  /// immediately when called from inside the event loop.
  /// @threadsafe
  template <class F>
  void dispatch(F fun) {
    if (std::this_thread::get_id() == thread_id()) {
      fun();
      return;
    }
    post(std::move(fun));
  }

  /// Invokes @p fun in the multiplexer's event loop, forcing
  /// execution to be delayed when called from inside the event loop.
  /// @threadsafe
  template <class F>
  void post(F fun) {
    struct impl : runnable {
      F f;
      impl(F&& mf) : f(std::move(mf)) { }
      resume_result resume(execution_unit*, size_t) override {
        f();
        return done;
      }
    };
    exec_later(new impl(std::move(fun)));
  }

  /// Retrieves a pointer to the implementation or `nullptr` if CAF was
  /// compiled using the default backend.
  virtual boost::asio::io_service* pimpl();

  inline const std::thread::id& thread_id() const {
    return tid_;
  }

  inline void thread_id(std::thread::id tid) {
    tid_ = std::move(tid);
  }

protected:
  /// Identifies the thread this multiplexer
  /// is running in. Must be set by the subclass.
  std::thread::id tid_;
};

using multiplexer_ptr = std::unique_ptr<multiplexer>;

} // namespace network
} // namespace io
} // namespace caf

#endif // CAF_IO_NETWORK_MULTIPLEXER_HPP
