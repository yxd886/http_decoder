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

#include "caf/scheduled_actor.hpp"

#include "caf/config.hpp"
#include "caf/actor_ostream.hpp"

#include "caf/detail/private_thread.hpp"
#include "caf/detail/sync_request_bouncer.hpp"
#include "caf/detail/default_invoke_result_visitor.hpp"

namespace caf {

// -- related free functions ---------------------------------------------------

result<message> reflect(scheduled_actor*, message_view& x) {
  return x.move_content_to_message();
}

result<message> reflect_and_quit(scheduled_actor* ptr, message_view& x) {
  error err = exit_reason::normal;
  scheduled_actor::default_error_handler(ptr, err);
  return reflect(ptr, x);
}

result<message> print_and_drop(scheduled_actor* ptr, message_view& x) {
  CAF_LOG_WARNING("unexpected message" << CAF_ARG(x.content()));
  aout(ptr) << "*** unexpected message [id: " << ptr->id()
            << ", name: " << ptr->name() << "]: "
            << x.content().stringify()
            << std::endl;
  return sec::unexpected_message;
}

result<message> drop(scheduled_actor*, message_view&) {
  return sec::unexpected_message;
}

// -- static helper functions --------------------------------------------------

void scheduled_actor::default_error_handler(scheduled_actor* ptr, error& x) {
  ptr->fail_state_ = std::move(x);
  ptr->is_terminated(true);
}

void scheduled_actor::default_down_handler(scheduled_actor* ptr, down_msg& x) {
  aout(ptr) << "*** unhandled down message [id: " << ptr->id()
             << ", name: " << ptr->name() << "]: " << to_string(x)
             << std::endl;
}

void scheduled_actor::default_exit_handler(scheduled_actor* ptr, exit_msg& x) {
  if (x.reason)
    default_error_handler(ptr, x.reason);
}

// -- constructors and destructors ---------------------------------------------

scheduled_actor::scheduled_actor(actor_config& cfg)
    : local_actor(cfg),
      timeout_id_(0),
      default_handler_(print_and_drop),
      error_handler_(default_error_handler),
      down_handler_(default_down_handler),
      exit_handler_(default_exit_handler),
      private_thread_(nullptr) {
  // nop
}

scheduled_actor::~scheduled_actor() {
  // signalize to the private thread object that it is
  // unrachable and can be destroyed as well
  if (private_thread_)
    private_thread_->notify_self_destroyed();
}

// -- overridden functions of abstract_actor -----------------------------------

void scheduled_actor::enqueue(mailbox_element_ptr ptr,
                                         execution_unit* eu) {
  CAF_PUSH_AID(id());
  CAF_LOG_TRACE(CAF_ARG(*ptr));
  CAF_ASSERT(ptr != nullptr);
  CAF_ASSERT(! is_blocking());
  auto mid = ptr->mid;
  auto sender = ptr->sender;
  switch (mailbox().enqueue(ptr.release())) {
    case detail::enqueue_result::unblocked_reader: {
      // add a reference count to this actor and re-schedule it
      intrusive_ptr_add_ref(ctrl());
      if (is_detached()) {
        CAF_ASSERT(private_thread_ != nullptr);
        private_thread_->resume();
      } else {
        if (eu)
          eu->exec_later(this);
        else
          home_system().scheduler().enqueue(this);
      }
      break;
    }
    case detail::enqueue_result::queue_closed: {
      if (mid.is_request()) {
        detail::sync_request_bouncer f{exit_reason()};
        f(sender, mid);
      }
      break;
    }
    case detail::enqueue_result::success:
      // enqueued to a running actors' mailbox; nothing to do
      break;
  }
}

// -- overridden functions of local_actor --------------------------------------

const char* scheduled_actor::name() const {
  return "scheduled_actor";
}

void scheduled_actor::launch(execution_unit* eu, bool lazy, bool hide) {
  CAF_LOG_TRACE(CAF_ARG(lazy) << CAF_ARG(hide));
  CAF_ASSERT(! is_blocking());
  is_registered(! hide);
  if (is_detached()) {
    private_thread_ = new detail::private_thread(this);
    private_thread_->start();
    return;
  }
  CAF_ASSERT(eu != nullptr);
  // do not schedule immediately when spawned with `lazy_init`
  // mailbox could be set to blocked
  if (lazy && mailbox().try_block())
    return;
  // scheduler has a reference count to the actor as long as
  // it is waiting to get scheduled
  intrusive_ptr_add_ref(ctrl());
  eu->exec_later(this);
}

bool scheduled_actor::cleanup(error&& fail_state, execution_unit* host) {
  if (is_detached()) {
    CAF_ASSERT(private_thread_ != nullptr);
    private_thread_->shutdown();
  }
  awaited_responses_.clear();
  multiplexed_responses_.clear();
  return local_actor::cleanup(std::move(fail_state), host);
}

// -- overridden functions of resumable ----------------------------------------

resumable::subtype_t scheduled_actor::subtype() const {
  return resumable::scheduled_actor;
}

void scheduled_actor::intrusive_ptr_add_ref_impl() {
  intrusive_ptr_add_ref(ctrl());
}

void scheduled_actor::intrusive_ptr_release_impl() {
  intrusive_ptr_release(ctrl());
}

resumable::resume_result
scheduled_actor::resume(execution_unit* ctx, size_t max_throughput) {
  CAF_PUSH_AID(id());
  if (! activate(ctx))
    return resume_result::done;
  size_t handled_msgs = 0;
  auto reset_timeout_if_needed = [&] {
    if (handled_msgs > 0 && ! bhvr_stack_.empty())
      request_timeout(bhvr_stack_.back().timeout());
  };
  mailbox_element_ptr ptr;
  while (handled_msgs < max_throughput) {
    do {
      ptr = next_message();
      if (! ptr && mailbox().try_block()) {
        reset_timeout_if_needed();
        return resumable::awaiting_message;
      }
    } while (! ptr);
    switch (reactivate(*ptr)) {
      case activation_result::terminated:
        return resume_result::done;
      case activation_result::success:
        ++handled_msgs;
        // iterate cache to see if we are now able
        // to process previously skipped messages
        while (consume_from_cache()) {
          ++handled_msgs;
          bhvr_stack_.cleanup();
          if (finalize()) {
            CAF_LOG_DEBUG("actor finalized while processing cache");
            return resume_result::done;
          }
        }
        break;
      case activation_result::skipped:
        push_to_cache(std::move(ptr));
        break;
      default:
        break;
    }
  }
  reset_timeout_if_needed();
  if (! has_next_message() && mailbox().try_block())
    return resumable::awaiting_message;
  // time's up
  return resumable::resume_later;
}

// -- scheduler callbacks ----------------------------------------------------

proxy_registry* scheduled_actor::proxy_registry_ptr() {
  return nullptr;
}

// -- state modifiers ----------------------------------------------------------

void scheduled_actor::quit(error x) {
  CAF_LOG_TRACE(CAF_ARG(x));
  fail_state_ = std::move(x);
  is_terminated(true);
}

// -- timeout management -------------------------------------------------------

uint32_t scheduled_actor::request_timeout(const duration& d) {
  if (! d.valid()) {
    has_timeout(false);
    return 0;
  }
  has_timeout(true);
  auto result = ++timeout_id_;
  auto msg = make_message(timeout_msg{++timeout_id_});
  CAF_LOG_TRACE("send new timeout_msg, " << CAF_ARG(timeout_id_));
  if (d.is_zero())
    // immediately enqueue timeout message if duration == 0s
    enqueue(ctrl(), invalid_message_id, std::move(msg), context());
  else
    system().scheduler().delayed_send(d, ctrl(), strong_actor_ptr(ctrl()),
                                      message_id::make(), std::move(msg));
  return result;
}

void scheduled_actor::reset_timeout(uint32_t timeout_id) {
  if (is_active_timeout(timeout_id)) {
    has_timeout(false);
  }
}

bool scheduled_actor::is_active_timeout(uint32_t tid) const {
  return has_timeout() && timeout_id_ == tid;
}

// -- message processing -------------------------------------------------------

void scheduled_actor::add_awaited_response_handler(message_id response_id,
                                                   behavior bhvr) {
  if (bhvr.timeout().valid())
    request_response_timeout(bhvr.timeout(), response_id);
  awaited_responses_.emplace_front(response_id, std::move(bhvr));
}

void scheduled_actor::add_multiplexed_response_handler(message_id response_id,
                                                       behavior bhvr) {
  if (bhvr.timeout().valid())
    request_response_timeout(bhvr.timeout(), response_id);
  multiplexed_responses_.emplace(response_id, std::move(bhvr));
}

scheduled_actor::message_category
scheduled_actor::categorize(mailbox_element& x) {
  auto& content = x.content();
  switch (content.type_token()) {
    case make_type_token<atom_value, atom_value, std::string>():
      if (content.get_as<atom_value>(0) == sys_atom::value
          && content.get_as<atom_value>(1) == get_atom::value) {
        auto& what = content.get_as<std::string>(2);
        if (what == "info") {
          CAF_LOG_DEBUG("reply to 'info' message");
          x.sender->enqueue(
            make_mailbox_element(ctrl(), x.mid.response_id(),
                                  {}, ok_atom::value, std::move(what),
                                  strong_actor_ptr{ctrl()}, name()),
            context());
        } else {
          x.sender->enqueue(
            make_mailbox_element(ctrl(), x.mid.response_id(),
                                  {}, sec::unsupported_sys_key),
            context());
        }
        return message_category::internal;
      }
      return message_category::ordinary;
    case make_type_token<timeout_msg>(): {
      auto& tm = content.get_as<timeout_msg>(0);
      auto tid = tm.timeout_id;
      CAF_ASSERT(! x.mid.valid());
      return is_active_timeout(tid) ? message_category::timeout
                                    : message_category::expired_timeout;
    }
    case make_type_token<exit_msg>(): {
      auto& em = content.get_mutable_as<exit_msg>(0);
      // make sure to get rid of attachables if they're no longer needed
      unlink_from(em.source);
      // exit_reason::kill is always fatal
      if (em.reason == exit_reason::kill) {
        fail_state_ = std::move(em.reason);
        is_terminated(true);
      } else {
        exit_handler_(this, em);
      }
      return message_category::internal;
    }
    case make_type_token<down_msg>(): {
      auto& dm = content.get_mutable_as<down_msg>(0);
      down_handler_(this, dm);
      return message_category::internal;
    }
    case make_type_token<error>(): {
      auto& err = content.get_mutable_as<error>(0);
      error_handler_(this, err);
      return message_category::internal;
    }
    default:
      return message_category::ordinary;
  }
}

invoke_message_result scheduled_actor::consume(mailbox_element& x) {
  CAF_LOG_TRACE(CAF_ARG(x));
  current_element_ = &x;
  // short-circuit awaited responses
  if (! awaited_responses_.empty()) {
    auto& pr = awaited_responses_.front();
    // skip all messages until we receive the currently awaited response
    if (x.mid != pr.first)
      return im_skipped;
    if (! pr.second(x.content())) {
      // try again with error if first attempt failed
      auto msg = make_message(make_error(sec::unexpected_response,
                                         x.move_content_to_message()));
      pr.second(msg);
    }
    awaited_responses_.pop_front();
    return im_success;
  }
  // handle multiplexed responses
  if (x.mid.is_response()) {
    auto mrh = multiplexed_responses_.find(x.mid);
    // neither awaited nor multiplexed, probably an expired timeout
    if (mrh == multiplexed_responses_.end())
      return im_dropped;
    if (! mrh->second(x.content())) {
      // try again with error if first attempt failed
      auto msg = make_message(make_error(sec::unexpected_response,
                                         x.move_content_to_message()));
      mrh->second(msg);
    }
    multiplexed_responses_.erase(mrh);
    return im_success;
  }
  // dispatch on the content of x
  switch (categorize(x)) {
    case message_category::expired_timeout:
      CAF_LOG_DEBUG("dropped expired timeout message");
      return im_dropped;
    case message_category::internal:
      CAF_LOG_DEBUG("handled system message");
      return im_success;
    case message_category::timeout: {
      CAF_LOG_DEBUG("handle timeout message");
      if (bhvr_stack_.empty())
        return im_dropped;
      bhvr_stack_.back().handle_timeout();
      return im_success;
    }
    case message_category::ordinary: {
      detail::default_invoke_result_visitor visitor{this};
      bool skipped = false;
      auto had_timeout = has_timeout();
      if (had_timeout)
        has_timeout(false);
      // restore timeout at scope exit if message was skipped
      auto timeout_guard = detail::make_scope_guard([&] {
        if (skipped && had_timeout)
          has_timeout(true);
      });
      auto call_default_handler = [&] {
        auto sres = default_handler_(this, x);
        switch (sres.flag) {
          default:
            break;
          case rt_error:
          case rt_value:
            visitor.visit(sres);
            break;
          case rt_skip:
            skipped = true;
        }
      };
      if (bhvr_stack_.empty()) {
        call_default_handler();
        return ! skipped ? im_success : im_skipped;
      }
      auto& bhvr = bhvr_stack_.back();
      switch (bhvr(visitor, x.content())) {
        default:
          break;
        case match_case::skip:
          skipped = true;
          break;
        case match_case::no_match:
          call_default_handler();
      }
      return ! skipped ? im_success : im_skipped;
    }
  }
  // should be unreachable
  CAF_CRITICAL("invalid message type");
}

/// Tries to consume `x`.
void scheduled_actor::consume(mailbox_element_ptr x) {
  switch (consume(*x)) {
    default:
      break;
    case im_skipped:
      push_to_cache(std::move(x));
  }
}

bool scheduled_actor::consume_from_cache() {
  CAF_LOG_TRACE("");
  auto& cache = mailbox().cache();
  auto i = cache.continuation();
  auto e = cache.end();
  while (i != e)
    switch (consume(*i)) {
      case im_success:
        cache.erase(i);
        return true;
      case im_skipped:
        ++i;
        break;
      case im_dropped:
        i = cache.erase(i);
        break;
    }
  return false;
}

bool scheduled_actor::activate(execution_unit* ctx) {
  CAF_LOG_TRACE("");
  CAF_ASSERT(ctx != nullptr);
  CAF_ASSERT(! is_blocking());
  context(ctx);
  if (is_initialized() && (! has_behavior() || is_terminated())) {
    CAF_LOG_DEBUG_IF(! has_behavior(),
                     "resume called on an actor without behavior");
    CAF_LOG_DEBUG_IF(is_terminated(),
                     "resume called on a terminated actor");
    return false;
  }
# ifndef CAF_NO_EXCEPTIONS
  try {
# endif // CAF_NO_EXCEPTIONS
    if (! is_initialized()) {
      initialize();
      if (finalize()) {
        CAF_LOG_DEBUG("actor_done() returned true right after make_behavior()");
        return false;
      } else {
        CAF_LOG_DEBUG("initialized actor:" << CAF_ARG(name()));
      }
    }
# ifndef CAF_NO_EXCEPTIONS
  }
  catch (...) {
    CAF_LOG_ERROR("actor died during initialization");
    auto eptr = std::current_exception();
    quit(exception_handler_(this, eptr));
    finalize();
    return false;
  }
# endif // CAF_NO_EXCEPTIONS
  return true;
}

auto scheduled_actor::activate(execution_unit* ctx, mailbox_element& x)
-> activation_result {
  CAF_LOG_TRACE(CAF_ARG(x));
  if (! activate(ctx))
    return activation_result::terminated;
  return reactivate(x);
}

auto scheduled_actor::reactivate(mailbox_element& x) -> activation_result {
  CAF_LOG_TRACE(CAF_ARG(x));
# ifndef CAF_NO_EXCEPTIONS
  try {
# endif // CAF_NO_EXCEPTIONS
    switch (consume(x)) {
      case im_dropped:
        return activation_result::dropped;
      case im_success:
        bhvr_stack_.cleanup();
        if (finalize()) {
          CAF_LOG_DEBUG("actor finalized");
          return activation_result::terminated;
        }
        return activation_result::success;
      case im_skipped:
        return activation_result::skipped;
    }
# ifndef CAF_NO_EXCEPTIONS
  }
  catch (std::exception& e) {
    CAF_LOG_INFO("actor died because of an exception, what: " << e.what());
    static_cast<void>(e); // keep compiler happy when not logging
    auto eptr = std::current_exception();
    quit(exception_handler_(this, eptr));
  }
  catch (...) {
    CAF_LOG_INFO("actor died because of an unknown exception");
    auto eptr = std::current_exception();
    quit(exception_handler_(this, eptr));
  }
  finalize();
  return activation_result::terminated;
# endif // CAF_NO_EXCEPTIONS
}

// -- behavior management ----------------------------------------------------

void scheduled_actor::do_become(behavior bhvr, bool discard_old) {
  if (discard_old && ! bhvr_stack_.empty())
    bhvr_stack_.pop_back();
  // request_timeout simply resets the timeout when it's invalid
  request_timeout(bhvr.timeout());
  bhvr_stack_.push_back(std::move(bhvr));
}

bool scheduled_actor::finalize() {
  if (has_behavior() && ! is_terminated())
    return false;
  CAF_LOG_DEBUG("actor either has no behavior or has set an exit reason");
  on_exit();
  bhvr_stack_.clear();
  bhvr_stack_.cleanup();
  cleanup(std::move(fail_state_), context());
  return true;
}

} // namespace caf
