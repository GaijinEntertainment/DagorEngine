#pragma once

#include <sstream>

#include <rapidcheck.h>

// To support Catch2 v3 we check if the new header has already been included,
// otherwise we include the old header.
#ifndef CATCH_TEST_MACROS_HPP_INCLUDED
  #include <catch2/catch.hpp>
#endif

namespace rc {

/// For use with `catch2/catch.hpp`. Use this function wherever you would use a
/// `SECTION` for convenient checking of properties.
///
/// @param description  A description of the property.
/// @param testable     The object that implements the property.
template <typename Testable>
void prop(const std::string &description, Testable &&testable, bool verbose=false) {
  using namespace detail;

#ifdef CATCH_CONFIG_PREFIX_ALL
  CATCH_SECTION(description) {
#else
  SECTION(description) {
#endif

    const auto result = checkTestable(std::forward<Testable>(testable));

    if (result.template is<SuccessResult>()) {
      const auto success = result.template get<SuccessResult>();
      if (verbose || !success.distribution.empty()) {
        std::cout << "- " << description << std::endl;
        printResultMessage(result, std::cout);
        std::cout << std::endl;
      }
#ifdef CATCH_CONFIG_PREFIX_ALL
      CATCH_SUCCEED();
#else
      SUCCEED();
#endif
    } else {
      std::ostringstream ss;
      printResultMessage(result, ss);
#ifdef CATCH_CONFIG_PREFIX_ALL
      CATCH_INFO(ss.str() << "\n");
      CATCH_FAIL();
#else
      INFO(ss.str() << "\n");
      FAIL();
#endif
    }
  }
}

} // namespace rc
