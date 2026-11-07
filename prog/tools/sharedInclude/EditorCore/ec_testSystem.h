// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <math/dag_Point2.h>

#include <gui/dag_imgui.h>
#include <generic/dag_functionRef.h>

#include <EASTL/functional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>

class TestRuntime;
class TestInput;
class SqModules;
typedef struct SQVM *HSQUIRRELVM;
namespace Sqrat
{
class Object;
}

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
  // Returns true when the test is done. See TestCallback::update().
  using TestFunc = eastl::function<bool(TestRuntime &)>;

  TestCallback(TestFunc test_func);
  virtual ~TestCallback();

  // Called once when the test starts.
  virtual void start(TestRuntime &rt);

  // Called from TestRuntime::frameEnd() until it returns true, each time after the input steps that the test queued
  // have finished, or also while they wait if canUpdateWithQueuedInput().
  // Returns true when the test is done, but only after its queued input has run, or the test fails.
  virtual bool update(TestRuntime &rt);

  // True if update() can also run while queued input waits, so that it can see a test that ended before its input ran.
  virtual bool canUpdateWithQueuedInput() const { return false; }

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
  TestState lastState() const { return state; }

  void resetState();
  void markPassed() { state = TestState::PASSED; }
  void markFailed() { state = TestState::FAILED; }

  // Keeps only the first reason of a run, because later failures are usually caused by the first one.
  void markFailed(const char *reason);

  // Empty if the test did not fail, or failed without a reason.
  const String &getFailReason() const { return reasonText; }

  // Clears the fail reason, sets state to running, then starts the test callback.
  void start(TestRuntime &rt);

  // Returns true when the test is done. A running test that is done passes.
  bool update(TestRuntime &rt);

  bool canUpdateWithQueuedInput() const { return testCallback->canUpdateWithQueuedInput(); }

  static void register_script_class(HSQUIRRELVM vm);

private:
  String testName;
  eastl::shared_ptr<TestCallback> testCallback;
  TestState state;
  String reasonText;

  String filePath;
  int line;
};

// A test can end, or fail, while it holds a mouse button or a key. The test runtime releases it, so the next test and
// the user do not get input they did not make.
// Each call queues input steps and returns at once. The returned Future resolves when the steps have finished, a script
// awaits it. A C++ test does not use the Future, TestCallback::update() is called again after the steps have finished.
class TestInput
{
public:
  TestInput(TestRuntime &runtime);

  // Direct input event control
  Sqrat::Object mouseButtonDown(int button);
  Sqrat::Object mouseButtonUp(int button);
  Sqrat::Object mouseClick(int button);
  Sqrat::Object mouseClickModifier(int button, int modifier);
  Sqrat::Object mousePosition(Point2 at);
  Sqrat::Object mouseMove(Point2 from, Point2 to, float over_seconds = 1.0f);
  Sqrat::Object keyDown(int key);
  Sqrat::Object keyUp(int key);
  Sqrat::Object keyPress(int key);
  Sqrat::Object keyPressModifier(int key, int modifier);
  Sqrat::Object keyHeld(int key, float over_seconds = 1.0f);
  Sqrat::Object keyHeldModifier(int key, int modifier, float over_seconds = 1.0f);
  Sqrat::Object characterInput(const char *text, float between_seconds = 0.0f);
  Sqrat::Object mouseDrag(int button, Point2 from, Point2 to, float over_seconds = 1.0f);

  // Query GUI item based input control (filepath/line is the call site for diagnostics).
  // The Future resolves to false in the same frame when the path is not found. That fails the running test when
  // failOnInvalidInputQuery is set. A C++ test that needs to know if the item exists can call
  // imgui_test_runtime_query_item() itself.
  Sqrat::Object mouseMoveItem(const char *path, float over_seconds = 1.0f, const char *filepath = nullptr, int line = 0);
  Sqrat::Object mouseClickItem(const char *path, int button, const char *filepath = nullptr, int line = 0);

  static void register_script_class(HSQUIRRELVM vm);

private:
  void queueMouseButton(int button, bool down);
  void queueMousePosition(Point2 at);
  void queueMouseLerp(Point2 from, Point2 to, float over_seconds);
  void queueKey(int key, bool down);
  void queueKeyWithModifier(int key, int modifier, bool down);
  bool queueMouseMoveItem(const char *path, float over_seconds, const char *filepath, int line);

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

  HSQUIRRELVM getVM() const;

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

// Not thread-safe. Call it, and its tests and input, only from the main thread.
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

  ~TestRuntime();

  void start();

  // Stops the runtime: a running test fails and does not continue, and the queued tests do not run.
  void stop();

  bool isRunning() const { return running; }

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

  // Snapshot of every registered test's name, state, call site and fail reason.
  Tab<TestInfo> testInfos() const;

  TestInput &input() { return inputSystem; }

  // Call it between ImGui::NewFrame() and the first submitted widget.
  // It is the only place that releases the input a test holds and enables accepting user input again, thus a stopped
  // run needs this call too.
  void frameBegin(float dt);

  // Call it after the last widget is submitted and before imgui_endframe().
  // A test runs only inside this call: it runs the queued input steps and resumes the script tasks.
  // A script step that never ends, for example an endless loop, blocks the main thread.
  void frameEnd();

  // Queue a wait step, like the TestInput calls. yieldFrame() resumes the test in the next frameEnd(), yieldTime() in
  // the first frameEnd() after the time.
  Sqrat::Object yieldFrame();
  Sqrat::Object yieldTime(float seconds);

  // Used by TestInput. A step runs once per frameEnd() until it returns true, then the next step runs in the same call.
  void queueInputStep(eastl::function<bool()> step);
  void queueWaitFrame();
  void queueWaitTime(float seconds);

  // Calls queue_steps, then returns a Future that resolves to its result after the steps queued so far.
  // Fails the running test if a script starts an input call before the previous one finished.
  Sqrat::Object inputAction(dag::FunctionRef<bool() const> queue_steps);

  float lastFrameDelta() const { return frameDeltaSeconds; }

  // Callable from inside a test to signal failure without throwing.
  // Logs one failure record when a location or a reason is given, then fails the running test with that record as its
  // fail reason.
  void markFailed(const char *filepath = nullptr, int line = 0, const char *reason = nullptr);

  bool failOnInvalidInputQuery = true;

  bool testsRunning() const { return getRemainingCount() > 0; }
  bool hasQueuedInput() const { return !inputSteps.empty(); }
  TestCase *currentlyRunningTest() { return runningTest; }
  int getRemainingCount() const;
  int getTotalEnqueued() const { return totalEnqueued; }

  ImGuiTestRuntimeOptions &options() { return debugOptions; }

  TestScriptModule &testScripts() { return scriptModule; }

  TestWindow &window() { return debugWindow; }

  static void register_script_class(HSQUIRRELVM vm);

private:
  enum class RunState
  {
    Idle,
    Running,
    Finishing, // The test is done, its held input is being released.
  };

  void tick();
  bool runInputSteps();
  bool pumpScripts();
  void finishRunningTest();
  bool anyTestFailed() const;
  bool callTest(TestCase &test, bool start);

  // ImGui gives key ownership back only on the frame after the key is not down, thus wait one more frame after the clear.
  bool inputReleasePending() const { return inputClearPending || frameCount < (inputClearFrame + 2); }

  // ordered by registration, name-searchable
  Tab<eastl::unique_ptr<TestCase>> registry;

  Tab<TestCase *> testQueue;

  int frameCount = 0;
  float frameDeltaSeconds = 0.0f;
  int inputClearFrame = -1; // The frame in which frameBegin() cleared the input.

  RunState runState = RunState::Idle;
  bool hidingWindow = false;
  bool ticking = false;
  bool pumping = false;

  Tab<eastl::function<bool()>> inputSteps;
  int inputStepWaitFrame = -1; // The frame in which the first input step returned false.

  bool inputClearPending = false;

  bool running = false;

  int totalEnqueued = 0;
  TestCase *runningTest = nullptr;

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
// on_complete(any_failed) fires when the run finishes. Instantly if the script registered no tests.
// TestRuntime::stop() during the run also fires it, with any_failed true unless all tests already ran and passed.
// Returns false if the script failed to load; on_complete is not called in that case.
bool editor_core_test_runtime_run_script(const char *script_path, eastl::function<void(bool any_failed)> on_complete);

void editor_core_test_runtime_set_window(bool visible);
bool editor_core_test_runtime_window_is_visible();
