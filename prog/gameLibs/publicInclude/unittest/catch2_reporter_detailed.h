//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// ===============================================================================
// Detailed Catch2 library reporter is a human readable reporter
// which logs all test cases and sections which were actually executed.
//
// Force using this reporter    : --reporter=detailed
// It supports verbosity levels : --verbosity=quiet|normal|high
// ===============================================================================

#include <catch2/catch_test_case_info.hpp>
#include <catch2/internal/catch_console_colour.hpp>
#include <catch2/internal/catch_console_width.hpp>
#include <catch2/reporters/catch_reporter_console.hpp>
#include <catch2/reporters/catch_reporter_helpers.hpp>
#include <ostream>


namespace Catch::gaijin_reporters
{


struct ExecutionStats
{
  Counts assertions;
  double durationInSeconds = 0.0;
  int sectionCount = 0;
  int testCaseCount = 0;

  void clear()
  {
    assertions = Counts();
    durationInSeconds = 0.0;
    sectionCount = 0;
    testCaseCount = 0;
  }

  void invalidate()
  {
    assertions = Counts();
    durationInSeconds = -999'999;
    sectionCount = -999'999;
    testCaseCount = -999'999;
  }

  void add(const ExecutionStats &other)
  {
    assertions += other.assertions;
    durationInSeconds += other.durationInSeconds;
    sectionCount += other.sectionCount;
    testCaseCount += other.testCaseCount;
  }

  std::string formatDuration() const { return getFormattedDuration(durationInSeconds) + " s"; }
};


class DetailedConsoleReporter : public Catch::ConsoleReporter
{
public:
  using Base = Catch::ConsoleReporter;

  static constexpr int INDENTATION = 4;
  static constexpr auto SRC_FILE_LEVEL_COLOR = Colour::Cyan;
  static constexpr auto TEST_CASE_LEVEL_COLOR = Colour::Yellow;
  static constexpr auto SECTION_LEVEL_COLOR = Colour::LightGrey;

public:
  static std::string getDescription()
  {
    return "Detailed human-readable console reporter"
           " with details about all running test cases and sections"
           " (reporter was added by Gaijin)";
  }

public:
  explicit DetailedConsoleReporter(ReporterConfig &&config) :
    Base(CATCH_MOVE(config)),
    allowNormalMessages(getVerbosity() != Verbosity::Quiet),
    allowDetailedMessages(getVerbosity() == Verbosity::High)
  {}

  void benchmarkPreparing(StringRef benchmarkName) override { Base::benchmarkPreparing(benchmarkName); }
  void benchmarkStarting(BenchmarkInfo const &benchmarkInfo) override { Base::benchmarkStarting(benchmarkInfo); }
  void benchmarkEnded(BenchmarkStats<> const &benchmarkStats) override { Base::benchmarkEnded(benchmarkStats); }
  void benchmarkFailed(StringRef benchmarkName) override { Base::benchmarkFailed(benchmarkName); }

  void fatalErrorEncountered(StringRef error) override { Base::fatalErrorEncountered(error); }
  void noMatchingTestCases(StringRef unmatchedSpec) override { Base::noMatchingTestCases(unmatchedSpec); }
  void reportInvalidTestSpec(StringRef invalidArgument) override { Base::reportInvalidTestSpec(invalidArgument); }

  void testRunStarting(TestRunInfo const &testRunInfo) override
  {
    Base::testRunStarting(testRunInfo);
    m_stream << m_colour->guardColour(Colour::ResultSuccess) << std::string(CATCH_CONFIG_CONSOLE_WIDTH - 1, '=') << '\n';
  }

  void testCaseStarting(TestCaseInfo const &testInfo) override
  {
    Base::testCaseStarting(testInfo);
    statsTestCase.clear();

    if (previousFilename != testInfo.lineInfo.file)
    {
      printFileStats();
      if (!previousFilename.empty())
      {
        // do not log the first time
        m_stream << m_colour->guardColour(SRC_FILE_LEVEL_COLOR) << std::string(CATCH_CONFIG_CONSOLE_WIDTH / 2, '=') << '\n';
      }
      m_stream << m_colour->guardColour(SRC_FILE_LEVEL_COLOR) << testInfo.lineInfo.file << '\n';

      previousFilename = testInfo.lineInfo.file;
      statsFile.clear();
    }

    if (allowNormalMessages)
    {
      m_stream << m_colour->guardColour(TEST_CASE_LEVEL_COLOR) << "TestCase " << testInfo.name << '\n';
    }
  }

  void testCasePartialStarting(const Catch::TestCaseInfo &testInfo, uint64_t partNumber) override
  {
    Base::testCasePartialStarting(testInfo, partNumber);
  }

  void sectionStarting(SectionInfo const &sectionInfo) override
  {
    Base::sectionStarting(sectionInfo);
    if (allowDetailedMessages)
    {
      const int indent = (m_sectionStack.size() - 1) * INDENTATION;
      m_stream << m_colour->guardColour(SECTION_LEVEL_COLOR) << "Section  " << std::string(indent, ' ') << sectionInfo.name << '\n';
    }
  }

  void assertionStarting(AssertionInfo const &assertionStats) override { Base::assertionStarting(assertionStats); }

  void assertionEnded(AssertionStats const &assertionStats) override { Base::assertionEnded(assertionStats); }

  void sectionEnded(SectionStats const &sectionStats) override
  {
    if (m_sectionStack.size() == 1)
    {
      // The root section of test part has ended
      const ExecutionStats additionalStats{sectionStats.assertions, sectionStats.durationInSeconds, 1, 0};
      statsTestCase.add(additionalStats);
      statsFile.add(additionalStats);
      statsTotal.add(additionalStats);
    }

    Base::sectionEnded(sectionStats);
  }

  void testCasePartialEnded(const Catch::TestCaseStats &testCaseStats, uint64_t partNumber) override
  {
    Base::testCasePartialEnded(testCaseStats, partNumber);
  }

  void testCaseEnded(TestCaseStats const &testCaseStats) override
  {
    if (allowNormalMessages)
    {
      m_stream << m_colour->guardColour(SECTION_LEVEL_COLOR) << "TestCase duration: " << statsTestCase.formatDuration()
               << "; test sections: " << statsTestCase.sectionCount << "; assertions: " << statsTestCase.assertions.total() << '\n';
    }

    statsTestCase.invalidate();
    statsFile.testCaseCount++;
    statsTotal.testCaseCount++;
    Base::testCaseEnded(testCaseStats);
  }

  void testRunEnded(TestRunStats const &testRunStats) override
  {
    printFileStats();
    m_stream << m_colour->guardColour(SRC_FILE_LEVEL_COLOR) << "Total duration for all tests: " << statsTotal.formatDuration()
             << "; test cases: " << statsTotal.testCaseCount << "; test sections: " << statsTotal.sectionCount
             << "; assertions: " << statsTotal.assertions.total() << '\n';
    Base::testRunEnded(testRunStats);
  }

  void skipTest(TestCaseInfo const &) override
  {
    // Don't do anything with this by default.
    // It can optionally be overridden in the derived class.
  }

protected:
  Verbosity getVerbosity() const { return m_config->verbosity(); }

  void printFileStats()
  {
    // do not log at first test case start
    if (!previousFilename.empty())
    {
      m_stream << m_colour->guardColour(TEST_CASE_LEVEL_COLOR) << "File duration: " << statsFile.formatDuration()
               << "; test cases: " << statsFile.testCaseCount << "; test sections: " << statsFile.sectionCount
               << "; assertions: " << statsFile.assertions.total() << '\n';
    }
  }

protected:
  const bool allowNormalMessages = true;
  const bool allowDetailedMessages = true;
  std::string previousFilename;
  ExecutionStats statsTestCase;
  ExecutionStats statsFile;
  ExecutionStats statsTotal;
};

} // namespace Catch::gaijin_reporters


// ===============================================================================
