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

#include "caf/message.hpp"

#include <iostream>

#include "caf/serializer.hpp"
#include "caf/actor_system.hpp"
#include "caf/deserializer.hpp"
#include "caf/message_builder.hpp"
#include "caf/message_handler.hpp"
#include "caf/string_algorithms.hpp"

#include "caf/detail/decorated_tuple.hpp"
#include "caf/detail/concatenated_tuple.hpp"
#include "caf/detail/dynamic_message_data.hpp"

namespace caf {

message::message(message&& other) noexcept : vals_(std::move(other.vals_)) {
  // nop
}

message::message(const data_ptr& ptr) noexcept : vals_(ptr) {
  // nop
}

message& message::operator=(message&& other) noexcept {
  vals_.swap(other.vals_);
  return *this;
}

message::~message() {
  // nop
}

void message::reset(raw_ptr new_ptr, bool add_ref) noexcept {
  vals_.reset(new_ptr, add_ref);
}

void message::swap(message& other) noexcept {
  vals_.swap(other.vals_);
}

void* message::get_mutable(size_t p) {
  CAF_ASSERT(vals_);
  return vals_->get_mutable(p);
}

message message::copy(const type_erased_tuple& xs) {
  message_builder mb;
  for (size_t i = 0; i < xs.size(); ++i)
    mb.emplace(xs.copy(i));
  return mb.move_to_message();
}

message& message::operator+=(const message& x) {
  auto tmp = *this + x;
  swap(tmp);
  return *this;
}

message message::drop(size_t n) const {
  CAF_ASSERT(vals_);
  if (n == 0)
    return *this;
  if (n >= size())
    return message{};
  std::vector<size_t> mapping (size() - n);
  size_t i = n;
  std::generate(mapping.begin(), mapping.end(), [&] { return i++; });
  return message {detail::decorated_tuple::make(vals_, mapping)};
}

message message::drop_right(size_t n) const {
  CAF_ASSERT(vals_);
  if (n == 0) {
    return *this;
  }
  if (n >= size()) {
    return message{};
  }
  std::vector<size_t> mapping(size() - n);
  std::iota(mapping.begin(), mapping.end(), size_t{0});
  return message{detail::decorated_tuple::make(vals_, std::move(mapping))};
}

message message::slice(size_t pos, size_t n) const {
  auto s = size();
  if (pos >= s) {
    return message{};
  }
  std::vector<size_t> mapping(std::min(s - pos, n));
  std::iota(mapping.begin(), mapping.end(), pos);
  return message{detail::decorated_tuple::make(vals_, std::move(mapping))};
}

optional<message> message::apply(message_handler handler) {
  return handler(*this);
}

message message::extract_impl(size_t start, message_handler handler) const {
  auto s = size();
  for (size_t i = start; i < s; ++i) {
    for (size_t n = (s - i) ; n > 0; --n) {
      auto next_slice = slice(i, n);
      auto res = handler(next_slice);
      if (res) {
        std::vector<size_t> mapping(s);
        std::iota(mapping.begin(), mapping.end(), size_t{0});
        auto first = mapping.begin() + static_cast<ptrdiff_t>(i);
        auto last = first + static_cast<ptrdiff_t>(n);
        mapping.erase(first, last);
        if (mapping.empty()) {
          return message{};
        }
        message next{detail::decorated_tuple::make(vals_, std::move(mapping))};
        return next.extract_impl(i, handler);
      }
    }
  }
  return *this;
}

message message::extract(message_handler handler) const {
  return extract_impl(0, handler);
}

message::cli_res message::extract_opts(std::vector<cli_arg> xs,
                                       help_factory f, bool no_help) const {
  std::string helpstr;
  auto make_error = [&](std::string err) -> cli_res {
    return {*this, std::set<std::string>{}, std::move(helpstr), std::move(err)};
  };
  // add default help item if user did not specify any help option
  auto pred = [](const cli_arg& arg) -> bool {
    std::vector<std::string> s;
    split(s, arg.name, is_any_of(","), token_compress_on);
    if (s.empty())
      return false;
    auto has_short_help = [](const std::string& opt) {
      return opt.find_first_of("h?") != std::string::npos;
    };
    return s[0] == "help"
           || std::find_if(s.begin() + 1, s.end(), has_short_help) != s.end();
  };
  if (! no_help && std::none_of(xs.begin(), xs.end(), pred)) {
    xs.push_back(cli_arg{"help,h,?", "print this text"});
  }
  std::map<std::string, cli_arg*> shorts;
  std::map<std::string, cli_arg*> longs;
  for (auto& cliarg : xs) {
    std::vector<std::string> s;
    split(s, cliarg.name, is_any_of(","), token_compress_on);
    if (s.empty()) {
      return make_error("invalid option name: " + cliarg.name);
    }
    longs["--" + s.front()] = &cliarg;
    for (size_t i = 1; i < s.size(); ++i) {
      if (s[i].size() != 1) {
        return make_error("invalid short option name: " + s[i]);
      }
      shorts["-" + s[i]] = &cliarg;
    }
    // generate helptext for this item
    auto& ht = cliarg.helptext;
    if (s.size() == 1) {
      ht += "--";
      ht += s.front();
    } else {
      ht += "-";
      ht += s[1];
      ht += " [";
      for (size_t i = 2; i < s.size(); ++i) {
        ht += "-";
        ht += s[i];
        ht += ",";
      }
      ht += "--";
      ht += s.front();
      ht += "]";
    }
    if (cliarg.fun) {
      ht += " arg";
    }
  }
  if (f) {
    helpstr = f(xs);
  } else {
    auto op = [](size_t tmp, const cli_arg& arg) {
      return std::max(tmp, arg.helptext.size());
    };
    auto name_width = std::accumulate(xs.begin(), xs.end(), size_t{0}, op);
    std::ostringstream oss;
    oss << std::left;
    oss << "Allowed options:" << std::endl;
    for (auto& ca : xs) {
      oss << "  ";
      oss.width(static_cast<std::streamsize>(name_width));
      oss << ca.helptext << "  : " << ca.text << std::endl;
    }
    helpstr = oss.str();
  }
  std::set<std::string> opts;
  auto insert_opt_name = [&](const cli_arg* ptr) {
    auto separator = ptr->name.find(',');
    if (separator == std::string::npos) {
      opts.insert(ptr->name);
    } else {
      opts.insert(ptr->name.substr(0, separator));
    }
  };
  // we can't `return make_error(...)` from inside `extract`, hence we
  // store any occurred error in a temporary variable returned at the end
  std::string error;
  auto res = extract({
    [&](const std::string& arg) -> optional<skip_t> {
      if (arg.empty() || arg.front() != '-') {
        return skip();
      }
      auto i = shorts.find(arg.substr(0, 2));
      if (i != shorts.end()) {
        if (i->second->fun) {
          // this short opt expects two arguments
          if (arg.size() > 2) {
             // this short opt comes with a value (no space), e.g., -x2
            if (! i->second->fun(arg.substr(2))) {
              error = "invalid value for " + i->second->name + ": " + arg;
              return skip();
            }
            insert_opt_name(i->second);
            return none;
          }
          // no value given, try two-argument form below
          return skip();
        } else if (i->second->flag) {
          *i->second->flag = true;
        }
        insert_opt_name(i->second);
        return none;
      }
      auto eq_pos = arg.find('=');
      auto j = longs.find(arg.substr(0, eq_pos));
      if (j != longs.end()) {
        if (j->second->fun) {
          if (eq_pos == std::string::npos) {
            error =  "missing argument to " + arg;
            return skip();
          }
          if (! j->second->fun(arg.substr(eq_pos + 1))) {
            error = "invalid value for " + j->second->name + ": " + arg;
            return skip();
          }
          insert_opt_name(j->second);
          return none;
        } else if (j->second->flag) {
          *j->second->flag = true;
        }
        insert_opt_name(j->second);
        return none;
      }
      error = "unknown command line option: " + arg;
      return skip();
    },
    [&](const std::string& arg1,
        const std::string& arg2) -> optional<skip_t> {
      if (arg1.size() < 2 || arg1[0] != '-' || arg1[1] == '-') {
        return skip();
      }
      auto i = shorts.find(arg1.substr(0, 2));
      if (i != shorts.end()) {
        if (! i->second->fun || arg1.size() > 2) {
          // this short opt either expects no argument or comes with a value
          // (no  space), e.g., -x2, so we have to parse it with the
          // one-argument form above
          return skip();
        }
        CAF_ASSERT(arg1.size() == 2);
        if (! i->second->fun(arg2)) {
          error = "invalid value for option " + i->second->name + ": " + arg2;
          return skip();
        }
        insert_opt_name(i->second);
        return none;
      }
      error = "unknown command line option: " + arg1;
      return skip();
    }
  });
  return {res, std::move(opts), std::move(helpstr), std::move(error)};
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr)
    : name(std::move(nstr)),
      text(std::move(tstr)),
      flag(nullptr) {
  // nop
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr, bool& arg)
  : name(std::move(nstr)),
    text(std::move(tstr)),
    flag(&arg) {
  arg = false;
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr, consumer f)
    : name(std::move(nstr)),
      text(std::move(tstr)),
      fun(std::move(f)),
      flag(nullptr) {
  // nop
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr, atom_value& arg)
    : name(std::move(nstr)),
      text(std::move(tstr)),
      fun([&arg](const std::string& str) -> bool {
        if (str.size() <= 10) {
          arg = static_cast<atom_value>(detail::atom_val(str.c_str()));
          return true;
        }
        return false;
      }),
      flag(nullptr) {
  // nop
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr, std::string& arg)
    : name(std::move(nstr)),
      text(std::move(tstr)),
      fun([&arg](const std::string& str) -> bool {
            arg = str;
            return true;
          }),
      flag(nullptr) {
  // nop
}

message::cli_arg::cli_arg(std::string nstr, std::string tstr,
                          std::vector<std::string>& arg)
    : name(std::move(nstr)),
      text(std::move(tstr)),
      fun([&arg](const std::string& str) -> bool {
        arg.push_back(str);
        return true;
      }),
      flag(nullptr) {
  // nop
}

message message::concat_impl(std::initializer_list<data_ptr> xs) {
  auto not_nullptr = [](const data_ptr& ptr) { return ptr.get() != nullptr; };
  switch (std::count_if(xs.begin(), xs.end(), not_nullptr)) {
    case 0:
      return message{};
    case 1:
      return message{*std::find_if(xs.begin(), xs.end(), not_nullptr)};
    default:
      return message{detail::concatenated_tuple::make(xs)};
  }
}

void serialize(serializer& sink, const message& msg, const unsigned int) {
  if (! sink.context())
    CAF_RAISE_ERROR("Cannot serialize message without context.");
  // build type name
  uint16_t zero = 0;
  std::string tname = "@<>";
  if (msg.empty()) {
    sink.begin_object(zero, tname);
    sink.end_object();
    return;
  }
  auto& types = sink.context()->system().types();
  auto n = msg.size();
  for (size_t i = 0; i < n; ++i) {
    auto rtti = msg.cvals()->type(i);
    auto ptr = types.portable_name(rtti);
    if (! ptr) {
      std::cerr << "[ERROR]: cannot serialize message because a type was "
                   "not added to the types list, typeid name: "
                << (rtti.second ? rtti.second->name() : "-not-available-")
                << std::endl;
      CAF_RAISE_ERROR("unknown type while serializing");
    }
    tname += '+';
    tname += *ptr;
  }
  sink.begin_object(zero, tname);
  for (size_t i = 0; i < n; ++i)
    msg.cvals()->save(i, sink);
  sink.end_object();
}

void serialize(deserializer& source, message& msg, const unsigned int) {
  if (! source.context())
    CAF_RAISE_ERROR("Cannot deserialize message without context.");
  uint16_t zero;
  std::string tname;
  source.begin_object(zero, tname);
  if (zero != 0)
    CAF_RAISE_ERROR("unexpected builtin type found in message");
  if (tname == "@<>") {
    msg = message{};
    return;
  }
  if (tname.compare(0, 4, "@<>+") != 0)
    CAF_RAISE_ERROR("type name does not start with @<>+: " + tname);
  // iterate over concatenated type names
  auto eos = tname.end();
  auto next = [&](std::string::iterator iter) {
    return std::find(iter, eos, '+');
  };
  auto& types = source.context()->system().types();
  auto dmd = make_counted<detail::dynamic_message_data>();
  std::string tmp;
  std::string::iterator i = next(tname.begin());
  ++i; // skip first '+' sign
  do {
    auto n = next(i);
    tmp.assign(i, n);
    auto ptr = types.make_value(tmp);
    if (! ptr)
      CAF_RAISE_ERROR("cannot serialize a value of type " + tmp);
    ptr->load(source);
    dmd->append(std::move(ptr));
    if (n != eos)
      i = n + 1;
    else
      i = eos;
  } while (i != eos);
  source.end_object();
  message result{std::move(dmd)};
  msg.swap(result);
}

std::string to_string(const message& msg) {
  if (msg.empty())
    return "<empty-message>";
  std::string str = "(";
  str += msg.cvals()->stringify(0);
  for (size_t i = 1; i < msg.size(); ++i) {
    str += ", ";
    str += msg.cvals()->stringify(i);
  }
  str += ")";
  return str;
}

} // namespace caf
