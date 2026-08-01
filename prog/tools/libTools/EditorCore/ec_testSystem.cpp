// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_testSystem.h>
#include <EditorCore/ec_interface.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>

#include <util/dag_console.h>
#include <coolConsole/coolConsole.h>
#include <coolConsole/conBatch.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <sqmodules/sqmodules.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/sqModulesDagor.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_strUtil.h>

#include <EASTL/algorithm.h>
#include <EASTL/string_view.h>

typedef struct SQVM *HSQUIRRELVM;

//==================================================================================================

TestCallback::TestCallback(TestFunc test_func) : testFunc(test_func) {}

TestCallback::~TestCallback() {}

void TestCallback::run(TestRuntime &rt) { testFunc(rt); }

//==================================================================================================

TestCase::TestCase(const char *name, eastl::shared_ptr<TestCallback> tc) :
  testName(name), testCallback(tc), state((int)TestState::IDLE)
{}

void TestCase::run(TestRuntime &rt)
{
  interlocked_release_store(state, (int)TestState::RUNNING);

  testCallback->run(rt);

  if (lastState() != TestState::FAILED)
    markPassed();
}

/*static*/
void TestCase::register_script_class(HSQUIRRELVM vm)
{
  Sqrat::Enumeration sqTestState(vm);
  sqTestState //
    .Const("IDLE", TestState::IDLE)
    .Const("RUNNING", TestState::RUNNING)
    .Const("PASSED", TestState::PASSED)
    .Const("FAILED", TestState::FAILED);

  Sqrat::Class<TestCase, Sqrat::NoConstructor<TestCase>> sqTestCase(vm, "TestCase");
  sqTestCase //
    .Func("lastState", &TestCase::lastState)
    .Func("resetState", &TestCase::resetState)
    .Func("markPassed", &TestCase::markPassed)
    .Func("markFailed", &TestCase::markFailed)
    /**/;
}

//==================================================================================================

static void addMouseButtonEvent(int button, bool down)
{
  ImGuiIO &io = ImGui::GetIO();
  io.SetAppAcceptingEvents(true);
  io.AddMouseButtonEvent(button, down);
  io.SetAppAcceptingEvents(false);
}

static void addMousePosEvent(real x, real y)
{
  ImGuiIO &io = ImGui::GetIO();
  io.SetAppAcceptingEvents(true);
  io.AddMousePosEvent(x, y);
  io.SetAppAcceptingEvents(false);
}

static void addMousePosEvent(Point2 pos) { addMousePosEvent(pos.x, pos.y); }

static void addKeyEvent(int key, bool down)
{
  ImGuiIO &io = ImGui::GetIO();
  io.SetAppAcceptingEvents(true);
  io.AddKeyEvent((ImGuiKey)key, down);
  io.SetAppAcceptingEvents(false);
}

static void addKeyWithModifierEvent(int key, int modifier, bool down)
{
  addKeyEvent(key, down);
  addKeyEvent(modifier, down);
}

static void addInputCharacter(unsigned int character)
{
  ImGuiIO &io = ImGui::GetIO();
  io.SetAppAcceptingEvents(true);
  io.AddInputCharacter(character);
  io.SetAppAcceptingEvents(false);
}

//==================================================================================================

TestInput::TestInput(TestRuntime &runtime) : runtime(runtime) {}

void TestInput::mouseClick(int button)
{
  addMouseButtonEvent(button, true);
  runtime.yieldFrame();
  addMouseButtonEvent(button, false);
  runtime.yieldFrame();
}

void TestInput::mousePosition(Point2 at)
{
  addMousePosEvent(at);
  runtime.yieldFrame();
}

void TestInput::mouseMove(Point2 from, Point2 to, float over_seconds)
{
  mousePosition(from);
  float elapsed = 0.0f;
  while (elapsed < over_seconds)
  {
    const float t = elapsed / over_seconds;
    addMousePosEvent(::lerp(from, to, t));
    elapsed += runtime.lastFrameDelta();
    runtime.yieldFrame();
  }
  // guarantee exact final position
  mousePosition(to);
}

void TestInput::keyPress(int key)
{
  addKeyEvent(key, true);
  runtime.yieldFrame();
  addKeyEvent(key, false);
  runtime.yieldFrame();
}

void TestInput::keyPressModifier(int key, int modifier)
{
  addKeyWithModifierEvent(key, modifier, true);
  runtime.yieldFrame();
  addKeyWithModifierEvent(key, modifier, false);
  runtime.yieldFrame();
}

void TestInput::keyHeld(int key, float over_seconds)
{
  addKeyEvent(key, true);
  runtime.yieldTime(over_seconds);
  addKeyEvent(key, false);
  runtime.yieldFrame();
}

void TestInput::keyHeldModifier(int key, int modifier, float over_seconds)
{
  addKeyWithModifierEvent(key, modifier, true);
  runtime.yieldTime(over_seconds);
  addKeyWithModifierEvent(key, modifier, false);
  runtime.yieldFrame();
}

void TestInput::characterInput(const char *text, float between_seconds)
{
  const char *textEnd = text + strlen(text);
  while (*text != 0)
  {
    unsigned int c = 0;
    text += ImTextCharFromUtf8(&c, text, textEnd);
    addInputCharacter(c);
    if (between_seconds > 0.0f)
      runtime.yieldTime(between_seconds);
    else
      runtime.yieldFrame();
  }
}

void TestInput::mouseDrag(int button, Point2 from, Point2 to, float over_seconds)
{
  mousePosition(from);
  addMouseButtonEvent(button, true);
  runtime.yieldFrame();

  float elapsed = 0.0f;
  while (elapsed < over_seconds)
  {
    const float t = elapsed / over_seconds;
    addMousePosEvent(::lerp(from, to, t));
    elapsed += runtime.lastFrameDelta();
    runtime.yieldFrame();
  }

  mousePosition(to);
  addMouseButtonEvent(button, false);
  runtime.yieldFrame();
}

//==================================================================================================

bool TestInput::mouseMoveItem(const char *path, float over_seconds)
{
  ImGuiID id, parentId;
  ImRect bb;
  if (imgui_test_runtime_query_item(path, id, bb, parentId))
  {
    ImVec2 p = ImGui::GetIO().MousePos;
    ImVec2 c = bb.GetCenter();
    mouseMove(p, c, over_seconds);
    return true;
  }
  else if (runtime.failOnInvalidInputQuery)
  {
    logerr("TestInput Item Error: query path \"%s\" not found!", path);
    runtime.markFailed();
  }
  return false;
}

bool TestInput::mouseClickItem(const char *path, int button)
{
  if (mouseMoveItem(path))
  {
    mouseClick(button);
    return true;
  }
  return false;
}

//==================================================================================================

/*static*/
void TestInput::register_script_class(HSQUIRRELVM vm)
{
  // Mouse, Keys, Modifier are exported through the "test_runtime" module table, not as globals.
  Sqrat::Class<TestInput, Sqrat::NoConstructor<TestInput>> sqTestInput(vm, "TestInput");
  sqTestInput //
    .Func("mouseClick", &TestInput::mouseClick)
    .Func("mousePosition", &TestInput::mousePosition)
    .Func("mouseMove", &TestInput::mouseMove)
    .Func("keyPress", &TestInput::keyPress)
    .Func("keyPressModifier", &TestInput::keyPressModifier)
    .Func("keyHeld", &TestInput::keyHeld)
    .Func("keyHeldModifier", &TestInput::keyHeldModifier)
    .Func("characterInput", &TestInput::characterInput)
    .Func("mouseDrag", &TestInput::mouseDrag)
    .Func("mouseMoveItem", &TestInput::mouseMoveItem)
    .Func("mouseClickItem", &TestInput::mouseClickItem)
    /**/;
}

//==================================================================================================

TestWindow::TestWindow(TestRuntime &tr) : runtime(tr) {}

static ImVec4 state_color(TestState state)
{
  switch (state)
  {
    case TestState::IDLE: return {0.5f, 0.5f, 0.5f, 1.0f};
    case TestState::RUNNING: return {0.2f, 0.2f, 0.8f, 1.0f};
    case TestState::PASSED: return {0.2f, 0.8f, 0.2f, 1.0f};
    case TestState::FAILED: return {0.9f, 0.2f, 0.2f, 1.0f};
  }
  return {0.0f, 0.0f, 0.0f, 1.0f};
}

static TestState aggregate_state(const Tab<TestRuntime::TestInfo> &infos)
{
  bool anyPassed = false;

  for (const auto &info : infos)
  {
    if (info.state == TestState::FAILED)
      return TestState::FAILED;
    if (info.state == TestState::RUNNING)
      return TestState::RUNNING;
    if (info.state == TestState::PASSED)
      anyPassed = true;
  }

  return anyPassed ? TestState::PASSED : TestState::IDLE;
}

void TestWindow::updateImgui()
{
  if (ImGui::BeginTabBar("##tabs"))
  {
    if (ImGui::BeginTabItem("Tests"))
    {
      ImGuiContext *g = ImGui::GetCurrentContext();
      bool enabled = g->TestEngineHookItems;
      ImGui::BeginDisabled(!enabled);

      bool running = runtime.isRunning();
      if (ImGui::Button(running ? "Stop Test Runtime" : "Start Test Runtime"))
      {
        if (running)
          runtime.stop();
        else
          runtime.start();
        running = runtime.isRunning();
      }

      ImGui::BeginDisabled(!running);

      ImGui::Checkbox("Hide test window during run", &hideDuringRun);

      const auto infos = runtime.testInfos();
      const float frameHeight = ImGui::GetFrameHeight();
      const ImVec2 indicatorSize = PropPanel::ImguiHelper::getFontSizedIconSize();

      const TestState allState = aggregate_state(infos);
      ImGui::ColorButton("##all_state", state_color(allState), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
        ImVec2(frameHeight, frameHeight));
      ImGui::SameLine();
      if (ImGui::Button("Run all"))
        runtime.enqueueAll(filterSelection, runOnlyNameFiltered ? filterText.c_str() : "");

      ImGui::SameLine();
      constexpr const char *filterItems[] = {"All", "Idle", "Passed", "Failed"};
      ImGui::SetNextItemWidth(PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(ImGui::CalcTextSize("Passed")).x);
      int filterSelectionInt = (int)filterSelection;
      ImGui::Combo("##filter", &filterSelectionInt, filterItems, IM_ARRAYSIZE(filterItems));
      filterSelection = (TestFilter)filterSelectionInt;

      ImGui::SameLine();
      ImGui::Checkbox("Run only filtered", &runOnlyNameFiltered);

      ImGui::SameLine();
      ImGui::SetNextItemWidth(-FLT_MIN);
      const bool textInputWasFocused = textInputFocused;
      PropPanel::ImguiHelper::inputTextWithEnterWorkaround("##filterTests", "Filter tests", &filterText, textInputWasFocused);
      textInputFocused = ImGui::IsItemFocused();

      ImGui::Separator();

      constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
      if (ImGui::BeginTable("test_table", 2, tableFlags))
      {
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        eastl::string_view filter = to_string_view(filterText);
        for (const auto &info : infos)
        {
          if (!filterText.empty() && !dd_stristr(to_string_view(info.name), filter))
            continue;

          ImGui::TableNextRow();

          // State column: colored indicator + Run button
          ImGui::TableSetColumnIndex(0);
          ImGui::Spacing();
          ImGui::SameLine();
          String indicatorId;
          indicatorId.setStrCat("##state_", info.name.c_str());
          ImGui::ColorButton(indicatorId.c_str(), state_color(info.state),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, indicatorSize);
          ImGui::SameLine();
          String runId;
          runId.setStrCat("Run##", info.name.c_str());
          if (ImGui::SmallButton(runId.c_str()))
            runtime.enqueue(info.name);

          // Test column: name
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(info.name.c_str());
        }

        ImGui::EndTable();
      }

      ImGui::EndDisabled();

      ImGui::EndDisabled();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("GUI Items"))
    {
      ImGuiContext *g = ImGui::GetCurrentContext();
      bool enabled = g->TestEngineHookItems;
      ImGuiTestRuntimeOptions *options = static_cast<ImGuiTestRuntimeOptions *>(g->TestEngine);
      if (ImGui::Checkbox("Test Runtime Set", &enabled))
      {
        if (options == nullptr)
          options = &runtime.options();
        imgui_test_runtime_set(enabled, options);
      }

      ImGui::BeginDisabled(!enabled);

      ImGui::Checkbox("Draw fake mouse pointer during run", &drawFakeMousePointerDuringRun);

      ImGui::Checkbox("Draw info of hovered GUI item", &runtime.options().drawHovered);

      ImGui::EndDisabled();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Script"))
    {
      ImGui::BeginDisabled(runtime.testsRunning());

      const bool scriptInputWasFocused = scriptInputFocused;
      PropPanel::ImguiHelper::inputTextWithEnterWorkaround("##scriptFilename", "Script filename", &scriptFilenameText,
        scriptInputWasFocused);
      scriptInputFocused = ImGui::IsItemFocused();

      if (ImGui::Button("Load script"))
        runtime.testScripts().bindScript(scriptFilenameText.c_str());

      ImGui::EndDisabled();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
}

//==================================================================================================

TestScriptModule::TestScriptModule(TestRuntime &rt) : runtime(rt), scriptFilename("") { init(); }

static void script_print_func(HSQUIRRELVM /*v*/, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(_MAKE4C('SQRL'), "%s", msg.c_str());
  va_end(vl);
}


static void script_err_print_func(HSQUIRRELVM /*v*/, const char *s, ...)
{
  va_list vl;
  va_start(vl, s);
  String msg(framemem_ptr());
  msg.cvprintf(0, s, vl);
  logmessage(LOGLEVEL_ERR, "%s", msg.c_str());
  va_end(vl);
}

void TestScriptModule::init()
{
  HSQUIRRELVM vm = sq_open(1280);
  sqstd_seterrorhandlers(vm);
  sq_setprintfunc(vm, script_print_func, script_err_print_func);

  moduleMgr = new SqModules(vm, &sq_modules_dagor_file_access);
  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerIoStreamLib();
  moduleMgr->registerIoLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();

  bindquirrel::register_reg_exp(moduleMgr);
  bindquirrel::register_utf8(moduleMgr);

  bindquirrel::sqrat_bind_dagor_math(moduleMgr);
  bindquirrel::bind_dagor_time(moduleMgr);
  bindquirrel::register_iso8601_time(moduleMgr);
  bindquirrel::register_platform_module(moduleMgr);

  bindquirrel::register_dagor_fs_module(moduleMgr);
  bindquirrel::register_dagor_clipboard(moduleMgr);
  bindquirrel::sqrat_bind_datablock(moduleMgr);

  TestCase::register_script_class(vm);
  TestInput::register_script_class(vm);
  TestRuntime::register_script_class(vm);

  Sqrat::Table exports(vm);
  exports //
    .Func("getInstance", editor_core_test_runtime_get_instance)
    /**/;

  Sqrat::Table sqMouse(vm);
  sqMouse //
    .SetValue("Left", (int)ImGuiMouseButton_Left)
    .SetValue("Right", (int)ImGuiMouseButton_Right)
    .SetValue("Middle", (int)ImGuiMouseButton_Middle)
    /**/;
  exports.SetValue("Mouse", sqMouse);

  Sqrat::Table sqModifier(vm);
  sqModifier //
    .SetValue("None", (int)ImGuiMod_None)
    .SetValue("Ctrl", (int)ImGuiMod_Ctrl)
    .SetValue("Shift", (int)ImGuiMod_Shift)
    .SetValue("Alt", (int)ImGuiMod_Alt)
    .SetValue("Super", (int)ImGuiMod_Super)
    /**/;
  exports.SetValue("Modifier", sqModifier);

  Sqrat::Table sqKeys(vm);
  for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
    sqKeys.SetValue(ImGui::GetKeyName((ImGuiKey)key), key);
  exports.SetValue("Keys", sqKeys);

  moduleMgr->addNativeModule("test_runtime", exports);
}

TestScriptModule::~TestScriptModule()
{
  HSQUIRRELVM vm = moduleMgr->getVM();
  delete moduleMgr;
  sq_close(vm);
}

bool TestScriptModule::bindScript(const char *script_filename)
{
  if (!::dd_file_exist(script_filename))
    return false;

  scriptFilename = script_filename;

  Sqrat::Object exports;
  Sqrat::string errMsg;
  if (!moduleMgr->requireModule(scriptFilename, true, scriptFilename, exports, errMsg))
  {
    logerr("TestScriptModule script error: %s [in %s]", errMsg.c_str(), script_filename);
    return false;
  }

  return true;
}

//==================================================================================================

TestRuntime::~TestRuntime() { stop(); }

void TestRuntime::start()
{
  if (interlocked_exchange(running, true))
    return;

  thread.start();
}

void TestRuntime::stop()
{
  if (!interlocked_exchange(running, false))
    return;

  thread.terminate(true);

  WinAutoLock lock(queueMutex);
  testQueue.clear();
}

//==================================================================================================

TestCase &TestRuntime::registerTest(const char *name, TestCallback::TestFunc test_func)
{
  return registerTestEx(name, eastl::make_shared<TestCallback>(test_func));
}

TestCase &TestRuntime::registerTestEx(const char *name, const eastl::shared_ptr<TestCallback> &test_callback)
{
  G_ASSERT(!eastl::any_of(registry.begin(), registry.end(), [&](const eastl::unique_ptr<TestCase> &tc) {
    return tc->name() == name;
  }) && "TestRuntime::registerTest: duplicate test name!");
  registry.push_back(eastl::make_unique<TestCase>(name, test_callback));
  return *registry.back();
}

//==================================================================================================

bool TestRuntime::enqueue(const char *name)
{
  auto it =
    eastl::find_if(registry.begin(), registry.end(), [&](const eastl::unique_ptr<TestCase> &tc) { return tc->name() == name; });
  if (it != registry.end())
  {
    WinAutoLock lock(queueMutex);
    testQueue.push_back(it->get());
    interlocked_release_store(anyTestRunning, true);
    return true;
  }
  G_ASSERT_FAIL("TestRuntime::enqueue: name not found in registry!");
  return false;
}

void TestRuntime::enqueueAll(TestFilter state, const char *name)
{
  WinAutoLock lock(queueMutex);
  int count = 0;
  eastl::string_view nameFilter(name);
  for (const auto &tc : registry)
  {
    const TestState lastState = tc->lastState();
    if ((state == TestFilter::IDLE && lastState != TestState::IDLE) ||
        (state == TestFilter::PASSED && lastState != TestState::PASSED) ||
        (state == TestFilter::FAILED && lastState != TestState::FAILED))
      continue;

    if (nameFilter.size() > 0 && !dd_stristr(to_string_view(tc->name()), nameFilter))
      continue;

    tc->resetState();
    testQueue.push_back(tc.get());

    count++;
  }
  if (count > 0)
    interlocked_release_store(anyTestRunning, true);
}

Tab<TestRuntime::TestInfo> TestRuntime::testInfos() const
{
  Tab<TestInfo> result;
  result.reserve(registry.size());
  for (const auto &tc : registry)
    result.push_back({tc->name(), tc->lastState()});
  return result;
}

//==================================================================================================

void TestRuntime::frameBegin(float dt)
{
  WinAutoLock lock(frameMutex);
  frameDeltaSeconds = dt;
  frameInProgress = true;
}

void TestRuntime::frameEnd()
{
  WinAutoLock lock(frameMutex);
  frameInProgress = false;
  ++frameCount;
}

//==================================================================================================

static void threadWait(WinAutoLock &lock, int time_msec)
{
  lock.unlock();
  sleep_msec(time_msec);
  lock.lock();
}

void TestRuntime::yieldFrame()
{
  WinAutoLock lock(frameMutex);
  const int target = frameCount + 1;
  while ((frameInProgress || frameCount < target) && running)
    threadWait(lock, 1);
}

void TestRuntime::yieldTime(float seconds)
{
  int targetTime = get_time_msec() + (seconds * 1000.0f);

  WinAutoLock lock(frameMutex);
  while ((get_time_msec() < targetTime) && running)
    threadWait(lock, 50);

  while (frameInProgress && running)
    threadWait(lock, 1);
}

void TestRuntime::markFailed()
{
  if (TestCase *test = currentlyRunningTest())
    test->markFailed();
}

//==================================================================================================

// Keeps a Sqrat::Function alive (referenced in HSQUIRRELVM) while the owning TestCase is alive
class SQTestCallback : public TestCallback
{
public:
  SQTestCallback(Sqrat::Function &fn) : TestCallback([](TestRuntime &rt) {}), function(fn) {}

  void run(TestRuntime &rt) override { function.Execute(&rt); }

private:
  Sqrat::Function function;
};

static SQInteger test_runtime_register_test(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TestRuntime *> self(vm, 1);
  Sqrat::Var<const char *> name(vm, 2);
  Sqrat::Var<Sqrat::Function> func(vm, 3);
  eastl::shared_ptr<SQTestCallback> funcPtr = eastl::make_shared<SQTestCallback>(func.value);
  TestCase &tc = self.value->registerTestEx(name.value, funcPtr);
  Sqrat::Var<TestCase *>::push(vm, &tc);
  return 1;
}

static SQInteger test_runtime_query_item(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  // Sqrat::Var<TestRuntime *> self(vm, 1);
  Sqrat::Var<const char *> path(vm, 2);

  uint32_t id;
  ImRect bb;
  uint32_t parentId;
  if (imgui_test_runtime_query_item(path.value, id, bb, parentId))
  {
    Sqrat::Table sqItemInfo(vm);
    sqItemInfo.SetValue("id", id);
    Sqrat::Table sqBoundingBox(vm);
    sqBoundingBox.SetValue("Max", Point2(bb.Max.x, bb.Max.y));
    sqBoundingBox.SetValue("Min", Point2(bb.Min.x, bb.Min.y));
    sqItemInfo.SetValue("bb", sqBoundingBox);
    sqItemInfo.SetValue("parentId", parentId);
    Sqrat::Var<Sqrat::Table>::push(vm, sqItemInfo);
    return 1;
  }
  sq_pushnull(vm);
  return 1;
}

/*static*/
void TestRuntime::register_script_class(HSQUIRRELVM vm)
{
  Sqrat::Class<TestRuntime, Sqrat::NoConstructor<TestRuntime>> sqTestRuntime(vm, "TestRuntime");
  sqTestRuntime //
    .Func("isRunning", &TestRuntime::isRunning)
    .SquirrelFunc("registerTest", &test_runtime_register_test, 3, "xsc")
    .Func("input", &TestRuntime::inputPtr)
    .Func("yieldFrame", &TestRuntime::yieldFrame)
    .Func("yieldTime", &TestRuntime::yieldTime)
    .Func("lastFrameDelta", &TestRuntime::lastFrameDelta)
    .SquirrelFunc("queryItem", &test_runtime_query_item, 2, "xs")
    .Func("markFailed", &TestRuntime::markFailed)
    .Func("testsRunning", &TestRuntime::testsRunning)
    .Func("currentlyRunningTest", &TestRuntime::currentlyRunningTest)
    /**/;
}

//==================================================================================================

void TestRuntime::threadLoop()
{
  bool hidingWindow = false;
  while (running)
  {
    WinAutoLock lock(queueMutex);
    while (testQueue.empty() && running)
      threadWait(lock, 100);

    if (!running)
      break;

    TestCase *test = testQueue.front();
    testQueue.erase(testQueue.begin());

    lock.unlock();

    if (debugWindow.hideDuringRun)
    {
      if (editor_core_test_runtime_window_is_visible())
      {
        editor_core_test_runtime_set_window(false);
        hidingWindow = true;
      }
    }

    interlocked_release_store_ptr(runningTest, test);
    try
    {
      ImGui::GetIO().SetAppAcceptingEvents(false);
      debugOptions.drawFakeMousePointer = debugWindow.drawFakeMousePointerDuringRun;
      test->run(*this);
      debugOptions.drawFakeMousePointer = false;
      ImGui::GetIO().SetAppAcceptingEvents(true);
    }
    catch (...)
    {
      test->markFailed();
    }
    interlocked_release_store_ptr(runningTest, (TestCase *)nullptr);

    {
      WinAutoLock doneCheck(queueMutex);
      if (testQueue.empty())
        interlocked_release_store(anyTestRunning, false);
    }
    yieldFrame();

    if (!testsRunning() && hidingWindow)
    {
      editor_core_test_runtime_set_window(true);
      hidingWindow = false;
    }
  }
}

static TestRuntime ec_tr;

class EditorCoreTestRuntimeConsoleCmdHandler : public console::ICommandProcessor
{
public:
  void destroy() override {}

  bool processCommand(const char *argv[], int argc) override
  {
    if (argc < 1)
      return false;

    const dag::Span<const char *> params = make_span(&argv[1], argc - 1);
    int found = 0;

    CONSOLE_CHECK_NAME("test_runtime", "start", 1, 1) { ec_tr.start(); }
    CONSOLE_CHECK_NAME("test_runtime", "stop", 1, 1) { ec_tr.stop(); }
    CONSOLE_CHECK_NAME("test_runtime", "run", 2, 2)
    {
      const char *name = params[0];
      if (!ec_tr.enqueue(name))
      {
        String msg;
        msg.printf(0, "enqueue: no such test registered (%s)", name);
        EDITORCORE->getConsole().addMessage((ILogWriter::MessageType)2, msg.c_str());
      }
    }
    CONSOLE_CHECK_NAME("test_runtime", "run_all", 1, 1) { ec_tr.enqueueAll(TestFilter::ALL, ""); }

    return found;
  }
};

static EditorCoreTestRuntimeConsoleCmdHandler ec_tr_con;

void editor_core_initialize_test_runtime()
{
  add_con_proc(&ec_tr_con);

  // Register your cpp tests here!
}

TestRuntime *editor_core_test_runtime_get_instance() { return &ec_tr; }

void editor_core_test_runtime_begin_frame() { ec_tr.frameBegin(::dagor_game_act_time); }

void editor_core_test_runtime_end_frame() { ec_tr.frameEnd(); }

static void editor_core_test_runtime_window() { ec_tr.window().updateImgui(); }

void editor_core_test_runtime_set_window(bool visible) { imgui_window_set_visible("Debug", "Test Runtime", visible); }

bool editor_core_test_runtime_window_is_visible() { return imgui_window_is_visible("Debug", "Test Runtime"); }

REGISTER_IMGUI_WINDOW("Debug", "Test Runtime", editor_core_test_runtime_window);
