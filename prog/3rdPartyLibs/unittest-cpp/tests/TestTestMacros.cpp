#include "UnitTest++/UnitTestPP.h"
#include "UnitTest++/TestMacros.h"
#include "UnitTest++/TestList.h"
#include "UnitTest++/TestResults.h"
#include "UnitTest++/TestReporter.h"
#include "UnitTest++/ReportAssert.h"
#include "RecordingReporter.h"
#include "ScopedCurrentTest.h"

using namespace UnitTest;
using namespace std;

/* test for c++11 support */
#ifndef _MSC_VER

   /* Test for clang >= 3.3 */
   #ifdef __clang__
      #if (__clang__ == 1) && (__clang_major__ > 3 || (__clang_major__ == 3 && (__clang_minor__ > 2 )))
         #define _NOEXCEPT_OP(x) noexcept(x)
      #else
         #define _NOEXCEPT_OP(x)
      #endif
   #endif

   #ifndef __clang__
      /* Test for GCC >= 4.8.0 */
      #ifdef __GNUC__
         #if (__GNUC__ > 4) || (__GNUC__ == 4 && (__GNUC_MINOR__ > 7 ))
            #define _NOEXCEPT_OP(x) noexcept(x)
         #else
            #define _NOEXCEPT_OP(x)
         #endif
      #endif
   #endif

#elif _MSC_VER

   #if (_MSC_VER > 1800)
      #define _NOEXCEPT_OP(x) noexcept(x)
   #else
      #define _NOEXCEPT_OP(x)
   #endif

#endif

namespace {

   TestList list1;
   UNITTEST_IMPL_TEST(DummyTest, list1)
   {}

   TEST (TestsAreAddedToTheListThroughMacro)
   {
      CHECK(list1.GetHead() != 0);
      CHECK(list1.GetHead()->m_nextTest == 0);
   }

#ifndef UNITTEST_NO_EXCEPTIONS

   struct ThrowingThingie
   {
      ThrowingThingie() : dummy(false)
      {
         if (!dummy)
            throw "Oops";
      }

      bool dummy;
   };

   TestList list2;
   UNITTEST_IMPL_TEST_FIXTURE(ThrowingThingie, DummyTestName, list2)
   {}

   TEST (ExceptionsInFixtureAreReportedAsHappeningInTheFixture)
   {
      RecordingReporter reporter;
      TestResults result(&reporter);
      {
         ScopedCurrentTest scopedResults(result);
         list2.GetHead()->Run();
      }

      CHECK(strstr(reporter.lastFailedMessage, "xception"));
      CHECK(strstr(reporter.lastFailedMessage, "fixture"));
      CHECK(strstr(reporter.lastFailedMessage, "ThrowingThingie"));
   }

#endif

   struct DummyFixture
   {
      int x;
   };

// We're really testing the macros so we just want them to compile and link
   SUITE(TestSuite1)
   {
      TEST(SimilarlyNamedTestsInDifferentSuitesWork)
      {}

      TEST_FIXTURE(DummyFixture, SimilarlyNamedFixtureTestsInDifferentSuitesWork)
      {}
   }

   SUITE(TestSuite2)
   {
      TEST(SimilarlyNamedTestsInDifferentSuitesWork)
      {}

      TEST_FIXTURE(DummyFixture,SimilarlyNamedFixtureTestsInDifferentSuitesWork)
      {}
   }

   TestList macroTestList1;
   UNITTEST_IMPL_TEST(MacroTestHelper1, macroTestList1)
   {}

   TEST(TestAddedWithTEST_EXMacroGetsDefaultSuite)
   {
      CHECK(macroTestList1.GetHead() != NULL);
      CHECK_EQUAL ("MacroTestHelper1", macroTestList1.GetHead()->m_details.testName);
      CHECK_EQUAL ("DefaultSuite", macroTestList1.GetHead()->m_details.suiteName);
   }

   TestList macroTestList2;
   UNITTEST_IMPL_TEST_FIXTURE(DummyFixture, MacroTestHelper2, macroTestList2)
   {}

   TEST(TestAddedWithTEST_FIXTURE_EXMacroGetsDefaultSuite)
   {
      CHECK(macroTestList2.GetHead() != NULL);
      CHECK_EQUAL ("MacroTestHelper2", macroTestList2.GetHead()->m_details.testName);
      CHECK_EQUAL ("DefaultSuite", macroTestList2.GetHead()->m_details.suiteName);
   }

#ifndef UNITTEST_NO_EXCEPTIONS

   struct FixtureCtorThrows
   {
      FixtureCtorThrows() {
         throw "exception";
      }
   };

   TestList throwingFixtureTestList1;
   UNITTEST_IMPL_TEST_FIXTURE(FixtureCtorThrows, FixtureCtorThrowsTestName, throwingFixtureTestList1)
   {}

   TEST(FixturesWithThrowingCtorsAreFailures)
   {
      CHECK(throwingFixtureTestList1.GetHead() != NULL);
      RecordingReporter reporter;
      TestResults result(&reporter);
      {
         ScopedCurrentTest scopedResult(result);
         throwingFixtureTestList1.GetHead()->Run();
      }

      int const failureCount = result.GetFailedTestCount();
      CHECK_EQUAL(1, failureCount);
      CHECK(strstr(reporter.lastFailedMessage, "while constructing fixture"));
   }

   struct FixtureDtorThrows
   {
      ~FixtureDtorThrows() _NOEXCEPT_OP(false) {
         throw "exception";
      }
   };

   TestList throwingFixtureTestList2;
   UNITTEST_IMPL_TEST_FIXTURE(FixtureDtorThrows, FixtureDtorThrowsTestName, throwingFixtureTestList2)
   {}

   TEST(FixturesWithThrowingDtorsAreFailures)
   {
      CHECK(throwingFixtureTestList2.GetHead() != NULL);

      RecordingReporter reporter;
      TestResults result(&reporter);
      {
         ScopedCurrentTest scopedResult(result);
         throwingFixtureTestList2.GetHead()->Run();
      }

      int const failureCount = result.GetFailedTestCount();
      CHECK_EQUAL(1, failureCount);
      CHECK(strstr(reporter.lastFailedMessage, "while destroying fixture"));
   }

   const int FailingLine = 123;

   struct FixtureCtorAsserts
   {
      FixtureCtorAsserts()
      {
         UnitTest::ReportAssert("assert failure", "file", FailingLine);
      }
   };

   TestList ctorAssertFixtureTestList;
   UNITTEST_IMPL_TEST_FIXTURE(FixtureCtorAsserts, CorrectlyReportsAssertFailureInCtor, ctorAssertFixtureTestList)
   {}

   TEST(CorrectlyReportsFixturesWithCtorsThatAssert)
   {
      RecordingReporter reporter;
      TestResults result(&reporter);
      {
         ScopedCurrentTest scopedResults(result);
         ctorAssertFixtureTestList.GetHead()->Run();
      }

      const int failureCount = result.GetFailedTestCount();
      CHECK_EQUAL(1, failureCount);
      CHECK_EQUAL(FailingLine, reporter.lastFailedLine);
      CHECK(strstr(reporter.lastFailedMessage, "assert failure"));
   }

#endif

}

// We're really testing if it's possible to use the same suite in two files
// to compile and link successfuly (TestTestSuite.cpp has suite with the same name)
// Note: we are outside of the anonymous namespace
SUITE(SameTestSuite)
{
   TEST(DummyTest1)
   {}
}

#define CUR_TEST_NAME CurrentTestDetailsContainCurrentTestInfo
#define INNER_STRINGIFY(X) #X
#define STRINGIFY(X) INNER_STRINGIFY(X)

TEST(CUR_TEST_NAME)
{
   const UnitTest::TestDetails* details = CurrentTest::Details();
   CHECK_EQUAL(STRINGIFY(CUR_TEST_NAME), details->testName);
}

#undef CUR_TEST_NAME
#undef INNER_STRINGIFY
#undef STRINGIFY
