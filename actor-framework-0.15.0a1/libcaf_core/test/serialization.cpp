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

#include "caf/config.hpp"

#define CAF_SUITE serialization
#include "caf/test/unit_test.hpp"

#include <new>
#include <set>
#include <list>
#include <stack>
#include <tuple>
#include <locale>
#include <memory>
#include <string>
#include <limits>
#include <vector>
#include <cstring>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <iterator>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "caf/message.hpp"
#include "caf/streambuf.hpp"
#include "caf/serializer.hpp"
#include "caf/ref_counted.hpp"
#include "caf/deserializer.hpp"
#include "caf/actor_system.hpp"
#include "caf/proxy_registry.hpp"
#include "caf/message_handler.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/primitive_variant.hpp"
#include "caf/binary_serializer.hpp"
#include "caf/binary_deserializer.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/make_type_erased_view.hpp"
#include "caf/make_type_erased_tuple_view.hpp"

#include "caf/detail/ieee_754.hpp"
#include "caf/detail/int_list.hpp"
#include "caf/detail/safe_equal.hpp"
#include "caf/detail/type_traits.hpp"
#include "caf/detail/get_mac_addresses.hpp"

using namespace std;
using namespace caf;
using caf::detail::type_erased_value_impl;

namespace {

using strmap = map<string, u16string>;

struct raw_struct {
  string str;
};

template <class Processor>
void serialize(Processor& proc, raw_struct& x) {
  proc & x.str;
}

bool operator==(const raw_struct& lhs, const raw_struct& rhs) {
  return lhs.str == rhs.str;
}

enum class test_enum : uint32_t {
  a,
  b,
  c
};

std::string to_string(test_enum x) {
  switch (x) {
    case test_enum::a: return "a";
    case test_enum::b: return "b";
    case test_enum::c: return "c";
  }
  return "???";
}

struct test_array {
  int value[4];
  int value2[2][4];
};

template <class Processor>
void serialize(Processor& proc, test_array& x, const unsigned int) {
  proc & x.value;
  proc & x.value2;
}

struct test_empty_non_pod {
  test_empty_non_pod() = default;
  test_empty_non_pod(const test_empty_non_pod&) = default;
  virtual void foo() {
    // nop
  }
  virtual ~test_empty_non_pod() {
    // nop
  }
};

template <class Processor>
void serialize(Processor&, test_empty_non_pod&, const unsigned int) {
  // nop
}

class config : public actor_system_config {
public:
  config() {
    add_message_type<test_enum>("test_enum");
    add_message_type<raw_struct>("raw_struct");
    add_message_type<test_array>("test_array");
    add_message_type<test_empty_non_pod>("test_empty_non_pod");
  }
};

struct fixture {
  int32_t i32 = -345;
  float f32 = 3.45f;
  double f64 = 54.3;
  test_enum te = test_enum::b;
  string str = "Lorem ipsum dolor sit amet.";
  raw_struct rs;
  test_array ta {
    {0, 1, 2, 3},
    {
      {0, 1, 2, 3},
      {4, 5, 6, 7}
    },
  };
  int ra[3] = {1, 2, 3};

  config cfg;
  actor_system system;
  scoped_execution_unit context;
  message msg;

  template <class Processor>
  void apply(Processor&) {
    // end of recursion
  }

  template <class Processor, class T, class... Ts>
  void apply(Processor& proc, T& x, Ts&... xs) {
    proc & x;
    apply(proc, xs...);
  }

  template <class T, class... Ts>
  vector<char> serialize(T& x, Ts&... xs) {
    vector<char> buf;
    binary_serializer bs{&context, buf};
    apply(bs, x, xs...);
    return buf;
  }

  template <class T, class... Ts>
  void deserialize(const vector<char>& buf, T& x, Ts&... xs) {
    binary_deserializer bd{&context, buf};
    apply(bd, x, xs...);
  }

  // serializes `x` and then deserializes and returns the serialized value
  template <class T>
  T roundtrip(T x) {
    T result;
    deserialize(serialize(x), result);
    return result;
  }

  // converts `x` to a message, serialize it, then deserializes it, and
  // finally returns unboxed value
  template <class T>
  T msg_roundtrip(const T& x) {
    message result;
    auto tmp = make_message(x);
    deserialize(serialize(tmp), result);
    CAF_REQUIRE(result.match_elements<T>());
    return result.get_as<T>(0);
  }

  fixture()
      : system(cfg),
        context(&system) {
    rs.str.assign(string(str.rbegin(), str.rend()));
    msg = make_message(i32, te, str, rs);
  }
};

struct is_message {
  explicit is_message(message& msgref) : msg(msgref) {
    // nop
  }

  message& msg;

  template <class T, class... Ts>
  bool equal(T&& v, Ts&&... vs) {
    bool ok = false;
    // work around for gcc 4.8.4 bug
    auto tup = tie(v, vs...);
    message_handler impl {
      [&](T const& u, Ts const&... us) {
        ok = tup == tie(u, us...);
      }
    };
    impl(msg);
    return ok;
  }
};

} // namespace <anonymous>

CAF_TEST_FIXTURE_SCOPE(serialization_tests, fixture)

CAF_TEST(ieee_754_conversion) {
  // check conversion of float
  float f1 = 3.1415925f;         // float value
  auto p1 = caf::detail::pack754(f1); // packet value
  CAF_CHECK_EQUAL(p1, static_cast<decltype(p1)>(0x40490FDA));
  auto u1 = caf::detail::unpack754(p1); // unpacked value
  CAF_CHECK_EQUAL(f1, u1);
  // check conversion of double
  double f2 = 3.14159265358979311600;  // double value
  auto p2 = caf::detail::pack754(f2); // packet value
  CAF_CHECK_EQUAL(p2, static_cast<decltype(p2)>(0x400921FB54442D18));
  auto u2 = caf::detail::unpack754(p2); // unpacked value
  CAF_CHECK_EQUAL(f2, u2);
}

CAF_TEST(i32_values) {
  auto buf = serialize(i32);
  int32_t x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(i32, x);
}

CAF_TEST(float_values) {
  auto buf = serialize(f32);
  float x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(f32, x);
}

CAF_TEST(double_values) {
  auto buf = serialize(f64);
  double x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(f64, x);
}

CAF_TEST(enum_classes) {
  auto buf = serialize(te);
  test_enum x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(te, x);
}

CAF_TEST(strings) {
  auto buf = serialize(str);
  string x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(str, x);
}

CAF_TEST(custom_struct) {
  auto buf = serialize(rs);
  raw_struct x;
  deserialize(buf, x);
  CAF_CHECK_EQUAL(rs, x);
}

CAF_TEST(atoms) {
  auto foo = atom("foo");
  CAF_CHECK_EQUAL(foo, roundtrip(foo));
  CAF_CHECK_EQUAL(foo, msg_roundtrip(foo));
  using bar_atom = atom_constant<atom("bar")>;
  CAF_CHECK_EQUAL(bar_atom::value, roundtrip(atom("bar")));
  CAF_CHECK_EQUAL(bar_atom::value, msg_roundtrip(atom("bar")));
}

CAF_TEST(raw_arrays) {
  auto buf = serialize(ra);
  int x[3];
  deserialize(buf, x);
  for (auto i = 0; i < 3; ++i)
    CAF_CHECK_EQUAL(ra[i], x[i]);
}

CAF_TEST(arrays) {
  auto buf = serialize(ta);
  test_array x;
  deserialize(buf, x);
  for (auto i = 0; i < 4; ++i)
    CAF_CHECK_EQUAL(ta.value[i], x.value[i]);
  for (auto i = 0; i < 2; ++i)
    for (auto j = 0; j < 4; ++j)
      CAF_CHECK_EQUAL(ta.value2[i][j], x.value2[i][j]);
}

CAF_TEST(empty_non_pods) {
  test_empty_non_pod x;
  auto buf = serialize(x);
  CAF_REQUIRE(buf.empty());
  deserialize(buf, x);
}

std::string hexstr(const std::vector<char>& buf) {
  using namespace std;
  ostringstream oss;
  oss << hex;
  oss.fill('0');
  for (auto& c : buf) {
    oss.width(2);
    oss << int{c};
  }
  return oss.str();
}

CAF_TEST(messages) {
  // serialize original message which uses tuple_vals internally and
  // deserialize into a message which uses type_erased_value pointers
  message x;
  auto buf1 = serialize(msg);
  deserialize(buf1, x);
  CAF_CHECK_EQUAL(to_string(msg), to_string(x));
  CAF_CHECK(is_message(x).equal(i32, te, str, rs));
  // serialize fully dynamic message again (do another roundtrip)
  message y;
  auto buf2 = serialize(x);
  CAF_CHECK_EQUAL(buf1, buf2);
  deserialize(buf2, y);
  CAF_CHECK_EQUAL(to_string(msg), to_string(y));
  CAF_CHECK(is_message(y).equal(i32, te, str, rs));
}

/*
CAF_TEST(actor_addr_via_message) {
  auto testee = system.spawn([] {});
  auto addr = actor_cast<actor_addr>(testee);
  auto addr_msg = make_message(addr);
  // serialize original message which uses tuple_vals and
  // deserialize into a message which uses message builder
  message x;
  deserialize(serialize(addr_msg), x);
  CAF_CHECK_EQUAL(to_string(addr_msg), to_string(x));
  CAF_CHECK(is_message(x).equal(addr));
  // serialize fully dynamic message again (do another roundtrip)
  message y;
  deserialize(serialize(x), y);
  CAF_CHECK_EQUAL(to_string(addr_msg), to_string(y));
  CAF_CHECK(is_message(y).equal(addr));
}
*/

CAF_TEST(multiple_messages) {
  auto m = make_message(rs, te);
  auto buf = serialize(te, m, msg);
  test_enum t;
  message m1;
  message m2;
  deserialize(buf, t, m1, m2);
  CAF_CHECK_EQUAL(std::make_tuple(t, to_string(m1), to_string(m2)),
                  std::make_tuple(te, to_string(m), to_string(msg)));
  CAF_CHECK(is_message(m1).equal(rs, te));
  CAF_CHECK(is_message(m2).equal(i32, te, str, rs));
}


CAF_TEST(type_erased_value) {
  auto buf = serialize(str);
  type_erased_value_ptr ptr{new type_erased_value_impl<std::string>};
  binary_deserializer bd{&context, buf.data(), buf.size()};
  ptr->load(bd);
  CAF_CHECK_EQUAL(str, *reinterpret_cast<const std::string*>(ptr->get()));
}

CAF_TEST(type_erased_view) {
  auto str_view = make_type_erased_view(str);
  auto buf = serialize(str_view);
  std::string res;
  deserialize(buf, res);
  CAF_CHECK_EQUAL(str, res);
}

CAF_TEST(type_erased_tuple) {
  auto tview = make_type_erased_tuple_view(str, i32);
  CAF_CHECK_EQUAL(to_string(tview), deep_to_string(std::make_tuple(str, i32)));
  auto buf = serialize(tview);
  CAF_REQUIRE(buf.size() > 0);
  std::string tmp1;
  int32_t tmp2;
  deserialize(buf, tmp1, tmp2);
  CAF_CHECK_EQUAL(tmp1, str);
  CAF_CHECK_EQUAL(tmp2, i32);
  deserialize(buf, tview);
  CAF_CHECK_EQUAL(to_string(tview), deep_to_string(std::make_tuple(str, i32)));
}

CAF_TEST(streambuf_serialization) {
  auto data = std::string{"The quick brown fox jumps over the lazy dog"};
  std::vector<char> buf;
  // First, we check the standard use case in CAF where stream serializers own
  // their stream buffers.
  stream_serializer<vectorbuf> bs{vectorbuf{buf}};
  bs << data;
  stream_deserializer<charbuf> bd{charbuf{buf}};
  std::string target;
  bd >> target;
  CAF_CHECK(data == target);
  // Second, we test another use case where the serializers only keep
  // references of the stream buffers.
  buf.clear();
  target.clear();
  vectorbuf vb{buf};
  stream_serializer<vectorbuf&> vs{vb};
  vs << data;
  charbuf cb{buf};
  stream_deserializer<charbuf&> vd{cb};
  vd >> target;
  CAF_CHECK(data == target);
}

CAF_TEST(byte_sequence_optimization) {
  std::vector<uint8_t> data(42);
  std::fill(data.begin(), data.end(), 0x2a);
  std::vector<uint8_t> buf;
  using streambuf_type = containerbuf<std::vector<uint8_t>>;
  streambuf_type cb{buf};
  stream_serializer<streambuf_type&> bs{cb};
  bs << data;
  data.clear();
  streambuf_type cb2{buf};
  stream_deserializer<streambuf_type&> bd{cb2};
  bd >> data;
  CAF_CHECK_EQUAL(data.size(), 42u);
  CAF_CHECK(std::all_of(data.begin(), data.end(),
                        [](uint8_t c) { return c == 0x2a; }));
}

CAF_TEST_FIXTURE_SCOPE_END()
