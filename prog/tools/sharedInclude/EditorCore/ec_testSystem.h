// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>

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
  TestCase(const char *name, eastl::shared_ptr<TestCallback> test_callback);

  const String &name() const { return testName; }
  TestState lastState() const { return (TestState)interlocked_acquire_load(state); }

  void resetState() { interlocked_release_store(state, (int)TestState::IDLE); }
  void markPassed() { interlocked_release_store(state, (int)TestState::PASSED); }
  void markFailed() { interlocked_release_store(state, (int)TestState::FAILED); }

  // Sets state to running, then invokes the test function.
  void run(TestRuntime &rt);

  static void register_script_class(HSQUIRRELVM vm);

private:
  String testName;
  eastl::shared_ptr<TestCallback> testCallback;
  volatile int state;
};

class TestInput
{
public:
  TestInput(TestRuntime &runtime);

  // Direct input event control
  void mouseClick(int button);
  void mousePosition(Point2 at);
  void mouseMove(Point2 from, Point2 to, float over_seconds = 1.0f);
  void keyPress(int key);
  void keyPressModifier(int key, int modifier);
  void keyHeld(int key, float over_seconds = 1.0f);
  void keyHeldModifier(int key, int modifier, float over_seconds = 1.0f);
  void characterInput(const char *text, float between_seconds = 0.0f);
  void mouseDrag(int button, Point2 from, Point2 to, float over_seconds = 1.0f);

  // Query GUI item based input control
  bool mouseMoveItem(const char *path, float over_seconds = 1.0f);
  bool mouseClickItem(const char *path, int button);

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

  String scriptFilenameText;
  bool scriptInputFocused = false;
};

class TestRuntime
{
public:
  struct TestInfo
  {
    String name;
    TestState state;
  };

  TestRuntime() = default;
  ~TestRuntime();

  void start();
  void stop();
  bool isRunning() const { return interlocked_acquire_load(running); }

  TestCase &registerTest(const char *name, TestCallback::TestFunc test_func);
  TestCase &registerTestEx(const char *name, const eastl::shared_ptr<TestCallback> &test_callback);

  bool enqueue(const char *name);
  void enqueueAll(TestFilter state, const char *name);

  // Snapshot of every registered test's name and current state. Thread-safe.
  Tab<TestInfo> testInfos() const;

  TestInput &input() { return inputSystem; }

  // Call from the main thread around each frame update + draw.
  void frameBegin(float dt);
  void frameEnd();

  // Call from inside a TestCase function (test thread only).
  void yieldFrame();
  void yieldTime(float seconds);

  float lastFrameDelta() const { return frameDeltaSeconds; }

  // Callable from inside a test to signal failure without throwing.
  void markFailed();

  bool failOnInvalidInputQuery = true;

  bool testsRunning() const { return interlocked_acquire_load(anyTestRunning); }
  TestCase *currentlyRunningTest() { return interlocked_acquire_load_ptr(runningTest); }

  ImGuiTestRuntimeOptions &options() { return debugOptions; }

  TestScriptModule &testScripts() { return scriptModule; }

  TestWindow &window() { return debugWindow; }

  static void register_script_class(HSQUIRRELVM vm);

private:
  void threadLoop();

  class TestThread final : public DaThread
  {
  public:
    TestThread(TestRuntime &rt) : DaThread("TestThread", 256 << 10, 0, 0), runtime(rt) {}

    void execute() override { runtime.threadLoop(); }

  private:
    TestRuntime &runtime;
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

  volatile bool running = false;

  volatile bool anyTestRunning = false;
  TestCase *volatile runningTest = nullptr;

  ImGuiTestRuntimeOptions debugOptions{this, false, 0, false};
  bool drawFakeMousePointer = true;

  TestInput inputSystem{*this};
  TestInput *inputPtr() { return &inputSystem; } // for sq

  TestWindow debugWindow{*this};

  TestScriptModule scriptModule{*this};
};

TestRuntime *editor_core_test_runtime_get_instance();

void editor_core_test_runtime_begin_frame();
void editor_core_test_runtime_end_frame();

void editor_core_test_runtime_set_window(bool visible);
bool editor_core_test_runtime_window_is_visible();
