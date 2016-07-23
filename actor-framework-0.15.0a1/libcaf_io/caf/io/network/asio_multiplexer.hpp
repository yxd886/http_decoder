/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 * Raphael Hiesgen <raphael.hiesgen (at) haw-hamburg.de>                      *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_IO_NETWORK_ASIO_MULTIPLEXER_HPP
#define CAF_IO_NETWORK_ASIO_MULTIPLEXER_HPP

#include "caf/config.hpp"

CAF_PUSH_WARNINGS
#include "boost/asio.hpp"
CAF_POP_WARNINGS

#include "caf/logger.hpp"

#include "caf/io/receive_policy.hpp"

#include "caf/io/network/multiplexer.hpp"
#include "caf/io/network/native_socket.hpp"
#include "caf/io/network/stream_manager.hpp"
#include "caf/io/network/acceptor_manager.hpp"

namespace caf {
namespace io {
namespace network {

/// Low-level backend for IO multiplexing.
using io_service = boost::asio::io_service;

/// Low-level socket type used as default.
using asio_tcp_socket = boost::asio::ip::tcp::socket;

/// Low-level socket type used as default.
using asio_tcp_socket_acceptor = boost::asio::ip::tcp::acceptor;

/// A wrapper for the boost::asio multiplexer
class asio_multiplexer : public multiplexer {
public:
  friend class io::middleman;
  friend class supervisor;

  expected<connection_handle>
  new_tcp_scribe(const std::string&, uint16_t) override;

  expected<void>
  assign_tcp_scribe(abstract_broker*, connection_handle hdl) override;

  template <class Socket>
  connection_handle add_tcp_scribe(abstract_broker*, Socket&& sock);

  connection_handle add_tcp_scribe(abstract_broker*, native_socket fd) override;

  expected<connection_handle>
  add_tcp_scribe(abstract_broker*, const std::string&, uint16_t) override;

  expected<std::pair<accept_handle, uint16_t>>
  new_tcp_doorman(uint16_t p, const char* in, bool rflag) override;

  expected<void>
  assign_tcp_doorman(abstract_broker*, accept_handle hdl) override;

  accept_handle add_tcp_doorman(abstract_broker*,
                                asio_tcp_socket_acceptor&& sock);

  accept_handle add_tcp_doorman(abstract_broker*, native_socket fd) override;

  expected<std::pair<accept_handle, uint16_t>>
  add_tcp_doorman(abstract_broker*, uint16_t, const char*, bool) override;

  void exec_later(resumable* ptr) override;

  asio_multiplexer(actor_system* sys);

  ~asio_multiplexer();

  supervisor_ptr make_supervisor() override;

  void run() override;

  boost::asio::io_service* pimpl() override;

  inline boost::asio::io_service& service() {
    return service_;
  }

private:
  io_service service_;
  std::mutex mtx_sockets_;
  std::mutex mtx_acceptors_;
  std::map<int64_t, asio_tcp_socket> unassigned_sockets_;
  std::map<int64_t, asio_tcp_socket_acceptor> unassigned_acceptors_;
};

template <class T>
connection_handle conn_hdl_from_socket(T& sock) {
  return connection_handle::from_int(
    int64_from_native_socket(sock.native_handle()));
}

template <class T>
accept_handle accept_hdl_from_socket(T& sock) {
  return accept_handle::from_int(
    int64_from_native_socket(sock.native_handle()));
}

/// @relates manager
using manager_ptr = intrusive_ptr<manager>;

/// A stream capable of both reading and writing. The stream's input
/// data is forwarded to its {@link stream_manager manager}.
template <class Socket>
class asio_stream {
public:
  /// A smart pointer to a stream manager.
  using manager_ptr = intrusive_ptr<stream_manager>;

  /// A buffer class providing a compatible interface to `std::vector`.
  using buffer_type = std::vector<char>;

  asio_stream(asio_multiplexer& ref)
      : writing_(false),
        ack_writes_(false),
        fd_(ref.service()),
        backend_(ref) {
    configure_read(receive_policy::at_most(1024));
  }

  /// Returns the IO socket.
  Socket& socket_handle() {
    return fd_;
  }

  /// Returns the IO socket.
  const Socket& socket_handle() const {
    return fd_;
  }

  /// Initializes this stream, setting the socket handle to `fd`.
  void init(Socket fd) {
    fd_ = std::move(fd);
  }

  /// Starts reading data from the socket, forwarding incoming data to `mgr`.
  void start(const manager_ptr& mgr) {
    CAF_ASSERT(mgr != nullptr);
    read_loop(mgr);
  }

  /// Configures how much data will be provided for the next `consume` callback.
  /// @warning Must not be called outside the IO multiplexers event loop
  ///          once the stream has been started.
  void configure_read(receive_policy::config config) {
    rd_flag_ = config.first;
    rd_size_ = config.second;
  }

  void ack_writes(bool enable) {
    CAF_LOG_TRACE(CAF_ARG(enable));
    ack_writes_ = enable;
  }

  /// Copies data to the write buffer.
  /// @note Not thread safe.
  void write(const void* buf, size_t num_bytes) {
    CAF_LOG_TRACE(CAF_ARG(num_bytes));
    auto first = reinterpret_cast<const char*>(buf);
    auto last = first + num_bytes;
    wr_offline_buf_.insert(wr_offline_buf_.end(), first, last);
  }

  /// Returns the write buffer of this stream.
  /// @warning Must not be modified outside the IO multiplexers event loop
  ///          once the stream has been started.
  buffer_type& wr_buf() {
    return wr_offline_buf_;
  }

  buffer_type& rd_buf() {
    return rd_buf_;
  }

  /// Sends the content of the write buffer, calling the `io_failure`
  /// member function of `mgr` in case of an error.
  /// @warning Must not be called outside the IO multiplexers event loop
  ///          once the stream has been started.
  void flush(const manager_ptr& mgr) {
    CAF_ASSERT(mgr != nullptr);
    if (! wr_offline_buf_.empty() && ! writing_) {
      writing_ = true;
      write_loop(mgr);
    }
  }

  /// Closes the network connection, thus stopping this stream.
  void stop() {
    CAF_LOG_TRACE("");
    fd_.close();
  }

  void stop_reading() {
    CAF_LOG_TRACE("");
    boost::system::error_code ec; // ignored
    fd_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
  }

  asio_multiplexer& backend() {
    return backend_;
  }

private:
  void read_loop(const manager_ptr& mgr) {
    auto cb = [=](const boost::system::error_code& ec, size_t read_bytes) {
      CAF_LOG_TRACE("");
      if (! ec) {
        mgr->consume(&backend(), rd_buf_.data(), read_bytes);
        read_loop(mgr);
      } else {
        mgr->io_failure(&backend(), operation::read);
      }
    };
    switch (rd_flag_) {
      case receive_policy_flag::exactly:
        if (rd_buf_.size() < rd_size_) {
          rd_buf_.resize(rd_size_);
        }
        boost::asio::async_read(fd_, boost::asio::buffer(rd_buf_, rd_size_),
                                cb);
        break;
      case receive_policy_flag::at_most:
        if (rd_buf_.size() < rd_size_) {
          rd_buf_.resize(rd_size_);
        }
        fd_.async_read_some(boost::asio::buffer(rd_buf_, rd_size_), cb);
        break;
      case receive_policy_flag::at_least: {
        // read up to 10% more, but at least allow 100 bytes more
        auto min_size = rd_size_ + std::max<size_t>(100, rd_size_ / 10);
        if (rd_buf_.size() < min_size) {
          rd_buf_.resize(min_size);
        }
        collect_data(mgr, 0);
        break;
      }
    }
  }

  void write_loop(const manager_ptr& mgr) {
    if (wr_offline_buf_.empty()) {
      writing_ = false;
      return;
    }
    wr_buf_.clear();
    wr_buf_.swap(wr_offline_buf_);
    boost::asio::async_write(
      fd_, boost::asio::buffer(wr_buf_),
      [=](const boost::system::error_code& ec, size_t nb) {
        CAF_LOG_TRACE("");
        if (ec) {
          CAF_LOG_DEBUG(CAF_ARG(ec.message()));
          mgr->io_failure(&backend(), operation::read);
          writing_ = false;
          return;
        }
        CAF_LOG_DEBUG(CAF_ARG(nb));
        if (ack_writes_)
          mgr->data_transferred(&backend(), nb, wr_offline_buf_.size());
        write_loop(mgr);
      });
  }

  void collect_data(const manager_ptr& mgr, size_t collected_bytes) {
    fd_.async_read_some(boost::asio::buffer(rd_buf_.data() + collected_bytes,
                                            rd_buf_.size() - collected_bytes),
                        [=](const boost::system::error_code& ec, size_t nb) {
      CAF_LOG_TRACE(CAF_ARG(nb));
      if (! ec) {
        auto sum = collected_bytes + nb;
        if (sum >= rd_size_) {
          mgr->consume(&backend(), rd_buf_.data(), sum);
          read_loop(mgr);
        } else {
          collect_data(mgr, sum);
        }
      } else {
        mgr->io_failure(&backend(), operation::write);
      }
    });
  }

  bool writing_;
  bool ack_writes_;
  Socket fd_;
  receive_policy_flag rd_flag_;
  size_t rd_size_;
  buffer_type rd_buf_;
  buffer_type wr_buf_;
  buffer_type wr_offline_buf_;
  asio_multiplexer& backend_;
};

/// An acceptor is responsible for accepting incoming connections.
template <class SocketAcceptor>
class asio_acceptor {
  using protocol_type = typename SocketAcceptor::protocol_type;
  using socket_type = boost::asio::basic_stream_socket<protocol_type>;

public:
  /// A manager providing the `accept` member function.
  using manager_type = acceptor_manager;

  /// A smart pointer to an acceptor manager.
  using manager_ptr = intrusive_ptr<manager_type>;

  asio_acceptor(asio_multiplexer& am, io_service& io)
      : backend_(am),
        accept_fd_(io),
        fd_(io) {
    // nop
  }

  /// Returns the `multiplexer` this acceptor belongs to.
  asio_multiplexer& backend() { return backend_; }

  /// Returns the IO socket.
  SocketAcceptor& socket_handle() {
    return accept_fd_;
  }

  /// Returns the IO socket.
  const SocketAcceptor& socket_handle() const {
    return accept_fd_;
  }

  /// Returns the accepted socket. This member function should
  ///        be called only from the `new_connection` callback.
  inline socket_type& accepted_socket() {
    return fd_;
  }

  /// Initializes this acceptor, setting the socket handle to `fd`.
  void init(SocketAcceptor fd) {
    accept_fd_ = std::move(fd);
  }

  /// Starts this acceptor, forwarding all incoming connections to
  /// `manager`. The intrusive pointer will be released after the
  /// acceptor has been closed or an IO error occured.
  void start(const manager_ptr& mgr) {
    accept_loop(mgr);
  }

  /// Closes the network connection, thus stopping this acceptor.
  void stop() {
    accept_fd_.close();
  }

private:
  void accept_loop(const manager_ptr& mgr) {
    accept_fd_.async_accept(fd_, [=](const boost::system::error_code& ec) {
      CAF_LOG_TRACE("");
      if (! ec) {
        mgr->new_connection(); // probably moves fd_
        // reset fd_ for next accept operation
        fd_ = socket_type{accept_fd_.get_io_service()};
        accept_loop(mgr);
      } else {
        mgr->io_failure(&backend(), operation::read);
      }
    });
  }

  asio_multiplexer& backend_;
  SocketAcceptor accept_fd_;
  socket_type fd_;
};

} // namesapce network
} // namespace io
} // namespace caf

#endif // CAF_IO_NETWORK_ASIO_MULTIPLEXER_HPP
