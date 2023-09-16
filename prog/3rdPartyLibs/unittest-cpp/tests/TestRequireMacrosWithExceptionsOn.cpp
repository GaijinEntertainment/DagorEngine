#include "UnitTest++/UnitTestPP.h"
#include "UnitTest++/CurrentTest.h"
#include "RecordingReporter.h"
#include "ScopedCurrentTest.h"

using namespace std;

#ifndef UNITTEST_NO_EXCEPTIONS

namespace {

   TEST(RequireCheckSucceedsOnTrue)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);

         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK(true);
         }
         catch(const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckFailsOnFalse)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK(false);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }


   TEST(RequireMacroSupportsMultipleChecks)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try{
            REQUIRE
            {
               CHECK(true);
               CHECK_EQUAL(1,1);
            }
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }


   TEST(RequireMacroSupportsMultipleChecksWithFailingChecks)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try{
            REQUIRE
            {
               CHECK(true);
               CHECK_EQUAL(1,2);
            }
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequireMacroDoesntExecuteCodeAfterAFailingCheck)
   {
      bool failure = false;
      bool exception = false;
      bool run = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try{
            REQUIRE
            {
               CHECK(false);
               run = true;     // this shouldn't get executed.
            }
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
      CHECK(!run);
   }

   TEST(FailureReportsCorrectTestName)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK(false);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL(m_details.testName, reporter.lastFailedTest);
   }

   TEST(RequiredCheckFailureIncludesCheckContents)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);
         const bool yaddayadda = false;

         try
         {
            REQUIRE CHECK(yaddayadda);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK(strstr(reporter.lastFailedMessage, "yaddayadda"));
   }

   TEST(RequiredCheckEqualSucceedsOnEqual)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_EQUAL(1,1);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckEqualFailsOnNotEqual)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_EQUAL(1, 2);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequiredCheckEqualFailureContainsCorrectDetails)
   {
      int line = 0;
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails const testDetails("testName", "suiteName", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         try
         {
            line = __LINE__; REQUIRE CHECK_EQUAL(1, 123);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

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

         try
         {
            REQUIRE CHECK_EQUAL(1, FunctionWithSideEffects());
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckEqualDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_EQUAL(2, FunctionWithSideEffects());
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }


   TEST(RequiredCheckCloseSucceedsOnEqual)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_CLOSE(1.0f, 1.001f, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckCloseFailsOnNotEqual)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_CLOSE (1.0f, 1.1f, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequiredCheckCloseFailureContainsCorrectDetails)
   {
      int line = 0;
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("test", "suite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         try
         {
            line = __LINE__; REQUIRE CHECK_CLOSE(1.0f, 1.1f, 0.01f);
            CHECK(false);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL("test", reporter.lastFailedTest);
      CHECK_EQUAL("suite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   TEST(RequiredCheckCloseDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_CLOSE (1, FunctionWithSideEffects(), 0.1f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         try
         {
            REQUIRE CHECK_CLOSE(2, FunctionWithSideEffects(), 0.1f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckArrayCloseSucceedsOnEqual)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);
         const float data[4] = { 0, 1, 2, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_CLOSE (data, data, 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckArrayCloseFailsOnNotEqual)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequiredCheckArrayCloseFailureIncludesCheckExpectedAndActual)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_CLOSE(data1, data2, 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
   }

   TEST(RequiredCheckArrayCloseFailureContainsCorrectDetails)
   {
      int line = 0;
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("arrayCloseTest", "arrayCloseSuite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            line = __LINE__; REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL("arrayCloseTest", reporter.lastFailedTest);
      CHECK_EQUAL("arrayCloseSuite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   TEST(RequiredCheckArrayCloseFailureIncludesTolerance)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         float const data1[4] = { 0, 1, 2, 3 };
         float const data2[4] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_CLOSE (data1, data2, 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK(strstr(reporter.lastFailedMessage, "0.01"));
   }

   TEST(RequiredCheckArrayEqualSuceedsOnEqual)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         const float data[4] = { 0, 1, 2, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_EQUAL (data, data, 4);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckArrayEqualFailsOnNotEqual)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequiredCheckArrayEqualFailureIncludesCheckExpectedAndActual)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK(strstr(reporter.lastFailedMessage, "xpected [ 0 1 2 3 ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ 0 1 3 3 ]"));
   }

   TEST(RequiredCheckArrayEqualFailureContainsCorrectInfo)
   {
      int line = 0;
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[4] = { 0, 1, 2, 3 };
         int const data2[4] = { 0, 1, 3, 3 };

         try
         {
            line = __LINE__; REQUIRE CHECK_ARRAY_EQUAL (data1, data2, 4);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL("RequiredCheckArrayEqualFailureContainsCorrectInfo", reporter.lastFailedTest);
      CHECK_EQUAL(__FILE__, reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   float const* FunctionWithSideEffects2()
   {
      ++g_sideEffect;
      static float const data[] = {1,2,3,4};
      return data;
   }

   TEST(RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenPassing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[] = { 1, 2, 3, 4 };

         REQUIRE CHECK_ARRAY_CLOSE (data, FunctionWithSideEffects2(), 4, 0.01f);
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckArrayCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[] = { 0, 1, 3, 3 };

         try
         {
            REQUIRE CHECK_ARRAY_CLOSE (data, FunctionWithSideEffects2(), 4, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckArray2DCloseSucceedsOnEqual)
   {
      bool failure = true;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         const float data[2][2] = { {0, 1}, {2, 3} };

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE(data, data, 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(!failure);
      CHECK(!exception);
   }

   TEST(RequiredCheckArray2DCloseFailsOnNotEqual)
   {
      bool failure = false;
      bool exception = false;
      {
         RecordingReporter reporter;
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {
            exception = true;
         }

         failure = (testResults.GetFailureCount() > 0);
      }

      CHECK(failure);
      CHECK(exception);
   }

   TEST(RequiredCheckArray2DCloseFailureIncludesCheckExpectedAndActual)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK(strstr(reporter.lastFailedMessage, "xpected [ [ 0 1 ] [ 2 3 ] ]"));
      CHECK(strstr(reporter.lastFailedMessage, "was [ [ 0 1 ] [ 3 3 ] ]"));
   }

   TEST(RequiredCheckArray2DCloseFailureContainsCorrectDetails)
   {
      int line = 0;
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         UnitTest::TestDetails testDetails("array2DCloseTest", "array2DCloseSuite", "filename", -1);
         ScopedCurrentTest scopedResults(testResults, &testDetails);

         int const data1[2][2] = { {0, 1}, {2, 3} };
         int const data2[2][2] = { {0, 1}, {3, 3} };

         try
         {
            line = __LINE__; REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

      CHECK_EQUAL("array2DCloseTest", reporter.lastFailedTest);
      CHECK_EQUAL("array2DCloseSuite", reporter.lastFailedSuite);
      CHECK_EQUAL("filename", reporter.lastFailedFile);
      CHECK_EQUAL(line, reporter.lastFailedLine);
   }

   TEST(RequiredCheckArray2DCloseFailureIncludesTolerance)
   {
      RecordingReporter reporter;
      {
         UnitTest::TestResults testResults(&reporter);
         ScopedCurrentTest scopedResults(testResults);

         float const data1[2][2] = { {0, 1}, {2, 3} };
         float const data2[2][2] = { {0, 1}, {3, 3} };

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE (data1, data2, 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }

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

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE (data, FunctionWithSideEffects3(), 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

   TEST(RequiredCheckArray2DCloseDoesNotHaveSideEffectsWhenFailing)
   {
      g_sideEffect = 0;
      {
         UnitTest::TestResults testResults;
         ScopedCurrentTest scopedResults(testResults);

         const float data[2][2] = { {0, 1}, {3, 3} };

         try
         {
            REQUIRE CHECK_ARRAY2D_CLOSE (data, FunctionWithSideEffects3(), 2, 2, 0.01f);
         }
         catch (const UnitTest::RequiredCheckException&)
         {}
      }
      CHECK_EQUAL(1, g_sideEffect);
   }

}

#endif
