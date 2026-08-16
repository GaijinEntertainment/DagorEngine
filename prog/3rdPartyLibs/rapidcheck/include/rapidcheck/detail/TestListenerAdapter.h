#pragma once

#include "rapidcheck/detail/TestListener.h"

namespace rc {
namespace detail {

/// `TestListener` that has no-op default implementations of all methods.
class TestListenerAdapter : public TestListener {
public:
  void onTestCaseFinished(const CaseDescription &/*description*/) override {}
  void onShrinkTried(const CaseDescription &/*shrink*/, bool /*accepted*/) override {}
  void onTestFinished(const TestMetadata &/*metadata*/, const TestResult &/*result*/) override {}
};

} // namespace detail
} // namespace rc
