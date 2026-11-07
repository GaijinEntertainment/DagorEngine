// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_testSystem.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_wndGlobal.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/propPanel.h>

#include <util/dag_console.h>
#include <coolConsole/coolConsole.h>
#include <coolConsole/conBatch.h>
#include <debug/dag_except.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <sqmodules/sqmodules.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/sqModulesDagor.h>
#include <squirrel/sqvm.h>
#include <sqasync.h>

#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_clipboard.h>
#include <perfMon/dag_cpuFreq.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_strUtil.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <EASTL/algorithm.h>
#include <EASTL/string_view.h>

typedef struct SQVM *HSQUIRRELVM;

// Source file and line of the CALL instruction that invoked the current native function.
// filepath aliases the calling closure's source string, which the VM owns.
static void get_sq_native_call_site(HSQUIRRELVM vm, const char *&filepath, int &line)
{
  filepath = nullptr;
  line = 0;

  // [cssize - 1] is this native closure's own frame, [cssize - 2] the Quirrel caller.
  const SQInteger cssize = vm->_callsstacksize;
  if (cssize < 2)
    return;

  SQVM::CallInfo &ci = vm->_callsstack[cssize - 2];
  SQFrameInfo fi;
  sq_get_frame_info(ci, ci._ip - 1, fi); // _ip - 1 is the call site, per sqvm.h
  filepath = fi.source;
  line = (int)fi.line;
}

//==================================================================================================

TestCallback::TestCallback(TestFunc test_func) : testFunc(test_func) {}

TestCallback::~TestCallback() {}

void TestCallback::start(TestRuntime &) {}

bool TestCallback::update(TestRuntime &rt) { return testFunc(rt); }

//==================================================================================================

TestCase::TestCase(const char *name, eastl::shared_ptr<TestCallback> tc, const char *filepath, int line_number) :
  testName(name), testCallback(tc), state(TestState::IDLE), filePath(filepath ? filepath : ""), line(line_number)
{
  if (filePath.empty() || line < 1)
  {
    filePath.clear();
    line = 0;
  }
}

void TestCase::resetState()
{
  reasonText.clear();
  state = TestState::IDLE;
}

void TestCase::markFailed(const char *reason)
{
  if (reason && *reason && reasonText.empty())
    reasonText = reason;

  markFailed();
}

void TestCase::start(TestRuntime &rt)
{
  reasonText.clear();
  state = TestState::RUNNING;
  testCallback->start(rt);
}

bool TestCase::update(TestRuntime &rt)
{
  if (!testCallback->update(rt))
    return false;

  if (state == TestState::RUNNING)
    state = TestState::PASSED;
  return true;
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
    .Func("markFailed", static_cast<void (TestCase::*)()>(&TestCase::markFailed))
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

  // ImGui drops the OS backend's mouse viewport while a test runs, so set the test mouse viewport with this event.
  const ImGuiViewportP *viewport = ImGui::FindHoveredViewportFromPlatformWindowStack(ImVec2(x, y));
  io.AddMouseViewportEvent(viewport ? viewport->ID : 0);

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

void TestInput::queueMouseButton(int button, bool down)
{
  runtime.queueInputStep([button, down]() {
    addMouseButtonEvent(button, down);
    return true;
  });
  runtime.queueWaitFrame();
}

void TestInput::queueMousePosition(Point2 at)
{
  runtime.queueInputStep([at]() {
    addMousePosEvent(at);
    return true;
  });
  runtime.queueWaitFrame();
}

void TestInput::queueMouseLerp(Point2 from, Point2 to, float over_seconds)
{
  runtime.queueInputStep([this, from, to, over_seconds, elapsed = 0.0f]() mutable {
    if (elapsed >= over_seconds)
      return true;
    addMousePosEvent(::lerp(from, to, elapsed / over_seconds));
    elapsed += runtime.lastFrameDelta();
    return false;
  });
}

void TestInput::queueKey(int key, bool down)
{
  runtime.queueInputStep([key, down]() {
    addKeyEvent(key, down);
    return true;
  });
  runtime.queueWaitFrame();
}

void TestInput::queueKeyWithModifier(int key, int modifier, bool down)
{
  runtime.queueInputStep([key, modifier, down]() {
    addKeyWithModifierEvent(key, modifier, down);
    return true;
  });
}

Sqrat::Object TestInput::mouseButtonDown(int button)
{
  return runtime.inputAction([&] {
    queueMouseButton(button, true);
    return true;
  });
}

Sqrat::Object TestInput::mouseButtonUp(int button)
{
  return runtime.inputAction([&] {
    queueMouseButton(button, false);
    return true;
  });
}

Sqrat::Object TestInput::mouseClick(int button)
{
  return runtime.inputAction([&] {
    queueMouseButton(button, true);
    queueMouseButton(button, false);
    return true;
  });
}

Sqrat::Object TestInput::mouseClickModifier(int button, int modifier)
{
  return runtime.inputAction([&] {
    queueKey(modifier, true);
    queueMouseButton(button, true);
    queueMouseButton(button, false);
    queueKey(modifier, false);
    return true;
  });
}

Sqrat::Object TestInput::mousePosition(Point2 at)
{
  return runtime.inputAction([&] {
    queueMousePosition(at);
    return true;
  });
}

Sqrat::Object TestInput::mouseMove(Point2 from, Point2 to, float over_seconds)
{
  return runtime.inputAction([&] {
    queueMousePosition(from);
    queueMouseLerp(from, to, over_seconds);
    // guarantee exact final position
    queueMousePosition(to);
    return true;
  });
}

Sqrat::Object TestInput::keyDown(int key)
{
  return runtime.inputAction([&] {
    queueKey(key, true);
    return true;
  });
}

Sqrat::Object TestInput::keyUp(int key)
{
  return runtime.inputAction([&] {
    queueKey(key, false);
    return true;
  });
}

Sqrat::Object TestInput::keyPress(int key)
{
  return runtime.inputAction([&] {
    queueKey(key, true);
    queueKey(key, false);
    return true;
  });
}

Sqrat::Object TestInput::keyPressModifier(int key, int modifier)
{
  return runtime.inputAction([&] {
    queueKeyWithModifier(key, modifier, true);
    runtime.queueWaitFrame();
    queueKeyWithModifier(key, modifier, false);
    runtime.queueWaitFrame();
    return true;
  });
}

Sqrat::Object TestInput::keyHeld(int key, float over_seconds)
{
  return runtime.inputAction([&] {
    runtime.queueInputStep([key]() {
      addKeyEvent(key, true);
      return true;
    });
    runtime.queueWaitTime(over_seconds);
    queueKey(key, false);
    return true;
  });
}

Sqrat::Object TestInput::keyHeldModifier(int key, int modifier, float over_seconds)
{
  return runtime.inputAction([&] {
    queueKeyWithModifier(key, modifier, true);
    runtime.queueWaitTime(over_seconds);
    queueKeyWithModifier(key, modifier, false);
    runtime.queueWaitFrame();
    return true;
  });
}

Sqrat::Object TestInput::characterInput(const char *text, float between_seconds)
{
  return runtime.inputAction([&] {
    const char *textEnd = text + strlen(text);
    while (*text != 0)
    {
      unsigned int c = 0;
      text += ImTextCharFromUtf8(&c, text, textEnd);
      runtime.queueInputStep([c]() {
        addInputCharacter(c);
        return true;
      });
      if (between_seconds > 0.0f)
        runtime.queueWaitTime(between_seconds);
      else
        runtime.queueWaitFrame();
    }
    return true;
  });
}

Sqrat::Object TestInput::mouseDrag(int button, Point2 from, Point2 to, float over_seconds)
{
  return runtime.inputAction([&] {
    queueMousePosition(from);
    queueMouseButton(button, true);
    queueMouseLerp(from, to, over_seconds);
    queueMousePosition(to);
    queueMouseButton(button, false);
    return true;
  });
}

//==================================================================================================

bool TestInput::queueMouseMoveItem(const char *path, float over_seconds, const char *filepath, int line)
{
  ImGuiID id, parentId;
  ImRect bb;
  if (imgui_test_runtime_query_item(path, id, bb, parentId))
  {
    const Point2 from(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
    const Point2 to(bb.GetCenter().x, bb.GetCenter().y);
    queueMousePosition(from);
    queueMouseLerp(from, to, over_seconds);
    queueMousePosition(to);
    return true;
  }
  else if (runtime.failOnInvalidInputQuery)
  {
    const String reason(0, "query path \"%s\" not found", path);
    runtime.markFailed(filepath, line, reason.c_str());
  }
  return false;
}

Sqrat::Object TestInput::mouseMoveItem(const char *path, float over_seconds, const char *filepath, int line)
{
  return runtime.inputAction([&] { return queueMouseMoveItem(path, over_seconds, filepath, line); });
}

Sqrat::Object TestInput::mouseClickItem(const char *path, int button, const char *filepath, int line)
{
  return runtime.inputAction([&] {
    const bool found = queueMouseMoveItem(path, 1.0f, filepath, line);
    if (found)
    {
      queueMouseButton(button, true);
      queueMouseButton(button, false);
    }
    return found;
  });
}

//==================================================================================================

static SQInteger test_input_mouse_move_item(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestInput *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TestInput *> self(vm, 1);
  Sqrat::Var<const char *> path(vm, 2);
  float overSeconds = Sqrat::Var<float>(vm, 3).value;

  const char *filepath;
  int line;
  get_sq_native_call_site(vm, filepath, line);

  Sqrat::Object result = self.value->mouseMoveItem(path.value, overSeconds, filepath, line);
  sq_pushobject(vm, result.GetObject());
  return 1;
}

static SQInteger test_input_mouse_click_item(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestInput *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TestInput *> self(vm, 1);
  Sqrat::Var<const char *> path(vm, 2);
  int button = Sqrat::Var<int>(vm, 3).value;

  const char *filepath;
  int line;
  get_sq_native_call_site(vm, filepath, line);

  Sqrat::Object result = self.value->mouseClickItem(path.value, button, filepath, line);
  sq_pushobject(vm, result.GetObject());
  return 1;
}

/*static*/
void TestInput::register_script_class(HSQUIRRELVM vm)
{
  // Mouse, Keys, Modifier are exported through the "test_runtime" module table, not as globals.
  Sqrat::Class<TestInput, Sqrat::NoConstructor<TestInput>> sqTestInput(vm, "TestInput");
  sqTestInput //
    .Func("mouseButtonDown", &TestInput::mouseButtonDown)
    .Func("mouseButtonUp", &TestInput::mouseButtonUp)
    .Func("mouseClick", &TestInput::mouseClick)
    .Func("mouseClickModifier", &TestInput::mouseClickModifier)
    .Func("mousePosition", &TestInput::mousePosition)
    .Func("mouseMove", &TestInput::mouseMove)
    .Func("keyDown", &TestInput::keyDown)
    .Func("keyUp", &TestInput::keyUp)
    .Func("keyPress", &TestInput::keyPress)
    .Func("keyPressModifier", &TestInput::keyPressModifier)
    .Func("keyHeld", &TestInput::keyHeld)
    .Func("keyHeldModifier", &TestInput::keyHeldModifier)
    .Func("characterInput", &TestInput::characterInput)
    .Func("mouseDrag", &TestInput::mouseDrag)
    .SquirrelFunc("mouseMoveItem", &test_input_mouse_move_item, 3, "xsn")
    .SquirrelFunc("mouseClickItem", &test_input_mouse_click_item, 3, "xsn")
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

static bool iconsCached = false;
static PropPanel::IconId folderIconId = PropPanel::IconId::Invalid;
static PropPanel::IconId deleteIconId = PropPanel::IconId::Invalid;
static PropPanel::IconId reloadIconId = PropPanel::IconId::Invalid;

void TestWindow::updateImgui()
{
  if (!iconsCached)
  {
    folderIconId = PropPanel::load_icon("folder");
    deleteIconId = PropPanel::load_icon("delete");
    reloadIconId = PropPanel::load_icon("reset");
    iconsCached = true;
  }

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

      auto deleteRef = PropPanel::get_im_texture_id_from_icon_id(deleteIconId);
      constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
      if (ImGui::BeginTable("test_table", 3, tableFlags))
      {
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_NoHeaderLabel);
        ImGui::TableHeadersRow();

        eastl::string_view filter = to_string_view(filterText);
        ImGuiStyle &style = ImGui::GetStyle();
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
          if (!info.filePath.empty() && ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::Text("%s:%d", info.filePath.c_str(), info.sourceLine);
            ImGui::EndTooltip();
          }
          if (info.state == TestState::FAILED && !info.failReason.empty())
          {
            ImGui::PushStyleColor(ImGuiCol_Text, state_color(TestState::FAILED));
            ImGui::TextWrapped("%s", info.failReason.c_str());
            ImGui::PopStyleColor();
          }

          // Delete column
          ImGui::TableSetColumnIndex(2);
          String deleteId;
          deleteId.setStrCat("##Delete_", info.name.c_str());
          float backupPaddingY = style.FramePadding.y;
          style.FramePadding.y = 0.0f;
          ImGui::BeginDisabled(runtime.testsRunning());
          if (ImGui::ImageButton(deleteId.c_str(), deleteRef, indicatorSize))
            runtime.deleteTest(info.name);
          ImGui::EndDisabled();
          style.FramePadding.y = backupPaddingY;
        }

        ImGui::EndTable();
      }

      if (runtime.testCount() > 0)
      {
        ImGui::BeginDisabled(runtime.testsRunning());
        if (ImGui::ImageButton("Delete all", deleteRef, indicatorSize))
          runtime.deleteAllTests();
        ImGui::SameLine();
        ImGui::TextUnformatted("Delete all tests");
        ImGui::EndDisabled();
      }

      if (runtime.testsRunning())
      {
        TestCase *running = runtime.currentlyRunningTest();
        const int remaining = runtime.getRemainingCount();
        const int total = runtime.getTotalEnqueued();
        const int done = max(0, total - remaining);
        runStatusText.printf(0, "Running test(s): \"%s\" %d/%d", running ? running->name().c_str() : "...", done, total);
      }
      else
        runStatusText = "Running test(s): 0";
      ImGui::TextUnformatted(runStatusText.c_str());

#if _TARGET_PC_WIN
      ImGui::BeginDisabled(!runtime.testsRunning());

      // ImGui input is blocked during test execution. Falling back to OS-level click detection.
      bool stopClicked = ImGui::Button("Stop");
      const ImRect stopBtnRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
      if (wingw::is_key_pressed(wingw::V_LBUTTON))
      {
        int osX = 0, osY = 0;
        wingw::get_mouse_pos(osX, osY);
        stopClicked = stopBtnRect.Contains(ImVec2(osX, osY));
      }
      if (stopClicked)
        runtime.clearQueue();

      ImGui::EndDisabled();
#endif

      ImGui::EndDisabled(); // !running

      ImGui::EndDisabled(); // !enabled

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("GUI Items"))
    {
      ImGuiContext *g = ImGui::GetCurrentContext();
      bool enabled = g->TestEngineHookItems;
      ImGuiTestRuntimeOptions *options = static_cast<ImGuiTestRuntimeOptions *>(g->TestEngine);
      // Rebind if enabled externally (e.g. console command) with no options pointer.
      if (enabled && options == nullptr)
      {
        options = &runtime.options();
        imgui_test_runtime_set(enabled, options);
      }
      if (ImGui::Checkbox("Test Runtime Set", &enabled))
      {
        if (options == nullptr)
          options = &runtime.options();
        imgui_test_runtime_set(enabled, options);
      }

      ImGui::BeginDisabled(!enabled);

      ImGui::Checkbox("Draw fake mouse pointer during run", &drawFakeMousePointerDuringRun);

      ImGuiTestRuntimeOptions &rtOptions = runtime.options();
      ImGui::Checkbox("Draw info of hovered GUI item", &rtOptions.drawHovered);

      ImGui::BeginDisabled(!rtOptions.drawHovered);

      ImGui::Checkbox("Ctrl+C: copy path of hovered GUI item", &rtOptions.copyHoveredPath);
      if (rtOptions.copyHoveredPath && !rtOptions.hoveredPath.empty())
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused))
          clipboard::set_clipboard_utf8_text(rtOptions.hoveredPath.c_str());

      ImGui::EndDisabled(); // !drawHovered

      ImGui::EndDisabled(); // !enabled

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Script"))
    {
      ImGui::BeginDisabled(runtime.testsRunning());

      auto folderRef = PropPanel::get_im_texture_id_from_icon_id(folderIconId);
      const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
      if (ImGui::ImageButton("##scriptOpenFile", folderRef, fontSizedIconSize))
      {
        auto wndManager = EDITORCORE->getWndManager();
        void *hwnd = wndManager ? wndManager->getMainWindow() : nullptr;
        const char *mount = EDITORCORE->getTestScriptMount();
        const char *mountPath = (mount && *mount == '%') ? dd_get_named_mount_path(mount + 1) : nullptr;
        String path = wingw::file_open_dlg(hwnd, "Load script...",
          "Quirrel (*.nut)|*.nut|"
          "All files(*.*)|*.*",
          "nut", mountPath ? mountPath : sgg::get_exe_path_full(), scriptFilenameText.c_str());
        if (path.length() > 0)
          scriptFilenameText = path;
      }

      const bool scriptInputWasFocused = scriptInputFocused;
      PropPanel::ImguiHelper::inputTextWithEnterWorkaround("##scriptFilename", "Script filename", &scriptFilenameText,
        scriptInputWasFocused);
      scriptInputFocused = ImGui::IsItemFocused();

      TestScriptModule &scripts = runtime.testScripts();
      if (ImGui::Button("Load script"))
      {
        if (scriptFilenameText != scripts.getScriptFilename())
        {
          runtime.deleteAllTests();
          scripts.bindScript(scriptFilenameText.c_str());
          currentScriptText.clear();
        }
      }

      const bool noScript = scripts.getScriptFilename().empty();
      ImGui::BeginDisabled(noScript);

      auto reloadRef = PropPanel::get_im_texture_id_from_icon_id(reloadIconId);
      if (ImGui::ImageButton("Reload script", reloadRef, fontSizedIconSize))
      {
        runtime.deleteAllTests();
        scripts.reloadScript();
        currentScriptText.clear();
      }
      PropPanel::set_previous_imgui_control_tooltip(ImGui::GetItemID(), "Reload script");

      if (currentScriptText.empty())
      {
        currentScriptText.append("Current script: ");
        if (!scripts.getScriptFilename().empty())
          currentScriptText.append(scripts.getScriptFilename());
        else
          currentScriptText.append("--");
      }
      ImGui::TextUnformatted(currentScriptText.c_str());

      ImGui::EndDisabled(); // noScript

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
  G_VERIFY(SQ_SUCCEEDED(sqasync::bind(vm)));

  moduleMgr = new SqModules(vm, &sq_modules_dagor_file_access);
  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerIoStreamLib();
  moduleMgr->registerIoLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();
  moduleMgr->registerAsyncLib();

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

HSQUIRRELVM TestScriptModule::getVM() const { return moduleMgr->getVM(); }

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

  Sqrat::Object exports;
  Sqrat::string errMsg;
  if (!moduleMgr->requireModule(script_filename, true, script_filename, exports, errMsg))
  {
    logerr("TestScriptModule script error: %s [in %s]", errMsg.c_str(), script_filename);
    return false;
  }

  scriptFilename = script_filename;

  return true;
}

bool TestScriptModule::reloadScript()
{
  String oldFilename = scriptFilename;
  scriptFilename.clear();

  Sqrat::Object exports;
  Sqrat::string errMsg;
  if (!moduleMgr->reloadModule(oldFilename, true, oldFilename, exports, errMsg))
  {
    logerr("TestScriptModule script error: %s [in %s]", errMsg.c_str(), oldFilename);
    return false;
  }

  scriptFilename = oldFilename;
  return true;
}

//==================================================================================================

static eastl::function<void(bool)> g_completion_cb;

static void report_run_completed(bool any_failed)
{
  if (auto cb = eastl::move(g_completion_cb))
    cb(any_failed);
}

TestRuntime::~TestRuntime() { stop(); }

void TestRuntime::start()
{
  if (running)
    return;

  G_ASSERT(runState == RunState::Idle);
  running = true;
}

void TestRuntime::stop()
{
  if (!running)
    return;

  running = false;

  if (runningTest && runState == RunState::Running)
    markFailed(nullptr, 0, "the test runtime stopped");

  // tick() reports only a run that ends, thus report a stopped run here, as failed if not all of its tests ran. When the
  // last test is finishing, all of them ran, but tick() has not reported the run yet.
  const bool reportRun = g_completion_cb && testsRunning();
  const bool anyFailed = runState != RunState::Finishing || !testQueue.empty() || anyTestFailed();

  // A stopped test must not continue, thus drop the steps. The steps hold the Futures that the script awaits, thus the
  // script task of the test never resumes. Each stop during a test leaks that task until the tool exits. Replace this if
  // a test task holds more than script memory, such as a file or a level, that must be released before the tool exits.
  inputSteps.clear();
  debugOptions.drawFakeMousePointer = false;
  if (runState == RunState::Running || runState == RunState::Finishing)
    inputClearPending = true;

  runState = RunState::Idle;
  testQueue.clear();
  totalEnqueued = 0;
  runningTest = nullptr;

  if (hidingWindow)
  {
    editor_core_test_runtime_set_window(true);
    hidingWindow = false;
  }

  // The callback could start a new test run, thus call it only after the cleanup.
  if (reportRun)
    report_run_completed(anyFailed);
}

//==================================================================================================

TestCase &TestRuntime::registerTest(const char *name, TestCallback::TestFunc test_func)
{
  return registerTestEx(name, eastl::make_shared<TestCallback>(test_func));
}

TestCase &TestRuntime::registerTestEx(const char *name, const eastl::shared_ptr<TestCallback> &test_callback, const char *filepath,
  int line)
{
  G_ASSERT(!eastl::any_of(registry.begin(), registry.end(), [&](const eastl::unique_ptr<TestCase> &tc) {
    return tc->name() == name;
  }) && "TestRuntime::registerTest: duplicate test name!");
  registry.push_back(eastl::make_unique<TestCase>(name, test_callback, filepath, line));
  return *registry.back();
}

bool TestRuntime::deleteTest(const char *name)
{
  G_ASSERT_RETURN(!testsRunning(), false);
  auto it =
    eastl::find_if(registry.begin(), registry.end(), [&](const eastl::unique_ptr<TestCase> &tc) { return tc->name() == name; });
  if (it == registry.end())
    return false;
  registry.erase(it);
  return true;
}

void TestRuntime::deleteAllTests()
{
  G_ASSERT_RETURN(!testsRunning(), );
  registry.clear();
}

//==================================================================================================

bool TestRuntime::enqueue(const char *name)
{
  auto it =
    eastl::find_if(registry.begin(), registry.end(), [&](const eastl::unique_ptr<TestCase> &tc) { return tc->name() == name; });
  if (it != registry.end())
  {
    testQueue.push_back(it->get());
    ++totalEnqueued;
    return true;
  }
  G_ASSERT_FAIL("TestRuntime::enqueue: name not found in registry!");
  return false;
}

int TestRuntime::enqueueAll(TestFilter state, const char *name)
{
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
  totalEnqueued += count;
  return count;
}

void TestRuntime::clearQueue()
{
  testQueue.clear();
  totalEnqueued = runningTest ? 1 : 0;
}

int TestRuntime::getRemainingCount() const { return (int)testQueue.size() + (runningTest ? 1 : 0); }

bool TestRuntime::anyTestFailed() const
{
  for (const auto &tc : registry)
    if (tc->lastState() == TestState::FAILED)
      return true;
  return false;
}

Tab<TestRuntime::TestInfo> TestRuntime::testInfos() const
{
  Tab<TestInfo> result;
  result.reserve(registry.size());
  for (const auto &tc : registry)
    result.push_back({tc->name(), tc->lastState(), tc->getFilePath(), tc->getSourceLine(), tc->getFailReason()});
  return result;
}

//==================================================================================================

void TestRuntime::frameBegin(float dt)
{
  // ImGui::NewFrame() read the event queue and no UI is submitted yet, thus this frame's widgets see the released keys.
  if (inputClearPending)
  {
    ImGuiIO &io = ImGui::GetIO();
    ImGuiContext &g = *ImGui::GetCurrentContext();

    bool heldButtons[IM_ARRAYSIZE(io.MouseDown)];
    for (int button = 0; button < IM_ARRAYSIZE(heldButtons); ++button)
      heldButtons[button] = io.MouseDown[button];

    bool heldKeys[IM_ARRAYSIZE(io.KeysData)];
    for (int key = 0; key < IM_ARRAYSIZE(heldKeys); ++key)
      heldKeys[key] = io.KeysData[key].Down;

    io.ClearInputKeys();
    io.ClearInputMouse();

    // ClearInputMouse() invalidated the position, so restore it. Some code might not check the position's validity.
    io.MousePos = g.MouseLastValidPos;

    // Report a release for the buttons that were down, so the tools can end the operation, a gizmo move for example.
    // A press the user makes from here still reports a click, because the duration of the button stays at -1.0f.
    for (int button = 0; button < IM_ARRAYSIZE(heldButtons); ++button)
      if (heldButtons[button])
      {
        io.MouseReleased[button] = true;
        io.MouseReleasedTime[button] = g.Time;
      }

    // Report the release of the keys too. ImGui derives it from the previous down duration of the key.
    for (int key = 0; key < IM_ARRAYSIZE(heldKeys); ++key)
      if (heldKeys[key])
        io.KeysData[key].DownDurationPrev = 0.0f;

    // An item that activates on release, a menu item for example, would handle that release as a press. Let no window
    // be hovered in this frame, as ImGui does when the mouse is disabled.
    g.HoveredWindow = g.HoveredWindowUnderMovingWindow = nullptr;

    // Prevent the user input reaching a widget that the test focused.
    ImGui::ClearActiveID();

    // Drop what the test pushed after the last ImGui::NewFrame() read the queue. Those events could press the keys again.
    io.ClearEventsQueue();

    // Test input has been cleared, so accept user input again from here, the earliest point.
    io.SetAppAcceptingEvents(true);

    // Prevent the navigation highlight from activating the held widget on button release.
    g.NavCursorVisible = false;
    g.NavHighlightItemUnderNav = false;

    inputClearFrame = frameCount;
    inputClearPending = false;
  }

  frameDeltaSeconds = dt;
}

void TestRuntime::frameEnd()
{
  ++frameCount;

  // A test step can run a nested work cycle, a C++ test can start a level export for example. The nested frames must
  // not run the test again, and sqasync::pump() is not reentrant.
  if (!running || ticking)
    return;

  ticking = true;
  tick();
  ticking = false;
}

void TestRuntime::queueInputStep(eastl::function<bool()> step) { inputSteps.push_back(eastl::move(step)); }

void TestRuntime::queueWaitFrame()
{
  queueInputStep([waited = false]() mutable {
    if (waited)
      return true;
    waited = true;
    return false;
  });
}

void TestRuntime::queueWaitTime(float seconds)
{
  queueInputStep([seconds, endTime = 0, started = false]() mutable {
    if (!started)
    {
      endTime = get_time_msec() + (int)(seconds * 1000.0f);
      started = true;
    }
    return get_time_msec() >= endTime;
  });
}

Sqrat::Object TestRuntime::inputAction(dag::FunctionRef<bool() const> queue_steps)
{
  // Only a script runs inside the pump. A C++ test can queue more calls in one update().
  if (pumping && !inputSteps.empty())
  {
    const char *filepath;
    int line;
    get_sq_native_call_site(scriptModule.getVM(), filepath, line);
    markFailed(filepath, line, "an input call started before the previous one finished, an await is missing");
  }

  const bool result = queue_steps();

  HSQUIRRELVM vm = scriptModule.getVM();
  HSQOBJECT handle;
  if (SQ_FAILED(sqasync::future_create(vm, &handle)))
    return Sqrat::Object();

  Sqrat::Object future(handle, vm);
  sq_release(vm, &handle);

  queueInputStep([future, result]() {
    HSQUIRRELVM vm = future.GetVM();
    sq_pushbool(vm, result);
    HSQOBJECT value;
    sq_getstackobj(vm, -1, &value);
    sqasync::future_resolve(vm, future.GetObject(), value);
    sq_pop(vm, 1);
    return true;
  });
  return future;
}

Sqrat::Object TestRuntime::yieldFrame()
{
  return inputAction([&] {
    queueWaitFrame();
    return true;
  });
}

Sqrat::Object TestRuntime::yieldTime(float seconds)
{
  return inputAction([&] {
    queueWaitTime(seconds);
    return true;
  });
}

// Returns true if a step finished.
bool TestRuntime::runInputSteps()
{
  // frameEnd() can call this more than once. A step that waits runs once per frame.
  if (inputStepWaitFrame == frameCount)
    return false;

  bool anyFinished = false;
  while (!inputSteps.empty())
  {
    if (!inputSteps[0]())
    {
      inputStepWaitFrame = frameCount;
      break;
    }
    inputSteps.erase(inputSteps.begin());
    anyFinished = true;
  }
  return anyFinished;
}

// Returns true if a script task or a callback ran.
bool TestRuntime::pumpScripts()
{
  pumping = true;
  const SQInteger count = sqasync::pump(scriptModule.getVM());
  pumping = false;
  return count > 0;
}

void TestRuntime::markFailed(const char *filepath, int line, const char *reason)
{
  TestCase *test = currentlyRunningTest();

  // The Tests tab shows the test name already, and the log keeps the full path, thus the stored reason holds only the
  // file name, the line and the given reason.
  String detail, shortDetail;
  if (filepath && *filepath && line > 0)
  {
    detail.printf(0, "%s:%d", filepath, line);
    shortDetail.printf(0, "%s:%d", dd_get_fname(filepath), line);
  }
  if (reason)
  {
    const char *format = detail.empty() ? "%s" : ": %s";
    detail.aprintf(0, format, reason);
    shortDetail.aprintf(0, format, reason);
  }

  if (!detail.empty())
  {
    if (test)
      logerr("Test \"%s\" failed: %s", test->name().c_str(), detail.c_str());
    else
      logerr("Test failed: %s", detail.c_str());
  }

  if (test)
    test->markFailed(shortDetail.c_str());
}

//==================================================================================================

static bool is_future(const Sqrat::Object &obj)
{
  if (obj.GetType() != OT_INSTANCE)
    return false;

  HSQUIRRELVM vm = obj.GetVM();
  const SQInteger top = sq_gettop(vm);
  bool result = false;
  if (SQ_SUCCEEDED(sqasync::future_push_class(vm)))
  {
    sq_pushobject(vm, obj.GetObject());
    result = sq_instanceof(vm) == SQTrue;
  }
  sq_settop(vm, top);
  return result;
}

// Keeps a Sqrat::Function alive (referenced in HSQUIRRELVM) while the owning TestCase is alive
class SQTestCallback : public TestCallback
{
public:
  SQTestCallback(Sqrat::Function &fn) : TestCallback(nullptr), function(fn) {}

  void start(TestRuntime &rt) override
  {
    // Eval only logs a script error.
    Sqrat::optional<Sqrat::Object> value = function.Eval<Sqrat::Object>(&rt);
    if (!value)
    {
      result.Release();
      rt.markFailed(nullptr, 0, "script error, see the debug log");
      return;
    }
    result = eastl::move(*value);

    // The queued input runs only after the body returned, so the body's checks do not see it.
    if (!is_future(result))
    {
      if (rt.hasQueuedInput())
        rt.markFailed(nullptr, 0, "The test queued input but is not async, so it cannot await the input.");
    }
    else if (getTaskState() != "pending")
    {
      // A plain function (not async) can return a settled Future. Finish it here, before its queued input runs.
      finishTask(rt);
    }
  }

  bool update(TestRuntime &rt) override
  {
    // A test function that is not async is done when it returns.
    if (!is_future(result))
      return true;

    if (getTaskState() == "pending")
      return false;

    finishTask(rt);
    return true;
  }

  bool canUpdateWithQueuedInput() const override { return true; }

private:
  Sqrat::string getTaskState() { return Sqrat::Function(result, "getState").Eval<Sqrat::string>().value_or(""); }

  void finishTask(TestRuntime &rt)
  {
    if (getTaskState() == "faulted")
    {
      Sqrat::Function(result, "markHandled").Execute();
      const Sqrat::string fault = Sqrat::Function(result, "getValue").Eval<Sqrat::string>().value_or("");
      const String reason(0, "script error: %s", fault.c_str());
      rt.markFailed(nullptr, 0, reason.c_str());
    }
    // If input is still queued, an input call has no await, and the checks after it read the UI state too early.
    else if (rt.hasQueuedInput())
    {
      rt.markFailed(nullptr, 0, "The test ended before its queued input ran, an await is missing.");
    }

    result.Release();
  }

  Sqrat::Function function;
  Sqrat::Object result;
};

static SQInteger test_runtime_register_test(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  Sqrat::Var<TestRuntime *> self(vm, 1);
  Sqrat::Var<const char *> name(vm, 2);
  Sqrat::Var<Sqrat::Function> func(vm, 3);
  eastl::shared_ptr<SQTestCallback> funcPtr = eastl::make_shared<SQTestCallback>(func.value);

  const char *filepath;
  int line;
  get_sq_native_call_site(vm, filepath, line);

  TestCase &tc = self.value->registerTestEx(name.value, funcPtr, filepath, line);
  Sqrat::Var<TestCase *>::push(vm, &tc);
  return 1;
}

static void test_item_info_to_sq_table(HSQUIRRELVM vm, Sqrat::Table &sqItemInfo, const char *label, uint32_t id, const ImRect &bb,
  uint32_t parent_id, const ImGuiTestItemExternalInfo &ext, const ImGuiTestItemState &state)
{
  sqItemInfo.SetValue("label", label);
  sqItemInfo.SetValue("displayName", ext.displayName);
  sqItemInfo.SetValue("controlType", ext.controlType);
  sqItemInfo.SetValue("controlId", ext.controlId);
  sqItemInfo.SetValue("id", id);
  Sqrat::Table sqBoundingBox(vm);
  sqBoundingBox.SetValue("Max", Point2(bb.Max.x, bb.Max.y));
  sqBoundingBox.SetValue("Min", Point2(bb.Min.x, bb.Min.y));
  sqItemInfo.SetValue("bb", sqBoundingBox);
  sqItemInfo.SetValue("parentId", parent_id);
  // null when only the previous frame submitted the item.
  sqItemInfo.SetValue("hovered", state.hovered ? Sqrat::Object(*state.hovered, vm) : Sqrat::Object(vm));
  sqItemInfo.SetValue("active", state.active ? Sqrat::Object(*state.active, vm) : Sqrat::Object(vm));
  sqItemInfo.SetValue("visible", state.visible);
  sqItemInfo.SetValue("toggledOpen", state.toggledOpen);
  sqItemInfo.SetValue("isWindow", state.isWindow);
}

static SQInteger test_runtime_query_item(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  // Sqrat::Var<TestRuntime *> self(vm, 1);
  Sqrat::Var<const char *> path(vm, 2);

  const char *label;
  uint32_t id;
  ImRect bb;
  uint32_t parentId;
  ImGuiTestItemExternalInfo ext;
  ImGuiTestItemState state;
  if (imgui_test_runtime_query_item(path.value, id, bb, parentId, &label, &ext, &state))
  {
    Sqrat::Table sqItemInfo(vm);
    test_item_info_to_sq_table(vm, sqItemInfo, label, id, bb, parentId, ext, state);
    Sqrat::Var<Sqrat::Table>::push(vm, sqItemInfo);
    return 1;
  }
  sq_pushnull(vm);
  return 1;
}

static SQInteger test_runtime_query_children(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  // Sqrat::Var<TestRuntime *> self(vm, 1);
  Sqrat::Var<const char *> path(vm, 2);

  Sqrat::Array res(vm);
  OnTestChildItemInfoFoundFunc func = [&](const char *label, uint32_t id, ImRect bb, uint32_t parentId,
                                        const ImGuiTestItemExternalInfo &ext, const ImGuiTestItemState &state) {
    Sqrat::Table sqItemInfo(vm);
    test_item_info_to_sq_table(vm, sqItemInfo, label, id, bb, parentId, ext, state);
    res.Append(sqItemInfo);
  };
  int count = imgui_test_runtime_query_children(path.value, func);

  if (count >= 0)
    Sqrat::Var<Sqrat::Array>::push(vm, res);
  else
    sq_pushnull(vm);
  return 1;
}

static SQInteger test_runtime_mark_failed(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<TestRuntime *>(vm))
    return SQ_ERROR;

  // "reason" is an optional parameter. The VM cannot express "1 or 2", so reject more parameters here.
  if (sq_gettop(vm) > 2)
    return sq_throwerror(vm, "markFailed() takes at most one parameter: the reason string");

  Sqrat::Var<TestRuntime *> self(vm, 1);

  // Var<const char *> stringifies, so a null would arrive as "null". Treat it as no "reason".
  const char *reason = nullptr;
  if (sq_gettop(vm) > 1 && sq_gettype(vm, 2) != OT_NULL)
    reason = Sqrat::Var<const char *>(vm, 2).value;

  const char *filepath;
  int line;
  get_sq_native_call_site(vm, filepath, line);

  self.value->markFailed(filepath, line, reason);
  return 0;
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
    .SquirrelFunc("queryChildren", &test_runtime_query_children, 2, "xs")
    .SquirrelFunc("markFailed", &test_runtime_mark_failed, -1, "xs|o")
    .Func("testsRunning", &TestRuntime::testsRunning)
    .Func("currentlyRunningTest", &TestRuntime::currentlyRunningTest)
    /**/;
}

//==================================================================================================

// Returns true if the test is done.
bool TestRuntime::callTest(TestCase &test, bool start)
{
  try
  {
    if (!start)
      return test.update(*this);
    test.start(*this);
    return false;
  }
  catch (const DagorException &e)
  {
    const String reason(0, "C++ exception: %s", e.excDesc ? e.excDesc : "DagorException without description");
    markFailed(nullptr, 0, reason.c_str());
  }
  catch (...)
  {
    markFailed(nullptr, 0, "C++ exception");
  }
  return true;
}

void TestRuntime::finishRunningTest()
{
  // If a test step stopped the runtime, stop() has already ended the test.
  if (!runningTest)
    return;

  // A failed test drops its input on purpose. A C++ test that passed must not end before its queued input ran.
  if (!inputSteps.empty() && runningTest->lastState() == TestState::PASSED)
    markFailed(nullptr, 0, "The test ended before its queued input ran.");
  inputSteps.clear();

  // Must be before clearing the pending input, because ImGui::ClearInputMouse() resets the mouse position.
  debugOptions.drawFakeMousePointer = false;

  // A test can end, or fail, while it holds a key or a mouse button, thus request a clear from frameBegin(). User
  // input will be re-enabled there.
  inputClearPending = true;
  runState = RunState::Finishing;
}

void TestRuntime::tick()
{
  // A test whose steps never wait would otherwise run forever inside one frame. At the cap, the test continues on the
  // next frame, so a test that needs more passes only takes longer.
  static constexpr int MAX_PASSES_PER_FRAME = 100;

  switch (runState)
  {
    case RunState::Idle:
      if (testQueue.empty())
        return;
      // Wait until the input a stopped test left held is released, so that this test does not lose its first input.
      if (inputReleasePending())
        return;
      runningTest = testQueue.front();
      testQueue.erase(testQueue.begin());

      if (debugWindow.hideDuringRun && editor_core_test_runtime_window_is_visible())
      {
        editor_core_test_runtime_set_window(false);
        hidingWindow = true;
      }

      ImGui::GetIO().SetAppAcceptingEvents(false);
      debugOptions.drawFakeMousePointer = debugWindow.drawFakeMousePointerDuringRun;

      runState = RunState::Running;
      if (callTest(*runningTest, true)) //-V595 Not null here, but the test step can stop the runtime.
      {
        finishRunningTest();
        return;
      }
      // If a test step stopped the runtime, stop() has already ended the test.
      if (!runningTest)
        return;
      [[fallthrough]];

    case RunState::Running:
      for (int pass = 0; pass < MAX_PASSES_PER_FRAME; ++pass)
      {
        bool progress = runInputSteps();
        progress = pumpScripts() || progress;
        if (!runningTest)
          return;
        if ((inputSteps.empty() || runningTest->canUpdateWithQueuedInput()) && callTest(*runningTest, false))
        {
          finishRunningTest();
          return;
        }
        if (!progress)
          return;
      }
      return;

    case RunState::Finishing:
      if (inputReleasePending())
        return;

      runningTest = nullptr;
      runState = RunState::Idle;
      if (!testQueue.empty())
        return;

      totalEnqueued = 0;
      if (hidingWindow)
      {
        editor_core_test_runtime_set_window(true);
        hidingWindow = false;
      }
      if (g_completion_cb)
        report_run_completed(anyTestFailed());
      return;
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
      if (ec_tr.isRunning())
      {
        const char *name = params[0];
        if (!ec_tr.enqueue(name))
        {
          String msg;
          msg.printf(0, "enqueue: no such test registered (%s)", name);
          EDITORCORE->getConsole().addMessage((ILogWriter::MessageType)2, msg.c_str());
        }
      }
      else
        EDITORCORE->getConsole().addMessage((ILogWriter::MessageType)2, "run: test runtime is not started");
    }
    CONSOLE_CHECK_NAME("test_runtime", "run_all", 1, 1)
    {
      if (ec_tr.isRunning())
        ec_tr.enqueueAll(TestFilter::ALL, "");
      else
        EDITORCORE->getConsole().addMessage((ILogWriter::MessageType)2, "run_all: test runtime is not started");
    }

    return found;
  }
};

static EditorCoreTestRuntimeConsoleCmdHandler ec_tr_con;

void editor_core_initialize_test_runtime()
{
  add_con_proc(&ec_tr_con);

  // Register your cpp tests here!
}

void editor_core_shutdown_test_runtime()
{
  // The tool is closing, so nothing waits for the report of a stopped run.
  g_completion_cb = nullptr;

  ec_tr.stop();
  del_con_proc(&ec_tr_con);
}

TestRuntime *editor_core_test_runtime_get_instance() { return &ec_tr; }

void editor_core_test_runtime_begin_frame() { ec_tr.frameBegin(::dagor_game_act_time); }

void editor_core_test_runtime_end_frame() { ec_tr.frameEnd(); }

bool editor_core_test_runtime_run_script(const char *script_path, eastl::function<void(bool)> on_complete)
{
  G_ASSERT_RETURN(!g_completion_cb, false);

  // Bind before touching any state, so a load failure leaves the runtime as it was.
  if (!ec_tr.testScripts().bindScript(script_path))
    return false;

  // Item queries need the ImGui item hooks.
  imgui_test_runtime_set(true, &ec_tr.options());

  ec_tr.window().hideDuringRun = true;
  ec_tr.start();

  g_completion_cb = eastl::move(on_complete);
  const int enqueued = ec_tr.enqueueAll(TestFilter::ALL, "");
  if (enqueued <= 0)
  {
    // No tests queued, we must report completion.
    report_run_completed(false);
  }
  return true;
}

static void editor_core_test_runtime_window() { ec_tr.window().updateImgui(); }

void editor_core_test_runtime_set_window(bool visible) { imgui_window_set_visible("Debug", "Test Runtime", visible); }

bool editor_core_test_runtime_window_is_visible() { return imgui_window_is_visible("Debug", "Test Runtime"); }

REGISTER_IMGUI_WINDOW("Debug", "Test Runtime", editor_core_test_runtime_window);
