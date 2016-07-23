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

#include "caf/io/basp/routing_table.hpp"

#include "caf/io/middleman.hpp"

namespace caf {
namespace io {
namespace basp {

routing_table::routing_table(abstract_broker* parent) : parent_(parent) {
  // nop
}

routing_table::~routing_table() {
  // nop
}

optional<routing_table::route> routing_table::lookup(const node_id& target) {
  auto hdl = lookup_direct(target);
  if (hdl != invalid_connection_handle)
    return route{parent_->wr_buf(hdl), target, hdl};
  // pick first available indirect route
  auto i = indirect_.find(target);
  if (i != indirect_.end()) {
    auto& hops = i->second;
    while (! hops.empty()) {
      auto& hop = *hops.begin();
      hdl = lookup_direct(hop);
      if (hdl != invalid_connection_handle)
        return route{parent_->wr_buf(hdl), hop, hdl};
      else
        hops.erase(hops.begin());
    }
  }
  return none;
}

void routing_table::flush(const route& r) {
  parent_->flush(r.hdl);
}

node_id routing_table::lookup_direct(const connection_handle& hdl) const {
  return get_opt(direct_by_hdl_, hdl, invalid_node_id);
}

connection_handle routing_table::lookup_direct(const node_id& nid) const {
  return get_opt(direct_by_nid_, nid, invalid_connection_handle);
}

node_id routing_table::lookup_indirect(const node_id& nid) const {
  auto i = indirect_.find(nid);
  if (i == indirect_.end())
    return invalid_node_id;
  if (i->second.empty())
    return invalid_node_id;
  return *i->second.begin();
}

void routing_table::blacklist(const node_id& hop, const node_id& dest) {
  blacklist_[dest].emplace(hop);
  auto i = indirect_.find(dest);
  if (i == indirect_.end())
    return;
  i->second.erase(hop);
  if (i->second.empty())
    indirect_.erase(i);
}

void routing_table::erase_direct(const connection_handle& hdl,
                                 erase_callback& cb) {
  auto i = direct_by_hdl_.find(hdl);
  if (i == direct_by_hdl_.end())
    return;
  cb(i->second);
  parent_->parent().notify<hook::connection_lost>(i->second);
  direct_by_nid_.erase(i->second);
  direct_by_hdl_.erase(i);
}

bool routing_table::erase_indirect(const node_id& dest) {
  auto i = indirect_.find(dest);
  if (i == indirect_.end())
    return false;
  if (parent_->parent().has_hook())
    for (auto& nid : i->second)
      parent_->parent().notify<hook::route_lost>(nid, dest);
  indirect_.erase(i);
  return true;
}

void routing_table::add_direct(const connection_handle& hdl,
                               const node_id& nid) {
  CAF_ASSERT(direct_by_hdl_.count(hdl) == 0);
  CAF_ASSERT(direct_by_nid_.count(nid) == 0);
  direct_by_hdl_.emplace(hdl, nid);
  direct_by_nid_.emplace(nid, hdl);
  parent_->parent().notify<hook::new_connection_established>(nid);
}

bool routing_table::add_indirect(const node_id& hop, const node_id& dest) {
  auto i = blacklist_.find(dest);
  if (i == blacklist_.end() || i->second.count(hop) == 0) {
    auto& hops = indirect_[dest];
    auto added_first = hops.empty();
    hops.emplace(hop);
    parent_->parent().notify<hook::new_route_added>(hop, dest);
    return added_first;
  }
  return false; // blacklisted
}

bool routing_table::reachable(const node_id& dest) {
  return direct_by_nid_.count(dest) > 0 || indirect_.count(dest) > 0;
}

size_t routing_table::erase(const node_id& dest, erase_callback& cb) {
  cb(dest);
  size_t res = 0;
  auto i = indirect_.find(dest);
  if (i != indirect_.end()) {
    res = i->second.size();
    for (auto& nid : i->second) {
      cb(nid);
      parent_->parent().notify<hook::route_lost>(nid, dest);
    }
    indirect_.erase(i);
  }
  auto hdl = lookup_direct(dest);
  if (hdl != invalid_connection_handle) {
    direct_by_hdl_.erase(hdl);
    direct_by_nid_.erase(dest);
    parent_->parent().notify<hook::connection_lost>(dest);
    ++res;
  }
  return res;
}

} // namespace basp
} // namespace io
} // namespace caf
