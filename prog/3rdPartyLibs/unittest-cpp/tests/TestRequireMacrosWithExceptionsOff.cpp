#include "UnitTest++/UnitTestPP.h"
#include "UnitTest++/CurrentTest.h"
#include "RecordingReporter.h"
#include "ScopedCurrentTest.h"

#include <cstring>

using namespace std;

#ifdef UNITTEST_NO_EXCEPTIONS

// NOTE: unit tests here use a work around for std::longjmp
// taking us out of the current running unit test. We use a
// follow on test to check the previous test exhibited correct
// behavior.

namespace {

   static RecordingReporter reporter;
   static std::string testName;
   static bool next = false;
   static int line = 0;
   
   // Use destructor to reset our globals
   struct DoValidationOn
   {
      ~DoValidationOn()
      {
         testName = "";
         next = false;
         line = 0;
         
         reporter.lastFailedLine = 0;
         memset(reporter.lastFailedTest, 0, sizeof(reporter.lastFailedTest));
         memset(reporter.lastFailedSuite, 0, sizeof(reporter.lastFailedSuite));
         memset(reporter.lastFailedFile, 0, sizeof(reporter.lastFailedFile));
      }
   };
   
   TEST(RequireCheckSucceedsOnTrue)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK(true);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequireCheckSucceedsOnTrue_FollowOn)
   {
      CHECK(next);
   }

   TEST(RequiredCheckFailsOnFalse)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK(false);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckFailsOnFalse_FollowOn)
   {
      CHECK(!next);
   }

   TEST(RequireMacroSupportsMultipleChecks)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE
         {
            CHECK(true);
            CHECK_EQUAL(1,1);
         }
         
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequireMacroSupportsMultipleChecks_FollowOn)
   {
      CHECK(next);
   }

   TEST(RequireMacroSupportsMultipleChecksWithFailingChecks)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE
         {
            CHECK(true);
            CHECK_EQUAL(1,2);
         }
         
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequireMacroSupportsMultipleChecksWithFailingChecks_FollowOn)
   {
      CHECK(!next);
   }

   TEST(RequireMacroDoesntExecuteCodeAfterAFailingCheck)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE
         {
            CHECK(false);
            next = true;
         }
      }
   }

   TEST_FIXTURE(DoValidationOn, RequireMacroDoesntExecuteCodeAfterAFailingCheck_FollowOn)
   {
      CHECK(!next);
   }
   
   TEST(FailureReportsCorrectTestName)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);
    
         testName = m_details.testName;
         REQUIRE CHECK(false);
      }
   }

   TEST_FIXTURE(DoValidationOn, FailureReportsCorrectTestName_FollowOn)
   {
      CHECK_EQUAL(testName, reporter.lastFailedTest);
   }

   TEST(RequiredCheckFailureIncludesCheckContents)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);
  
         testName = m_details.testName;
         const bool yaddayadda = false;

         REQUIRE CHECK(yaddayadda);
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckFailureIncludesCheckContents_FollowOn)
   {
      CHECK(strstr(reporter.lastFailedMessage, "yaddayadda"));
   }
   
   TEST(RequiredCheckEqualSucceedsOnEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_EQUAL(1,1);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckEqualSucceedsOnEqual_FollowOn)
   {
      CHECK(next);
   }

   TEST(RequiredCheckEqualFailsOnNotEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_EQUAL(1, 2);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckEqualFailsOnNotEqual_FollowOn)
   {
// TODO: check reporter last test name
      CHECK(!next);
   }

   TEST(RequiredCheckEqualFailureContainsCorrectDetails)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails const testDetails("testName", "suiteName", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         line = __LINE__; REQUIRE CHECK_EQUAL(1, 123);
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckEqualFailureContainsCorrectDetails_FollowOn)
   {
      CHECK_EQUAL("testName", reporter.lastFailedTest);
      CHECK_EQUAL("suiteName", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   int g_sideEffect = 0;
   int FunctionWithSideEffects()
   {
      ++g_sideEffect;
      return 1;
   }

   TEST(RequiredCheckEqualDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_EQUAL(1, FunctionWithSideEffects());
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckEqualDoesNotHaveSideEffectsWhenPassing_FollowOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(next);
   }
   
   TEST(RequiredCheckEqualDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_EQUAL(2, FunctionWithSideEffects());
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckEqualDoesNotHaveSideEffectsWhenFailing_FollowOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(!next);
   }

   TEST(RequiredCheckCloseSucceedsOnEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_CLOSE(1.0f, 1.001f, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckCloseSucceedsOnEqual_FollowOn)
   {
      CHECK(next);
   }

   TEST(RequiredCheckCloseFailsOnNotEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_CLOSE (1.0f, 1.1f, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckCloseFailsOnNotEqual_FollowOn)
   {
      CHECK(!next);
   }

   TEST(RequiredCheckCloseFailureContainsCorrectDetails)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("test", "suite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         line = __LINE__; REQUIRE CHECK_CLOSE(1.0f, 1.1f, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckCloseFailureContainsCorrectDetails_FollowOn)
   {
      CHECK_EQUAL("test", reporter.lastFailedTest);
      CHECK_EQUAL("suite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
      
      CHECK(!next);
   }
   
   TEST(RequiredCheckCloseDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_CLOSE (1, FunctionWithSideEffects(), 0.1f);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckCloseDoesNotHaveSideEffectsWhenPassing_FollowOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(next);
   }

   TEST(RequiredCheckCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         REQUIRE CHECK_CLOSE(2, FunctionWithSideEffects(), 0.1f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckCloseDoesNotHaveSideEffectsWhenFailingOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(!next);
   }

   TEST(RequiredCheckArrayCloseSucceedsOnEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);
         const float data[4] = { 0, 1, 2, 3 };

         REQUIRE CHECK_ARRAY_CLOSE (data, data, 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseSucceedsOnEqual_FollowOn)
   {
      CHECK(next);
   }
   
   TEST(RequiredCheckArrayCloseFailsOnNotEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseFailsOnNotEqual_FollowOn)
   {
      CHECK(!next);
   }
   
   TEST(RequiredCheckArrayCloseFailureIncludesCheckExpectedAndActual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseFailureIncludesCheckExpectedAndActual_FollowOn)
   {
      CHECK(!next);
      
      CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
   }
   
   TEST(RequiredCheckArrayCloseFailureContainsCorrectDetails)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("arrayCloseTest", "arrayCloseSuite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         line = __LINE__; REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseFailureContainsCorrectDetails_FollowOn)
   {
      CHECK(!next);
      
      CHECK_EQUAL("arrayCloseTest", reporter.lastFailedTest);
      CHECK_EQUAL("arrayCloseSuite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }
   
   TEST(RequiredCheckArrayCloseFailureIncludesTolerance)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         float const data1[4] = { 0, 1, 2, 3 };
         float const data2[4] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseFailureIncludesTolerance_FollowOn)
   {
      CHECK(!next);
      CHECK(strstr(reporter.lastFailedMessage, "0.01"));
   }
   
   TEST(RequiredCheckArrayEqualSuceedsOnEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         const float data[4] = { 0, 1, 2, 3 };

         REQUIRE CHECK_ARRAY_EQUAL (data, data, 4);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayEqualSuceedsOnEqual_FollowOn)
   {
      CHECK(next);
   }
   
   TEST(RequiredCheckArrayEqualFailsOnNotEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayEqualFailsOnNotEqual)
   {
      CHECK(!next);
   }

   TEST(RequiredCheckArrayEqualFailureIncludesCheckExpectedAndActual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayEqualFailureIncludesCheckExpectedAndActual_FollowOn)
   {
      CHECK(!next);
      
      CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
   }

   TEST(RequiredCheckArrayEqualFailureContainsCorrectInfo)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         line = __LINE__; REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayEqualFailureContainsCorrectInfo_FollowOn)
   {
      CHECK(!next);
      
      CHECK_EQUAL("RequiredCheckArrayEqualFailureContainsCorrectInfo", reporter.lastFailedTest);
      CHECK_EQUAL(__FILE__, reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   float const* FunctionWithSideEffects2()
   {
      ++g_sideEffect;
      static float const data[] = { 0, 1, 2, 3};
      return data;
   }

   TEST(RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[] = { 0, 1, 2, 3 };

         REQUIRE CHECK_ARRAY_CLOSE (data, FunctionWithSideEffects2(), 4, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenPassing_FollowOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(next);
   }

   TEST(RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[] = { 0, 1, 3, 3 };

         REQUIRE CHECK_ARRAY_CLOSE (data, FunctionWithSideEffects2(), 4, 0.01f);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenFailing_FollowOn)
   {
      CHECK_EQUAL(1, g_sideEffect);
      CHECK(!next);
   }

   TEST(RequiredCheckArray2DCloseSucceedsOnEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         const float data[2][2] = { {0, 1}, {2, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE(data, data, 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseSucceedsOnEqual_FollowOn)
   {
      CHECK(next);
   }
   
   TEST(RequiredCheckArray2DCloseFailsOnNotEqual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseFailsOnNotEqual_FollowOn)
   {
      CHECK(!next);
   }
   
   TEST(RequiredCheckArray2DCloseFailureIncludesCheckExpectedAndActual)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseFailureIncludesCheckExpectedAndActual_FollowOn)
   {
      CHECK(!next);
      
      CHECK(strstr(reporter.lastFailedMessage, "xpected [ [ 0 1 ] [ 2 3 ] ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ [ 0 1 ] [ 3 3 ] ]"));
   }
   
   TEST(RequiredCheckArray2DCloseFailureContainsCorrectDetails)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("array2DCloseTest", "array2DCloseSuite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         line = __LINE__; REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseFailureContainsCorrectDetails_FollowOn)
   {
      CHECK(!next);
      
      CHECK_EQUAL("array2DCloseTest", reporter.lastFailedTest);
      CHECK_EQUAL("array2DCloseSuite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }
   
   TEST(RequiredCheckArray2DCloseFailureIncludesTolerance)
   {
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         float const data1[2][2] = { {0, 1}, {2, 3} };
         float const data2[2][2] = { {0, 1}, {3, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseFailureIncludesTolerance_FollowOn)
   {
      CHECK(!next);
      CHECK(strstr(reporter.lastFailedMessage, "0.01"));
   }
   
   float const* const* FunctionWithSideEffects3()
   {
      ++g_sideEffect;
      static float const data1[] = {0,1};
      static float const data2[] = {2,3};
      static const float* const data[] = {data1, data2};
      return data;
   }

   TEST(RequiredCheckArray2DCloseDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[2][2] = { {0, 1}, {2, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE (data, FunctionWithSideEffects3(), 2, 2, 0.01f);
         next = true;
      }
   }
   
   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseDoesNotHaveSideEffectsWhenPassing_FollowOn)
   {
      CHECK(next);
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckArray2DCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[2][2] = { {0, 1}, {3, 3} };

         REQUIRE CHECK_ARRAY2D_CLOSE (data, FunctionWithSideEffects3(), 2, 2, 0.01f);
         next = true;
      }
   }

   TEST_FIXTURE(DoValidationOn, RequiredCheckArray2DCloseDoesNotHaveSideEffectsWhenFailing_FollowOn)
   {
      CHECK(!next);
      CHECK_EQUAL(1, g_sideEffect);
   }
}

#endif
