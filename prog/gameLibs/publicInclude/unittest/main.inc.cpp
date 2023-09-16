#include <UnitTest++/UnitTestPP.h>
#include <UnitTest++/TestReporterStdout.h>

#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_fatal.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_symHlp.h>
#ifdef USE_EASTL
#include <EASTL/internal/config.h>
#endif

static bool fatal_handler(const char *msg, const char * /*call_stack*/, const char *origin_file, int origin_line)
{
  UnitTest::ReportAssert(msg, origin_file, origin_line);
  return false;
}
static bool assertion_handler(bool /*verify*/, const char *file, int line, const char * /*func*/, const char *cond, const char *fmt,
  const DagorSafeArg *args, int anum)
{
  String totalMessage(0, "dagor assertion \"%s\" failed", cond);
  if (fmt)
  {
    totalMessage.append(" with message:\n");
    totalMessage.avprintf(0, fmt, args, anum);
  }
  UnitTest::ReportAssert(totalMessage.c_str(), file, line);
  return false;
}

#if defined(USE_EASTL) && EASTL_ASSERT_ENABLED
static void eastl_assertion_failure_function(const char *expr, void * /*ctx*/) { UnitTest::ReportAssert(expr, "", 0); }
#endif

int __cdecl main(int argc, char const *argv[])
{
  ::symhlp_init_default();
  ::dgs_fatal_handler = fatal_handler;
  ::dgs_assertion_handler = assertion_handler;
#if defined(USE_EASTL) && EASTL_ASSERT_ENABLED
  eastl::SetAssertionFailureFunction(&eastl_assertion_failure_function, NULL);
#endif

#ifdef CUSTOM_UNITTEST_CODE
  CUSTOM_UNITTEST_CODE
#endif

  const char *suiteName = nullptr;

  if (argc >= 2)
  {
    if (strcmp(argv[1], "*") != 0)
      suiteName = argv[1];
  }

  auto predicate = [argc, argv](const UnitTest::Test *test) {
    if (argc < 3)
      return true;

    for (int i = 2; i < argc; ++i)
    {
      if (strcmp(argv[i], test->m_details.testName) == 0)
        return true;
    }

    return false;
  };

  UnitTest::TestReporterStdout reporter;
  UnitTest::TestRunner runner(reporter);
  return runner.RunTestsIf(UnitTest::Test::GetTestList(), suiteName, predicate, 0);
}
