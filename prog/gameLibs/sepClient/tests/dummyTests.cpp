// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClient.h>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("Dummy test", "[dummy]") // [dummy] is a tag for the test
{
  int a = 1;
  int b = 2;
  CHECK(a != b);
  CHECK(a == a);
  CHECK(a < b);
}


TEST_CASE("Dummy test", "[dummy2]") // the same test name is ok once tag differs
{
  int a = 10;
  int b = 20;
  CHECK(a != b);
  CHECK(a == a);
  CHECK(a < b);
}


static void verify_size(const std::vector<std::string> &v, size_t expected_size)
{
  if (expected_size == 0)
  {
    CHECK(v.empty());
  }
  else
  {
    CHECK(!v.empty());
  }
  CHECK(v.capacity() >= expected_size);
  REQUIRE(v.size() == expected_size);
}


SCENARIO("Vector can be sized and resized", "[dummy]")
{
  GIVEN("An empty vector")
  {
    auto v = std::vector<std::string>{};

    THEN("The size and capacity start at 0") { verify_size(v, 0); }

    WHEN("push_back() is called")
    {
      v.push_back("hullo");

      THEN("The size changes") { verify_size(v, 1); }
    }

    WHEN("push_back() is called")
    {
      v.push_back("hullo2");

      THEN("The size changes") { verify_size(v, 1); }

      WHEN("push_back() is called again")
      {
        v.push_back("hullo3");

        THEN("The size changes") { verify_size(v, 2); }
      }
    }
  }
}
