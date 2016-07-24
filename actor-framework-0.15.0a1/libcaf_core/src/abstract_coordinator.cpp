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

#include "caf/scheduler/abstract_coordinator.hpp"

#include <ios>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <condition_variable>

#include "caf/send.hpp"
#include "caf/actor_system.hpp"
#include "caf/scoped_actor.hpp"
#include "caf/actor_ostream.hpp"
#include "caf/system_messages.hpp"
#include "caf/scheduled_actor.hpp"
#include "caf/actor_system_config.hpp"

#include "caf/scheduler/coordinator.hpp"

#include "caf/policy/work_stealing.hpp"

#include "caf/logger.hpp"

namespace caf {
namespace scheduler {

/******************************************************************************
 *                     utility and implementation details                     *
 ******************************************************************************/

namespace {

using hrc = std::chrono::high_resolution_clock;

struct delayed_msg {
  strong_actor_ptr from;
  strong_actor_ptr to;
  message_id mid;
  message msg;
};

inline void deliver(delayed_msg& dm) {
  dm.to->enqueue(dm.from, dm.mid, std::move(dm.msg), nullptr);
}

template <class Map, class... Ts>
inline void insert_dmsg(Map& storage, const duration& d, Ts&&... xs) {
  auto tout = hrc::now();
  tout += d;
  delayed_msg dmsg{std::forward<Ts>(xs)...};
  storage.emplace(std::move(tout), std::move(dmsg));
}

class timer_actor : public blocking_actor {
public:
  explicit timer_actor(actor_config& cfg) : blocking_actor(cfg) {
    // nop
  }

  bool await_data(const hrc::time_point& tp) {
    if (has_next_message())
      return true;
    return mailbox().synchronized_await(mtx_, cv_, tp);
  }

  mailbox_element_ptr try_dequeue() {
    blocking_actor::await_data();
    return next_message();
  }

  mailbox_element_ptr try_dequeue(const hrc::time_point& tp) {
    if (await_data(tp))
      return next_message();
    return mailbox_element_ptr{};
  }

  void act() override {
    // setup & local variables
    std::multimap<hrc::time_point, delayed_msg> messages;
    // message handling rules
    message_handler mfun{
      [&](const duration& d, strong_actor_ptr& from, strong_actor_ptr& to,
          message_id mid, message& msg) {
         insert_dmsg(messages, d, std::move(from),
                     std::move(to), mid, std::move(msg));
      }
    };
    mailbox_element_ptr msg_ptr;
    for (;;) {
      while (! msg_ptr) {
        if (messages.empty()) {
          msg_ptr = try_dequeue();
        } else {
          auto tout = hrc::now();
          // handle timeouts (send messages)
          auto it = messages.begin();
          while (it != messages.end() && (it->first) <= tout) {
            deliver(it->second);
            it = messages.erase(it);
          }
          // wait for next message or next timeout
          if (it != messages.end())
            msg_ptr = try_dequeue(it->first);
        }
      }
      auto& content = msg_ptr->content();
      if (content.type_token() == make_type_token<exit_msg>()) {
        auto& em = content.get_as<exit_msg>(0);
        if (em.reason) {
          fail_state(em.reason);
          return;
        }
      }
      mfun(content);
      msg_ptr.reset();
    }
  }
};

using string_sink = std::function<void (std::string&&)>;

// the first value is the use count, the last ostream_handle that
// decrements it to 0 removes the ostream pointer from the map
using counted_sink = std::pair<size_t, string_sink>;

using sink_cache = std::map<std::string, counted_sink>;

class sink_handle {
public:
  using iterator = sink_cache::iterator;

  sink_handle() : cache_(nullptr) {
    // nop
  }

  sink_handle(sink_cache* fc, iterator iter) : cache_(fc), iter_(iter) {
    if (cache_)
      ++iter_->second.first;
  }

  sink_handle(const sink_handle& other) : cache_(nullptr) {
    *this = other;
  }

  sink_handle& operator=(const sink_handle& other) {
    if (cache_ != other.cache_ || iter_ != other.iter_) {
      clear();
      cache_ = other.cache_;
      if (cache_) {
        iter_ = other.iter_;
        ++iter_->second.first;
      }
    }
    return *this;
  }

  ~sink_handle() {
    clear();
  }

  explicit operator bool() const {
    return cache_ != nullptr;
  }

  string_sink& operator*() {
    CAF_ASSERT(iter_->second.second != nullptr);
    return iter_->second.second;
  }

private:
  void clear() {
    if (cache_ && --iter_->second.first == 0) {
      cache_->erase(iter_);
      cache_ = nullptr;
    }
  }

  sink_cache* cache_;
  sink_cache::iterator iter_;
};

string_sink make_sink(actor_system& sys, const std::string& fn, int flags) {
  if (fn.empty())
    return nullptr;
  if (fn.front() == ':') {
    // "virtual file" name given, translate this to group communication
    auto grp = sys.groups().get_local(fn);
    return [grp, fn](std::string&& out) { anon_send(grp, fn, std::move(out)); };
  }
  auto append = static_cast<bool>(flags & actor_ostream::append);
  auto fs = std::make_shared<std::ofstream>();
  fs->open(fn, append ? std::ios_base::out | std::ios_base::app
                      : std::ios_base::out);
  if (fs->is_open())
    return [fs](std::string&& out) { *fs << out; };
std::cerr << "cannot open file: " << fn << std::endl;
  return nullptr;
}

sink_handle get_sink_handle(actor_system& sys, sink_cache& fc,
                            const std::string& fn, int flags) {
  auto i = fc.find(fn);
  if (i != fc.end())
    return {&fc, i};
  auto fs = make_sink(sys, fn, flags);
  if (fs) {
    i = fc.emplace(fn, sink_cache::mapped_type{0, std::move(fs)}).first;
    return {&fc, i};
  }
  return {};
}

void printer_loop(blocking_actor* self) {
  struct actor_data {
    std::string current_line;
    sink_handle redirect;
    actor_data() {
      // nop
    }
  };
  using data_map = std::unordered_map<actor_id, actor_data>;
  sink_cache fcache;
  sink_handle global_redirect;
  data_map data;
  auto get_data = [&](actor_id addr, bool insert_missing) -> actor_data* {
    if (addr == invalid_actor_id)
      return nullptr;
    auto i = data.find(addr);
    if (i == data.end() && insert_missing)
      i = data.emplace(addr, actor_data{}).first;
    if (i != data.end())
      return &(i->second);
    return nullptr;
  };
  auto flush = [&](actor_data* what, bool forced) {
    if (! what)
      return;
    auto& line = what->current_line;
    if (line.empty() || (line.back() != '\n' && !forced))
      return;
    if (what->redirect)
      (*what->redirect)(std::move(line));
    else if (global_redirect)
      (*global_redirect)(std::move(line));
    else
      std::cout << line << std::flush;
    line.clear();
  };
  bool done = false;
  self->do_receive(
    [&](add_atom, actor_id aid, std::string& str) {
      if (str.empty() || aid == invalid_actor_id)
        return;
      auto d = get_data(aid, true);
      if (d) {
        d->current_line += str;
        flush(d, false);
      }
    },
    [&](flush_atom, actor_id aid) {
      flush(get_data(aid, false), true);
    },
    [&](delete_atom, actor_id aid) {
      auto data_ptr = get_data(aid, false);
      if (data_ptr) {
        flush(data_ptr, true);
        data.erase(aid);
      }
    },
    [&](redirect_atom, const std::string& fn, int flag) {
      global_redirect = get_sink_handle(self->system(), fcache, fn, flag);
    },
    [&](redirect_atom, actor_id aid, const std::string& fn, int flag) {
      auto d = get_data(aid, true);
      if (d)
        d->redirect = get_sink_handle(self->system(), fcache, fn, flag);
    },
    [&](exit_msg& em) {
      self->fail_state(std::move(em.reason));
      done = true;
    }
  ).until([&] { return done; });
}

} // namespace <anonymous>

/******************************************************************************
 *                       implementation of coordinator                        *
 ******************************************************************************/

actor abstract_coordinator::printer() const {
  return actor_cast<actor>(printer_);
}

void abstract_coordinator::start() {
  CAF_LOG_TRACE("");
  // launch utility actors
  timer_ = actor_cast<strong_actor_ptr>(system_.spawn<timer_actor, hidden + detached>());
  printer_ = actor_cast<strong_actor_ptr>(system_.spawn<hidden + detached>(printer_loop));
}

void abstract_coordinator::init(actor_system_config& cfg) {
  max_throughput_ = cfg.scheduler_max_throughput;
  num_workers_ = cfg.scheduler_max_threads;
}

actor_system::module::id_t abstract_coordinator::id() const {
  return module::scheduler;
}

void* abstract_coordinator::subtype_ptr() {
  return this;
}

void abstract_coordinator::stop_actors() {
  CAF_LOG_TRACE("");
  scoped_actor self{system_, true};
  anon_send_exit(timer_, exit_reason::user_shutdown);
  anon_send_exit(printer_, exit_reason::user_shutdown);
  self->wait_for(timer_, printer_);
}

abstract_coordinator::abstract_coordinator(actor_system& sys)
    : next_worker_(0),
      max_throughput_(0),
      num_workers_(0),
      system_(sys) {
  // nop
}

void abstract_coordinator::cleanup_and_release(resumable* ptr) {
  class dummy_unit : public execution_unit {
  public:
    dummy_unit(local_actor* job) : execution_unit(&job->home_system()) {
      // nop
    }
    void exec_later(resumable* job) override {
      resumables.push_back(job);
    }
    std::vector<resumable*> resumables;
  };
  switch (ptr->subtype()) {
    case resumable::scheduled_actor:
    case resumable::io_actor: {
      auto dptr = static_cast<scheduled_actor*>(ptr);
      dummy_unit dummy{dptr};
      dptr->cleanup(make_error(exit_reason::user_shutdown), &dummy);
      while (! dummy.resumables.empty()) {
        auto sub = dummy.resumables.back();
        dummy.resumables.pop_back();
        switch (sub->subtype()) {
          case resumable::scheduled_actor:
          case resumable::io_actor: {
            auto dsub = static_cast<scheduled_actor*>(sub);
            dsub->cleanup(make_error(exit_reason::user_shutdown), &dummy);
            break;
          }
          default:
            break;
        }
      }
      break;
    }
    default:
      break;
  }
  intrusive_ptr_release(ptr);
}

} // namespace scheduler
} // namespace caf
