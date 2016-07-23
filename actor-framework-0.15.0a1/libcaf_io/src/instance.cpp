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

#include "caf/io/basp/instance.hpp"

#include "caf/streambuf.hpp"
#include "caf/binary_serializer.hpp"
#include "caf/binary_deserializer.hpp"
#include "caf/actor_system_config.hpp"

#include "caf/io/basp/version.hpp"

namespace caf {
namespace io {
namespace basp {

instance::callee::callee(actor_system& sys, proxy_registry::backend& backend)
    : namespace_(sys, backend) {
  // nop
}

instance::callee::~callee() {
  // nop
}

instance::instance(abstract_broker* parent, callee& lstnr)
    : tbl_(parent),
      this_node_(parent->system().node()),
      callee_(lstnr) {
  CAF_ASSERT(this_node_ != invalid_node_id);
}

connection_state instance::handle(execution_unit* ctx,
                                  new_data_msg& dm, header& hdr,
                                  bool is_payload) {
  CAF_LOG_TRACE(CAF_ARG(dm) << CAF_ARG(is_payload));
  // function object providing cleanup code on errors
  auto err = [&]() -> connection_state {
    auto cb = make_callback([&](const node_id& nid){
      callee_.purge_state(nid);
    });
    tbl_.erase_direct(dm.handle, cb);
    return close_connection;
  };
  std::vector<char>* payload = nullptr;
  if (is_payload) {
    payload = &dm.buf;
    if (payload->size() != hdr.payload_len) {
      CAF_LOG_WARNING("received invalid payload");
      return err();
    }
  } else {
    binary_deserializer bd{ctx, dm.buf};
    bd >> hdr;
    if (! valid(hdr)) {
      CAF_LOG_WARNING("received invalid header:" << CAF_ARG(hdr));
      return err();
    }
    if (hdr.payload_len > 0) {
      CAF_LOG_DEBUG("await payload before processing further");
      return await_payload;
    }
  }
  CAF_LOG_DEBUG(CAF_ARG(hdr));
  // needs forwarding?
  if (!is_handshake(hdr) && !is_heartbeat(hdr) && hdr.dest_node != this_node_) {
    CAF_LOG_DEBUG("forward message");
    auto path = lookup(hdr.dest_node);
    if (path) {
      binary_serializer bs{ctx, path->wr_buf};
      bs << hdr;
      if (payload)
        bs.apply_raw(payload->size(), payload->data());
      tbl_.flush(*path);
      notify<hook::message_forwarded>(hdr, payload);
    } else {
      CAF_LOG_INFO("cannot forward message, no route to destination");
      if (hdr.source_node != this_node_) {
        // TODO: signalize error back to sending node
        auto reverse_path = lookup(hdr.source_node);
        if (! reverse_path) {
          CAF_LOG_WARNING("cannot send error message: no route to source");
        } else {
          CAF_LOG_WARNING("not implemented yet: signalize forward failure");
        }
      } else {
        CAF_LOG_WARNING("lost packet with probably spoofed source");
      }
      notify<hook::message_forwarding_failed>(hdr, payload);
    }
    return await_header;
  }
  // function object for checking payload validity
  auto payload_valid = [&]() -> bool {
    return payload != nullptr && payload->size() == hdr.payload_len;
  };
  // handle message to ourselves
  switch (hdr.operation) {
    case message_type::server_handshake: {
      actor_id aid = invalid_actor_id;
      std::set<std::string> sigs;
      if (payload_valid()) {
        binary_deserializer bd{ctx, *payload};
        std::string remote_appid;
        bd >> remote_appid;
        if (remote_appid != callee_.system().config().middleman_app_identifier) {
          CAF_LOG_ERROR("app identifier mismatch");
          return err();
        }
        bd >> aid >> sigs;
      } else {
        CAF_LOG_ERROR("fail to receive the app identifier");
        return err();
      }
      // close self connection after handshake is done
      if (hdr.source_node == this_node_) {
        CAF_LOG_INFO("close connection to self immediately");
        callee_.finalize_handshake(hdr.source_node, aid, sigs);
        return err();
      }
      // close this connection if we already have a direct connection
      if (tbl_.lookup_direct(hdr.source_node) != invalid_connection_handle) {
        CAF_LOG_INFO("close connection since we already have a "
                     "direct connection: " << CAF_ARG(hdr.source_node));
        callee_.finalize_handshake(hdr.source_node, aid, sigs);
        return err();
      }
      // add direct route to this node and remove any indirect entry
      CAF_LOG_INFO("new direct connection:" << CAF_ARG(hdr.source_node));
      tbl_.add_direct(dm.handle, hdr.source_node);
      auto was_indirect = tbl_.erase_indirect(hdr.source_node);
      // write handshake as client in response
      auto path = tbl_.lookup(hdr.source_node);
      if (!path) {
        CAF_LOG_ERROR("no route to host after server handshake");
        return err();
      }
      write_client_handshake(ctx, path->wr_buf, hdr.source_node);
      callee_.learned_new_node_directly(hdr.source_node, was_indirect);
      callee_.finalize_handshake(hdr.source_node, aid, sigs);
      flush(*path);
      break;
    }
    case message_type::client_handshake: {
      if (tbl_.lookup_direct(hdr.source_node) != invalid_connection_handle) {
        CAF_LOG_INFO("received second client handshake:"
                     << CAF_ARG(hdr.source_node));
        break;
      }
      if (payload_valid()) {
        binary_deserializer bd{ctx, *payload};
        std::string remote_appid;
        bd >> remote_appid;
        if (remote_appid != callee_.system().config().middleman_app_identifier) {
          CAF_LOG_ERROR("app identifier mismatch");
          return err();
        }
      } else {
        CAF_LOG_ERROR("fail to receive the app identifier");
        return err();
      }
      // add direct route to this node and remove any indirect entry
      CAF_LOG_INFO("new direct connection:" << CAF_ARG(hdr.source_node));
      tbl_.add_direct(dm.handle, hdr.source_node);
      auto was_indirect = tbl_.erase_indirect(hdr.source_node);
      callee_.learned_new_node_directly(hdr.source_node, was_indirect);
      break;
    }
    case message_type::dispatch_message: {
      if (! payload_valid())
        return err();
      // in case the sender of this message was received via a third node,
      // we assume that that node to offers a route to the original source
      auto last_hop = tbl_.lookup_direct(dm.handle);
      if (hdr.source_node != invalid_node_id
          && hdr.source_node != this_node_
          && last_hop != hdr.source_node
          && tbl_.lookup_direct(hdr.source_node) == invalid_connection_handle
          && tbl_.add_indirect(last_hop, hdr.source_node))
        callee_.learned_new_node_indirectly(hdr.source_node);
      binary_deserializer bd{ctx, *payload};
      auto receiver_name = static_cast<atom_value>(0);
      std::vector<strong_actor_ptr> forwarding_stack;
      message msg;
      if (hdr.has(header::named_receiver_flag))
        bd >> receiver_name;
      bd >> forwarding_stack >> msg;
      CAF_LOG_DEBUG(CAF_ARG(forwarding_stack) << CAF_ARG(msg));
      if (hdr.has(header::named_receiver_flag))
        callee_.deliver(hdr.source_node, hdr.source_actor, receiver_name,
                        message_id::from_integer_value(hdr.operation_data),
                        forwarding_stack, msg);
      else
        callee_.deliver(hdr.source_node, hdr.source_actor, hdr.dest_actor,
                        message_id::from_integer_value(hdr.operation_data),
                        forwarding_stack, msg);
      break;
    }
    case message_type::announce_proxy:
      callee_.proxy_announced(hdr.source_node, hdr.dest_actor);
      break;
    case message_type::kill_proxy: {
      if (! payload_valid())
        return err();
      binary_deserializer bd{ctx, *payload};
      error fail_state;
      bd >> fail_state;
      callee_.kill_proxy(hdr.source_node, hdr.source_actor, fail_state);
      break;
    }
    case message_type::heartbeat: {
      CAF_LOG_TRACE("received heartbeat: " << CAF_ARG(hdr.source_node));
      callee_.handle_heartbeat(hdr.source_node);
      break;
    }
    default:
      CAF_LOG_ERROR("invalid operation");
      return err();
  }
  return await_header;
}

void instance::handle_heartbeat(execution_unit* ctx) {
  CAF_LOG_TRACE("");
  for (auto& kvp: tbl_.direct_by_hdl_) {
    CAF_LOG_TRACE(CAF_ARG(kvp.first) << CAF_ARG(kvp.second));
    write_heartbeat(ctx, tbl_.parent_->wr_buf(kvp.first), kvp.second);
    tbl_.parent_->flush(kvp.first);
  }
}

void instance::handle_node_shutdown(const node_id& affected_node) {
  CAF_LOG_TRACE(CAF_ARG(affected_node));
  if (affected_node == invalid_node_id)
    return;
  CAF_LOG_INFO("lost direct connection:" << CAF_ARG(affected_node));
  auto cb = make_callback([&](const node_id& nid){
    callee_.purge_state(nid);
  });
  tbl_.erase(affected_node, cb);
}

optional<routing_table::route> instance::lookup(const node_id& target) {
  return tbl_.lookup(target);
}

void instance::flush(const routing_table::route& path) {
  tbl_.flush(path);
}

void instance::write(execution_unit* ctx, const routing_table::route& r,
                     header& hdr, payload_writer* writer) {
  CAF_LOG_TRACE(CAF_ARG(hdr));
  CAF_ASSERT(hdr.payload_len == 0 || writer != nullptr);
  write(ctx, r.wr_buf, hdr, writer);
  tbl_.flush(r);
}

void instance::add_published_actor(uint16_t port,
                                   strong_actor_ptr published_actor,
                                   std::set<std::string> published_interface) {
  CAF_LOG_TRACE(CAF_ARG(port) << CAF_ARG(published_actor)
                << CAF_ARG(published_interface));
  using std::swap;
  auto& entry = published_actors_[port];
  swap(entry.first, published_actor);
  swap(entry.second, published_interface);
  notify<hook::actor_published>(entry.first, entry.second, port);
}

size_t instance::remove_published_actor(uint16_t port,
                                        removed_published_actor* cb) {
  CAF_LOG_TRACE(CAF_ARG(port));
  auto i = published_actors_.find(port);
  if (i == published_actors_.end())
    return 0;
  if (cb)
    (*cb)(i->second.first, i->first);
  published_actors_.erase(i);
  return 1;
}

size_t instance::remove_published_actor(const actor_addr& whom,
                                        uint16_t port,
                                        removed_published_actor* cb) {
  CAF_LOG_TRACE(CAF_ARG(whom) << CAF_ARG(port));
  size_t result = 0;
  if (port != 0) {
    auto i = published_actors_.find(port);
    if (i != published_actors_.end() && i->second.first == whom) {
      if (cb)
        (*cb)(i->second.first, port);
      published_actors_.erase(i);
      result = 1;
    }
  } else {
    auto i = published_actors_.begin();
    while (i != published_actors_.end()) {
      if (i->second.first == whom) {
        if (cb)
          (*cb)(i->second.first, i->first);
        i = published_actors_.erase(i);
        ++result;
      } else {
        ++i;
      }
    }
  }
  return result;
}

bool instance::dispatch(execution_unit* ctx, const strong_actor_ptr& sender,
                        const std::vector<strong_actor_ptr>& forwarding_stack,
                        const strong_actor_ptr& receiver, message_id mid,
                        const message& msg) {
  CAF_LOG_TRACE(CAF_ARG(sender) << CAF_ARG(receiver)
                << CAF_ARG(mid) << CAF_ARG(msg));
  CAF_ASSERT(receiver && system().node() != receiver->node());
  auto path = lookup(receiver->node());
  if (! path) {
    notify<hook::message_sending_failed>(sender, receiver, mid, msg);
    return false;
  }
  auto writer = make_callback([&](serializer& sink) {
    sink << forwarding_stack << msg;
  });
  header hdr{message_type::dispatch_message, 0, 0, mid.integer_value(),
             sender ? sender->node() : this_node(), receiver->node(),
             sender ? sender->id() : invalid_actor_id, receiver->id()};
  write(ctx, path->wr_buf, hdr, &writer);
  flush(*path);
  notify<hook::message_sent>(sender, path->next_hop, receiver, mid, msg);
  return true;
}

void instance::write(execution_unit* ctx, buffer_type& buf,
                     header& hdr, payload_writer* pw) {
  CAF_LOG_TRACE(CAF_ARG(hdr));
  try {
    if (pw) {
      auto pos = buf.size();
      // write payload first (skip first 72 bytes and write header later)
      char placeholder[basp::header_size];
      buf.insert(buf.end(), std::begin(placeholder), std::end(placeholder));
      binary_serializer bs{ctx, buf};
      (*pw)(bs);
      auto plen = buf.size() - pos - basp::header_size;
      CAF_ASSERT(plen <= std::numeric_limits<uint32_t>::max());
      hdr.payload_len = static_cast<uint32_t>(plen);
      stream_serializer<charbuf> out{ctx, buf.data() + pos, basp::header_size};
      out << hdr;
    } else {
      binary_serializer bs{ctx, buf};
      bs << hdr;
    }
  }
  catch (std::exception& e) {
    CAF_LOG_ERROR(CAF_ARG(e.what()));
  }
}

void instance::write_server_handshake(execution_unit* ctx,
                                      buffer_type& out_buf,
                                      optional<uint16_t> port) {
  CAF_LOG_TRACE(CAF_ARG(port));
  using namespace detail;
  published_actor* pa = nullptr;
  if (port) {
    auto i = published_actors_.find(*port);
    if (i != published_actors_.end())
      pa = &i->second;
  }
  CAF_LOG_DEBUG_IF(! pa && port, "no actor published");
  auto writer = make_callback([&](serializer& sink) {
    sink << callee_.system().config().middleman_app_identifier;
    if (pa) {
      auto i = pa->first ? pa->first->id() : invalid_actor_id;
      sink << i << pa->second;
    } else {
      sink << invalid_actor_id << std::set<std::string>{};
    }
  });
  header hdr{message_type::server_handshake, 0, 0, version,
             this_node_, invalid_node_id,
             pa && pa->first ? pa->first->id() : invalid_actor_id,
             invalid_actor_id};
  write(ctx, out_buf, hdr, &writer);
}

void instance::write_client_handshake(execution_unit* ctx,
                                      buffer_type& buf,
                                      const node_id& remote_side) {
  CAF_LOG_TRACE(CAF_ARG(remote_side));
  auto writer = make_callback([&](serializer& sink) {
    sink << callee_.system().config().middleman_app_identifier;
  });
  header hdr{message_type::client_handshake, 0, 0, 0,
             this_node_, remote_side, invalid_actor_id, invalid_actor_id};
  write(ctx, buf, hdr, &writer);
}

void instance::write_announce_proxy(execution_unit* ctx, buffer_type& buf,
                                    const node_id& dest_node, actor_id aid) {
  CAF_LOG_TRACE(CAF_ARG(dest_node) << CAF_ARG(aid));
  header hdr{message_type::announce_proxy, 0, 0, 0,
             this_node_, dest_node, invalid_actor_id, aid};
  write(ctx, buf, hdr);
}

void instance::write_kill_proxy(execution_unit* ctx, buffer_type& buf,
                                const node_id& dest_node, actor_id aid,
                                const error& rsn) {
  CAF_LOG_TRACE(CAF_ARG(dest_node) << CAF_ARG(aid) << CAF_ARG(rsn));
  auto writer = make_callback([&](serializer& sink) {
    sink << rsn;
  });
  header hdr{message_type::kill_proxy, 0, 0, 0,
             this_node_, dest_node, aid, invalid_actor_id};
  write(ctx, buf, hdr, &writer);
}

void instance::write_heartbeat(execution_unit* ctx,
                               buffer_type& buf,
                               const node_id& remote_side) {
  CAF_LOG_TRACE(CAF_ARG(remote_side));
  header hdr{message_type::heartbeat, 0, 0, 0,
             this_node_, remote_side, invalid_actor_id, invalid_actor_id};
  write(ctx, buf, hdr);
}

} // namespace basp
} // namespace io
} // namespace caf
