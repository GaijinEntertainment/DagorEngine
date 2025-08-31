// Copyright (C) Gaijin Games KFT.  All rights reserved.

// ===============================================================================

#include <unittest/catch2_reporter_detailed.h>

#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_symHlp.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>

#ifdef USE_EASTL
#include <EASTL/internal/config.h>
#endif

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/internal/catch_compiler_capabilities.hpp>
#include <catch2/internal/catch_config_wchar.hpp>
#include <catch2/internal/catch_enforce.hpp>
#include <catch2/internal/catch_leak_detector.hpp>
#include <catch2/internal/catch_platform.hpp>
#include <catch2/internal/catch_reporter_spec_parser.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

// ===============================================================================

CATCH_REGISTER_REPORTER("detailed", Catch::gaijin_reporters::DetailedConsoleReporter)

// ===============================================================================

namespace Catch
{
CATCH_INTERNAL_START_WARNINGS_SUPPRESSION
CATCH_INTERNAL_SUPPRESS_GLOBALS_WARNINGS
static LeakDetector leakDetector;
CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION
} // namespace Catch

// ===============================================================================

static bool fatal_handler(const char *msg, const char *call_stack, const char *origin_file, int origin_line)
{
  FAIL(origin_file << "(" << origin_line << "): Fatal handler error: " << msg
                   << "; callstack: " << (call_stack ? call_stack : "<no_callstack>"));
  return false;
}

static bool assertion_handler(bool /*verify*/, const char *file, int line, const char *func, const char *cond, const char *fmt,
  const DagorSafeArg *args, int anum)
{
  String totalMessage(0, "Dagor assertion \"%s\" failed", cond);
  if (fmt)
  {
    totalMessage.append(" with message:\n");
    totalMessage.avprintf(0, fmt, args, anum);
  }
  FAIL(file << "(" << line << "): " << totalMessage << "; function: " << (func ? func : "<no_func>"));
  return false;
}

#if defined(USE_EASTL) && EASTL_ASSERT_ENABLED
static void eastl_assertion_failure_function(const char *expr, void * /*ctx*/) { FAIL("EASTL Assertion: " << expr); }
#endif

// ===============================================================================

static void set_default_reporter(Catch::ConfigData &config_data, const std::string &default_reporter_spec)
{
  // Exactly the same logic as in `Config::Config( ConfigData const& data )` (catch_config.cpp)
  if (config_data.reporterSpecifications.empty())
  {
    auto parsed = Catch::parseReporterSpec(default_reporter_spec);
    CATCH_ENFORCE(parsed, "Cannot parse the provided default reporter spec: '" << default_reporter_spec << '\'');
    config_data.reporterSpecifications.push_back(eastl::move(*parsed));
  }
}

// ===============================================================================

#include <supp/dag_define_KRNLIMP.h>
extern KRNLIMP void init_main_thread_id();
#include <supp/dag_undef_KRNLIMP.h>

int main(int argc, char *argv[])
{
  ::init_main_thread_id();
  ::symhlp_init_default();
  ::dgs_fatal_handler = fatal_handler;
  ::dgs_assertion_handler = assertion_handler;
#if defined(USE_EASTL) && EASTL_ASSERT_ENABLED
  eastl::SetAssertionFailureFunction(&eastl_assertion_failure_function, NULL);
#endif

#ifdef CUSTOM_UNITTEST_CODE
  CUSTOM_UNITTEST_CODE
#endif

  {
    // We want to force the linker not to discard the global variable
    // and its constructor, as it (optionally) registers leak detector
    (void)&Catch::leakDetector;
  }

  Catch::Session session;

  int returnCode = session.applyCommandLine(argc, argv);

  if (returnCode == 0)
  {
    set_default_reporter(session.configData(), "detailed");
    returnCode = session.run();
  }

#ifdef CUSTOM_UNITTEST_SHUTDOWN_CODE
  CUSTOM_UNITTEST_SHUTDOWN_CODE
#endif

  return returnCode;
}

// ===============================================================================
