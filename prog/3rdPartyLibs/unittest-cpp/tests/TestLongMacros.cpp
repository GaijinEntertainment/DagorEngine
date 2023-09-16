#define UNITTEST_DISABLE_SHORT_MACROS

#include "UnitTest++/UnitTestPP.h"

// This file is not intended to test every little thing, just a few basics to hopefully ensure
// the macros are working and the short macros are not defined.
UNITTEST_SUITE(LongMacros)
{
   UNITTEST_TEST(LongCheckMacroWorks)
   {
      UNITTEST_CHECK(true);
   }

   class Fixture
   {
   public:
      Fixture() : sanity_(true) {}
   protected:
      bool sanity_;
   };

   UNITTEST_TEST_FIXTURE(Fixture, LongFixtureMacroWorks)
   {
      UNITTEST_REQUIRE UNITTEST_CHECK(sanity_);
   }

   UNITTEST_TEST(ShortMacrosAreNotDefined)
   {
#if defined(CHECK) || \
    defined(CHECK_EQUAL) || \
    defined(CHECK_CLOSE) || \
    defined(CHECK_ARRAY_EQUAL) || \
    defined(CHECK_ARRAY_CLOSE) || \
    defined(CHECK_ARRAY2D_CLOSE) || \
    defined(CHECK_THROW) || \
    defined(CHECK_ASSERT) || \
    defined(SUITE) || \
    defined(TEST) || \
    defined(TEST_FIXTURE) || \
    defined(REQUIRE)

      UNITTEST_CHECK(false);
#endif
   }
}
