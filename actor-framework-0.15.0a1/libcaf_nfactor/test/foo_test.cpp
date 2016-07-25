#include "caf/config.hpp"
#define CAF_SUITE fb
#include "caf/test/unit_test.hpp"

#include "caf/all.hpp"
#include "caf/nfactor/foo.hpp"

using namespace caf::nfactor;

namespace{
  constexpr const char* hehe = "hehe";

  struct fixture{
    fixture():f(){};
    foo f;
  };
} //anonymous name space

CAF_TEST_FIXTURE_SCOPE(foobar_tests, fixture)

CAF_TEST(foobar_num1){
  CAF_CHECK_EQUAL(f.bar(), 1);
}

CAF_TEST(foobar_num2){
  CAF_CHECK_NOT_EQUAL(f.bar(), 0);
}

CAF_TEST(foobar_num3){
  CAF_CHECK_NOT_EQUAL(f.bar(), 0);
}

CAF_TEST_FIXTURE_SCOPE_END()
