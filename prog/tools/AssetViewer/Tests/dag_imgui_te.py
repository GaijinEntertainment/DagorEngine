from ctypes import *
from typing import Any, Optional, Tuple, Callable, overload, Union
from imgui_bundle import imgui as imgui
from inspect import getframeinfo, stack


dll: WinDLL = None
get_imgui_test_engine = None
register_imgui_test = None
set_imgui_test_func = None

CLASS_RUN_TEST_FUNC = None
CLASS_REGISTER_TESTS = None


def _prep_lib_func(func, argtypes, restype = None):
    func.argtypes = argtypes
    func.restype = restype
    return func


def register_library(lib: WinDLL, register_tests_callback):
    global dll, get_imgui_test_engine, register_imgui_test, set_imgui_test_func
    dll = lib
    get_imgui_test_engine = _prep_lib_func(dll.get_imgui_test_engine, (), c_void_p)
    register_imgui_test = _prep_lib_func(dll.register_imgui_test, (c_void_p, c_char_p, c_char_p, c_char_p, c_int, ), c_void_p)
    set_imgui_test_func = _prep_lib_func(dll.set_imgui_test_func, (c_void_p, ))
    global CLASS_RUN_TEST_FUNC
    register_run_test_func = dll.register_run_test_func
    register_run_test_func.argtypes = (c_void_p, )
    register_run_test_func(CLASS_RUN_TEST_FUNC)
    global CLASS_REGISTER_TESTS
    CLASS_REGISTER_TESTS = CFUNCTYPE(None)(register_tests_callback)
    dll.register_test_callback(CLASS_REGISTER_TESTS)


WindowFlags = imgui.WindowFlags_
TableFlags = imgui.TableFlags_
PopupFlags = imgui.PopupFlags_
ImVec2 = imgui.ImVec2
ImVec2Like = imgui.ImVec2Like
ImVec4 = imgui.ImVec4
ImVec4Like = imgui.ImVec4Like
Viewport = imgui.Viewport
TableSortSpecs = imgui.TableSortSpecs
MouseButton = imgui.MouseButton_
Key = imgui.Key
Dir = imgui.Dir
SortDirection = imgui.SortDirection
DataType = imgui.DataType_


ItemStatusFlags = imgui.internal.ItemStatusFlags_
DockNode = imgui.internal.DockNode
Axis = imgui.internal.Axis
Window = imgui.internal.Window
ImRect = imgui.internal.ImRect
Context = imgui.internal.Context
LastItemData = imgui.internal.LastItemData
TabBar = imgui.internal.TabBar
InputSource = imgui.internal.InputSource


ID = int
Str = str
Key_None = Key.none
KeyChord = int
TestRunFlags = int
TestLogFlags = int


TestStatus = imgui.test_engine.TestStatus
TestVerboseLevel = imgui.test_engine.TestVerboseLevel
TestItemList = imgui.test_engine.TestItemList
TestRef = imgui.test_engine.TestRef
TestOpFlags = imgui.test_engine.TestOpFlags_
TestItemInfo = imgui.test_engine.TestItemInfo
TestAction = imgui.test_engine.TestAction
TestActionFilter = imgui.test_engine.TestActionFilter


Function_TestRunner = Callable[[Any], None]


Str30 = str
Str256 = str
ImVector_char = list[int]
ImVector_Window_ptr = list[Window]


class TestEngine:

    def __init__(self, engine_ref):
        self.engine_ref = engine_ref


class Test:

    def __init__(self, engine: TestEngine, test_ref, category: bytes, name: bytes) -> None:
        self.engine: TestEngine = engine
        self.test_ref = test_ref
        self.c: bytes = category
        self.category: str = category.decode("utf-8")
        self.n: bytes = name
        self.name: str = name.decode("utf-8")
        self._test_func: Function_TestRunner = None

    @property
    def test_func(self) -> Function_TestRunner:
        return self._test_func

    @test_func.setter
    def test_func(self, value: Function_TestRunner):
        self._test_func = value
        set_imgui_test_func(self.test_ref)


class CaptureArgs:

    def __init__(self, cap_args_ref, ctx_ref):
        self.cap_args_ref = cap_args_ref
        self.ctx_ref = ctx_ref

    @property
    def in_output_file(self) -> str:
        return _prep_lib_func(dll.cap_args_get_in_output_file, (c_void_p,), c_char_p)(self.cap_args_ref)

    @in_output_file.setter
    def in_output_file(self, file: str):
        _prep_lib_func(dll.cap_args_set_in_output_file, (c_void_p, c_char_p))(self.cap_args_ref, file.encode())

    @property
    def in_padding(self) -> float:
        return _prep_lib_func(dll.cap_args_get_in_padding, (c_void_p,), c_float)(self.cap_args_ref)

    @in_padding.setter
    def in_padding(self, padding: float):
        _prep_lib_func(dll.cap_args_set_in_padding, (c_void_p, c_float))(self.cap_args_ref, padding)


class TestContext:

    def __init__(self, test: Test, ctx_ref):
        self.test: Test = test
        self.ctx_ref = ctx_ref
        self._capture_args = None

    # -------------------------------------------------------------------------
    # Public API
    # -------------------------------------------------------------------------

    # Main control

    def finish(self, status: TestStatus = TestStatus.success) -> None:
        _prep_lib_func(dll.ctx_finish, (c_void_p, c_int))(self.ctx_ref, status.value)

    # Main status queries

    def is_error(self) -> bool:
        return _prep_lib_func(dll.ctx_is_error, (c_void_p,), c_bool)(self.ctx_ref)

    # Logging

    def log_debug(self, fmt: str) -> None:
        _prep_lib_func(dll.ctx_log_debug, (c_void_p, c_char_p))(self.ctx_ref, fmt.encode())

    def log_info(self, fmt: str) -> None:
        _prep_lib_func(dll.ctx_log_info, (c_void_p, c_char_p))(self.ctx_ref, fmt.encode())

    def log_warning(self, fmt: str) -> None:
        _prep_lib_func(dll.ctx_log_warning, (c_void_p, c_char_p))(self.ctx_ref, fmt.encode())

    def log_error(self, fmt: str) -> None:
        _prep_lib_func(dll.ctx_log_error, (c_void_p, c_char_p))(self.ctx_ref, fmt.encode())

    def log_basic_ui_state(self) -> None:
        _prep_lib_func(dll.ctx_log_basic_ui_state, (c_void_p,))(self.ctx_ref)

    # Yield, Timing

    def yield_(self, count: int = 1) -> None:
        _prep_lib_func(dll.ctx_yield, (c_void_p, c_int))(self.ctx_ref, count)

    def sleep(self, time_in_second: float) -> None:
        _prep_lib_func(dll.ctx_sleep, (c_void_p, c_float))(self.ctx_ref, time_in_second)

    def sleep_short(self) -> None:
        _prep_lib_func(dll.ctx_sleep_short, (c_void_p,))(self.ctx_ref)

    def sleep_standard(self) -> None:
        _prep_lib_func(dll.ctx_sleep_standard, (c_void_p,))(self.ctx_ref)

    def sleep_no_skip(self, time_in_second: float, framestep_in_second: float) -> None:
        _prep_lib_func(dll.ctx_sleep_no_skip, (c_void_p, c_float, c_float))(self.ctx_ref, time_in_second, framestep_in_second)

    # Base Reference

    # - ItemClick("Window/Button")                --> click "Window/Button"
    # - SetRef("Window"), ItemClick("Button")     --> click "Window/Button"
    # - SetRef("Window"), ItemClick("/Button")    --> click "Window/Button"
    # - SetRef("Window"), ItemClick("//Button")   --> click "/Button"
    # - SetRef("//$FOCUSED"), ItemClick("Button") --> click "Button" in focused window.
    # See https://github.com/ocornut/imgui_test_engine/wiki/Named-References about using ImGuiTestRef in all ImGuiTestContext functions.
    # Note: SetRef() may take multiple frames to complete if specified ref is an item id.
    # Note: SetRef() ignores current reference, so they are always absolute path.

    def set_ref(self, ref: str) -> None:
        _prep_lib_func(dll.ctx_set_ref, (c_void_p, c_char_p))(self.ctx_ref, ref.encode())

    # This is special boundled call because to overcome the following API limitation:
    # https://github.com/ocornut/imgui_test_engine/wiki/Named-References#using-windowinfo-to-easily-access-child-windows
    def set_ref_window(self, window_ref: str) -> None:
        _prep_lib_func(dll.ctx_set_ref_window, (c_void_p, c_char_p))(self.ctx_ref, window_ref.encode())

    # Windows

    def window_close(self, window_ref: Union[TestRef, str]) -> None:
        _prep_lib_func(dll.ctx_window_close, (c_void_p, c_char_p))(self.ctx_ref, window_ref.encode())

    def window_collapse(self, window_ref: Union[TestRef, str], collapsed: bool) -> None:
        _prep_lib_func(dll.ctx_window_collapse, (c_void_p, c_char_p, c_bool))(self.ctx_ref, window_ref.encode(), collapsed)

    def window_focus(self, window_ref: Union[TestRef, str], flags: TestOpFlags = TestOpFlags.none) -> None:
        _prep_lib_func(dll.ctx_window_focus, (c_void_p, c_char_p, c_int))(self.ctx_ref, window_ref.encode(), int(flags))

    def window_bring_to_front(self, window_ref: Union[TestRef, str], flags: TestOpFlags = TestOpFlags.none) -> None:
        _prep_lib_func(dll.ctx_window_bring_to_front, (c_void_p, c_char_p, c_int))(self.ctx_ref, window_ref.encode(), int(flags))

    # Popups

    # Get hash for a decorated ID Path.

    # Miscellaneous helpers

    # Screenshot/Video Captures

    @property
    def capture_args(self) -> CaptureArgs:
        if self._capture_args is None:
            cap_args_ref = _prep_lib_func(dll.ctx_get_capture_args, (c_void_p, ), c_void_p)(self.ctx_ref)
            self._capture_args = CaptureArgs(cap_args_ref, self.ctx_ref)
        return self._capture_args

    def capture_reset(self) -> None:
        self._capture_args = None
        _prep_lib_func(dll.ctx_capture_reset, (c_void_p,))(self.ctx_ref)

    def capture_set_extension(self, ext: str) -> None:
        _prep_lib_func(dll.ctx_capture_set_extension, (c_void_p, c_char_p))(self.ctx_ref, ext.encode())

    def capture_add_window(self, ref: Union[TestRef, str]) -> bool:
        return _prep_lib_func(dll.ctx_capture_add_window, (c_void_p, c_char_p), c_bool)(self.ctx_ref, ref.encode())

    def capture_screenshot_window(self, ref: Union[TestRef, str], capture_flags: int = 0) -> None:
        _prep_lib_func(dll.ctx_capture_screenshot_window, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), capture_flags)

    def capture_screenshot(self, capture_flags: int = 0) -> bool:
        _prep_lib_func(dll.ctx_capture_screenshot, (c_void_p, c_int))(self.ctx_ref, capture_flags)

    # Mouse inputs

    def mouse_move(self, ref: Union[TestRef, str], flags: TestOpFlags = TestOpFlags.none) -> None:
        _prep_lib_func(dll.ctx_mouse_move, (c_void_p, c_char_p))(self.ctx_ref, ref.encode())

    def mouse_click(self, button: MouseButton = 0) -> None:
        _prep_lib_func(dll.ctx_mouse_click, (c_void_p, c_int))(self.ctx_ref, int(button))

    def mouse_click_multi(self, button: MouseButton, count: int) -> None:
        _prep_lib_func(dll.ctx_mouse_click_multi, (c_void_p, c_int, c_int))(self.ctx_ref, int(button), count)

    def mouse_double_click(self, button: MouseButton = 0) -> None:
        _prep_lib_func(dll.ctx_mouse_double_click, (c_void_p, c_int))(self.ctx_ref, int(button))

    def mouse_down(self, button: MouseButton = 0) -> None:
        _prep_lib_func(dll.ctx_mouse_down, (c_void_p, c_int))(self.ctx_ref, int(button))

    def mouse_up(self, button: MouseButton = 0) -> None:
        _prep_lib_func(dll.ctx_mouse_up, (c_void_p, c_int))(self.ctx_ref, int(button))

    def mouse_wheel_x(self, dx: float) -> None:
        _prep_lib_func(dll.ctx_mouse_wheel_x, (c_void_p, c_float))(self.ctx_ref, dx)

    def mouse_wheel_y(self, dy: float) -> None:
        _prep_lib_func(dll.ctx_mouse_wheel_y, (c_void_p, c_float))(self.ctx_ref, dy)

    # Mouse inputs: Viewports

    # Keyboard inputs

    def key_down(self, key_chord: KeyChord) -> None:
        _prep_lib_func(dll.ctx_key_down, (c_void_p, c_int))(self.ctx_ref, key_chord)

    def key_up(self, key_chord: KeyChord) -> None:
        _prep_lib_func(dll.ctx_key_up, (c_void_p, c_int))(self.ctx_ref, key_chord)

    def key_press(self, key_chord: KeyChord, count: int = 1) -> None:
        _prep_lib_func(dll.ctx_key_press, (c_void_p, c_int, c_int))(self.ctx_ref, key_chord, count)

    def key_hold(self, key_chord: KeyChord, time: float) -> None:
        _prep_lib_func(dll.ctx_key_hold, (c_void_p, c_int, c_float))(self.ctx_ref, key_chord, time)

    def key_set_ex(self, key_chord: KeyChord, is_down: bool, time: float = 0.0) -> None:
        _prep_lib_func(dll.ctx_key_set_ex, (c_void_p, c_int, c_bool, c_float))(self.ctx_ref, key_chord, is_down, time)

    def key_chars(self, chars: str) -> None:
        _prep_lib_func(dll.ctx_key_chars, (c_void_p, c_char_p))(self.ctx_ref, chars.encode())

    def key_chars_append(self, chars: str) -> None:
        _prep_lib_func(dll.ctx_key_chars_append, (c_void_p, c_char_p))(self.ctx_ref, chars.encode())

    def key_chars_append_enter(self, chars: str) -> None:
        _prep_lib_func(dll.ctx_key_chars_append_enter, (c_void_p, c_char_p))(self.ctx_ref, chars.encode())

    def key_chars_replace(self, chars: str) -> None:
        _prep_lib_func(dll.ctx_key_chars_replace, (c_void_p, c_char_p))(self.ctx_ref, chars.encode())

    def key_chars_replace_enter(self, chars: str) -> None:
        _prep_lib_func(dll.ctx_key_chars_replace_enter, (c_void_p, c_char_p))(self.ctx_ref, chars.encode())

    # Navigation inputs

    # Low-level queries

    def item_click(self, ref: Union[TestRef, str], button: MouseButton = 0, flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_click, (c_void_p, c_char_p, c_int, c_int))(self.ctx_ref, ref.encode(), int(button), int(flags))

    def item_double_click(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_double_click, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    def item_check(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_check, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    def item_uncheck(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_uncheck, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    def item_open(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_open, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    def item_close(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_close, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    def item_input(self, ref: Union[TestRef, str], flags: TestOpFlags = 0) -> None:
        _prep_lib_func(dll.ctx_item_input, (c_void_p, c_char_p, c_int))(self.ctx_ref, ref.encode(), int(flags))

    # Item/Widgets: Batch actions over an entire scope

    # Item/Widgets: Helpers to easily set a value

    # Item/Widgets: Helpers to easily read a value by selecting Slider/Drag/Input text, copying into clipboard and parsing it.

    # Item/Widgets: Status query

    # Item/Widgets: Drag and Mouse operations

    # Helpers for Tab Bars widgets

    # Helpers for MenuBar and Menus widgets

    # Helpers for Combo Boxes

    # Helpers for Tables

    # Viewports

    # Docking

    # Performances Measurement (use along with Dear ImGui Perf Tool)


def get_test_engine() -> TestEngine:
    global get_imgui_test_engine
    engine_ref = get_imgui_test_engine()
    return TestEngine(engine_ref)


TESTS = {}
register_run_test_func = None


def run_test_func():
    global TESTS
    context_ref = _prep_lib_func(dll.get_current_ctx, (), c_void_p)()
    test_ref = _prep_lib_func(dll.get_ctx_test, (c_void_p, ), c_void_p)(context_ref)
    test: Test = TESTS[test_ref]
    context: TestContext = TestContext(test, context_ref)
    test.test_func(context)


CLASS_RUN_TEST_FUNC = CFUNCTYPE(None)(run_test_func)


def register_test(engine: TestEngine, category: str, name: str) -> Test:
    c = category.encode()
    n = name.encode()
    caller = getframeinfo(stack()[1][0])
    filename = caller.filename.encode()
    line = caller.lineno
    test_ref = register_imgui_test(engine.engine_ref, c, n, filename, line)
    test: Test = Test(engine, test_ref, c, n)
    TESTS[test_ref] = test
    return test

