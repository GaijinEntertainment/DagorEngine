// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_events.h>

#include <util/dag_string.h>
#include <math/dag_Point2.h>

#include <gui/dag_imgui.h>

#include <EASTL/functional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>

class TestRuntime;
class TestInput;
class SqModules;
typedef struct SQVM *HSQUIRRELVM;

enum class TestState : int
{
  IDLE,
  RUNNING,
  PASSED,
  FAILED,
};

class TestCallback
{
public:
  using TestFunc = eastl::function<void(TestRuntime &)>;

  TestCallback(TestFunc test_func);
  virtual ~TestCallback();

  virtual void run(TestRuntime &rt);

private:
  TestFunc testFunc;
};

class TestCase
{
public:
  TestCase(const char *name, eastl::shared_ptr<TestCallback> test_callback, const char *filepath = nullptr, int line = 0);

  const String &name() const { return testName; }
  const String &getFilePath() const { return filePath; }
  int getSourceLine() const { return line; }
  TestState lastState() const { return (TestState)interlocked_acquire_load(state); }

  void resetState();
  void markPassed() { interlocked_release_store(state, (int)TestState::PASSED); }
  void markFailed() { interlocked_release_store(state, (int)TestState::FAILED); }

  // Keeps only the first reason of a run, because later failures are usually caused by the first one.
  void markFailed(const char *reason);

  // Empty if the test did not fail, or failed without a reason. Thread-safe.
  String getFailReason() const;

  // Clears the fail reason, sets state to running, then invokes the test function.
  void run(TestRuntime &rt);

  static void register_script_class(HSQUIRRELVM vm);

private:
  String testName;
  eastl::shared_ptr<TestCallback> testCallback;
  volatile int state;

  // The test thread writes the reason, the main thread reads it for the Tests tab.
  mutable WinCritSec reasonMutex;
  String reasonText;

  String filePath;
  int line;
};

// A test can end, or fail, while it holds a mouse button or a key. The test runtime releases it, so the next test and
// the user do not get input they did not make.
class TestInput
{
public:
  TestInput(TestRuntime &runtime);

  // Direct input event control
  void mouseButtonDown(int button);
  void mouseButtonUp(int button);
  void mouseClick(int button);
  void mouseClickModifier(int button, int modifier);
  void mousePosition(Point2 at);
  void mouseMove(Point2 from, Point2 to, float over_seconds = 1.0f);
  void keyDown(int key);
  void keyUp(int key);
  void keyPress(int key);
  void keyPressModifier(int key, int modifier);
  void keyHeld(int key, float over_seconds = 1.0f);
  void keyHeldModifier(int key, int modifier, float over_seconds = 1.0f);
  void characterInput(const char *text, float between_seconds = 0.0f);
  void mouseDrag(int button, Point2 from, Point2 to, float over_seconds = 1.0f);

  // Query GUI item based input control (filepath/line is the call site for diagnostics).
  // A path that is not found fails the running test when failOnInvalidInputQuery is set, then returns false.
  bool mouseMoveItem(const char *path, float over_seconds = 1.0f, const char *filepath = nullptr, int line = 0);
  bool mouseClickItem(const char *path, int button, const char *filepath = nullptr, int line = 0);

  static void register_script_class(HSQUIRRELVM vm);

private:
  TestRuntime &runtime;
};

class TestScriptModule
{
public:
  TestScriptModule(TestRuntime &rt);
  ~TestScriptModule();

  bool bindScript(const char *script_filename);
  bool reloadScript();

  const String &getScriptFilename() const { return scriptFilename; }

private:
  TestRuntime &runtime;

  void init();

  String scriptFilename;
  SqModules *moduleMgr = nullptr;
};

enum class TestFilter
{
  ALL,
  IDLE,
  PASSED,
  FAILED,
};

class TestWindow
{
public:
  TestWindow(TestRuntime &rt);

  void updateImgui();

  bool drawFakeMousePointerDuringRun = true;
  bool hideDuringRun = false;

private:
  TestRuntime &runtime;

  TestFilter filterSelection = TestFilter::ALL;
  bool runOnlyNameFiltered = false;
  String filterText;
  bool textInputFocused = false;

  String runStatusText;

  String scriptFilenameText;
  bool scriptInputFocused = false;
  String currentScriptText;
};

// Registers a test and records the call site (file and line) for diagnostics.
#define EDITOR_CORE_REGISTER_TEST(runtime, name, func) \
  (runtime).registerTestEx((name), eastl::make_shared<TestCallback>(func), __FILE__, __LINE__)

// Marks the currently running test as failed and logs the reason with the call site.
#define EDITOR_CORE_TEST_FAIL(runtime, reason) (runtime).markFailed(__FILE__, __LINE__, (reason))

// Input helpers that record the call site on failure.
#define EDITOR_CORE_MOUSE_MOVE_ITEM(input, path, over_seconds) (input).mouseMoveItem((path), (over_seconds), __FILE__, __LINE__)
#define EDITOR_CORE_MOUSE_CLICK_ITEM(input, path, button)      (input).mouseClickItem((path), (button), __FILE__, __LINE__)

class TestRuntime
{
public:
  struct TestInfo
  {
    String name;
    TestState state = TestState::IDLE;
    // Empty filePath means no call site was recorded; sourceLine is 0 in that case.
    String filePath;
    int sourceLine = 0;
    String failReason;
  };

  TestRuntime();
  ~TestRuntime();

  void start();
  void stop();
  bool isRunning() const { return interlocked_acquire_load(running); }

  TestCase &registerTest(const char *name, TestCallback::TestFunc test_func);
  TestCase &registerTestEx(const char *name, const eastl::shared_ptr<TestCallback> &test_callback, const char *filepath = nullptr,
    int line = 0);

  int testCount() const { return registry.size(); }

  // Both assert and do nothing if tests are running (queue holds raw pointers into the registry).
  bool deleteTest(const char *name);
  void deleteAllTests();

  bool enqueue(const char *name);
  int enqueueAll(TestFilter state, const char *name);
  // Removes all pending tests from the queue (currently running test finishes naturally).
  void clearQueue();

  // Snapshot of every registered test's name, state, call site and fail reason. Thread-safe.
  Tab<TestInfo> testInfos() const;

  TestInput &input() { return inputSystem; }

  // Call from the main thread, between ImGui::NewFrame() and the first submitted widget.
  // It is the only place that releases the input a test holds and enables accepting user input again, thus a stopped
  // run needs this call too.
  void frameBegin(float dt);

  // Call from the main thread, after the last widget is submitted and before imgui_endframe().
  // A test runs only inside this call. It wakes a waiting test, then waits until the test yields again.
  // If the test does not yield in time (see TIMEOUT_MSEC), it fails, the main thread stops waiting for it, and the test
  // runs outside this call until it ends.
  void frameEnd();

  // Call from inside a TestCase function (test thread only).
  // The main thread waits while a test runs, thus a test must not wait for the main thread other than with these calls.
  void yieldFrame();
  void yieldTime(float seconds);

  float lastFrameDelta() const { return frameDeltaSeconds; }

  // Callable from inside a test to signal failure without throwing.
  // Logs one failure record when a location or a reason is given, then fails the running test with that record as its
  // fail reason.
  void markFailed(const char *filepath = nullptr, int line = 0, const char *reason = nullptr);

  bool failOnInvalidInputQuery = true;

  bool testsRunning() const { return interlocked_acquire_load(anyTestRunning); }
  TestCase *currentlyRunningTest() { return interlocked_acquire_load_ptr(runningTest); }
  int getRemainingCount();
  int getTotalEnqueued() const { return interlocked_acquire_load(totalEnqueued); }

  ImGuiTestRuntimeOptions &options() { return debugOptions; }

  TestScriptModule &testScripts() { return scriptModule; }

  TestWindow &window() { return debugWindow; }

  static void register_script_class(HSQUIRRELVM vm);

private:
  void threadLoop();
  void waitInputClear();
  void waitNextFrame(WinAutoLock &frame_lock);

  class TestThread final : public DaThread
  {
  public:
    TestThread(TestRuntime &rt) : DaThread("TestThread", 256 << 10, 0, 0), runtime(rt) {}

    void execute() override { runtime.threadLoop(); }

  private:
    TestRuntime &runtime;
  };

  enum class TestThreadState
  {
    Idle,         // No test step runs or waits for frameEnd().
    WaitingToRun, // The test waits until frameEnd() wakes it.
    Running,      // A test step runs, frameEnd() waits until it yields.
  };

  TestThread thread{*this};

  // ordered by registration, name-searchable
  Tab<eastl::unique_ptr<TestCase>> registry;

  WinCritSec queueMutex;
  Tab<TestCase *> testQueue;

  WinCritSec frameMutex;
  int frameCount = 0;
  bool frameInProgress = false;
  float frameDeltaSeconds = 0.0f;
  int inputClearFrame = -1; // The frame in which frameBegin() cleared the input.

  // The state and the flag are written under frameMutex, except in start(), which runs while no test thread exists.
  // The two threads do not run in parallel, except when a test does not yield in time. Then they run in parallel until
  // the test ends.
  os_event_t testThreadCanRun, mainThreadCanRun;
  TestThreadState testThreadState = TestThreadState::Idle;
  bool mainThreadWaitTimedOut = false;

  volatile bool inputClearPending = false;

  volatile bool running = false;

  // Written under queueMutex; read lock-free via interlocked_acquire_load.
  // runningTest is also cleared without the mutex after a test finishes (single writer).
  volatile bool anyTestRunning = false;
  volatile int totalEnqueued = 0;
  TestCase *volatile runningTest = nullptr;

  ImGuiTestRuntimeOptions debugOptions{this, false, 0, false};
  bool drawFakeMousePointer = true;

  TestInput inputSystem{*this};
  TestInput *inputPtr() { return &inputSystem; } // for sq

  TestWindow debugWindow{*this};

  TestScriptModule scriptModule{*this};
};

void editor_core_initialize_test_runtime();

// Call before imgui_shutdown(), because while shutting down the tests, a test can call dag_imgui, it can make the
// "Test Runtime" window visible again.
void editor_core_shutdown_test_runtime();

TestRuntime *editor_core_test_runtime_get_instance();

void editor_core_test_runtime_begin_frame();
void editor_core_test_runtime_end_frame();

// Load script_path, start the runtime, enqueue all tests.
// on_complete(any_failed) fires on the main thread when the run finishes. Instantly if the script registered no tests.
// Returns false if the script failed to load; on_complete is not called in that case.
bool editor_core_test_runtime_run_script(const char *script_path, eastl::function<void(bool any_failed)> on_complete);

void editor_core_test_runtime_set_window(bool visible);
bool editor_core_test_runtime_window_is_visible();
