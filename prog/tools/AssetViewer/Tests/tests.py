from ctypes import *
from imgui_bundle import imgui
import dag_imgui_te


MouseButton = imgui.MouseButton_
Key = imgui.Key


def register_cases(engine):

    test_add_asset = dag_imgui_te.register_test(engine, "AV smoke test", "Add asset")
    def test_add_asset_test_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.set_ref("Assets Tree")
        ctx.mouse_move("/TabBar/All")
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/Root")
        ctx.yield_()
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/Root/assets/example/cdk_test_building_cmp")
        ctx.yield_()
        ctx.mouse_click(MouseButton.left)
    test_add_asset.test_func = test_add_asset_test_func


    test_icon_screenshot = dag_imgui_te.register_test(engine, "AV smoke test", "Take screenshot with toolbar")
    def test_icon_screenshot_test_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.key_press(Key.mod_ctrl | Key.mod_shift | Key.z, 1)
        ctx.set_ref_window("/Root/Toolbar")
        ctx.mouse_move("$$0/$$11/ib__screenshot")
        ctx.mouse_click(MouseButton.left)
        ctx.log_warning("Capturing 'Viewport' screenshot!")
    test_icon_screenshot.test_func = test_icon_screenshot_test_func


    test_multiple_screenshot = dag_imgui_te.register_test(engine, "AV smoke test", "Take screenshot from different angles")
    def test_multiple_screenshot_test_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_shift | Key.p, 1)
        ctx.yield_()
        ctx.key_press(Key.mod_ctrl | Key.mod_shift | Key.z, 1)
        ctx.window_focus("Viewport")
        capture = ctx.capture_args
        capture.in_output_file = "test_images/test_capture_perspective.png"
        capture.in_padding = 64
        ctx.capture_add_window("Viewport")
        ctx.capture_screenshot()
        ctx.yield_()
        ctx.capture_reset()
        ctx.log_warning("Capturing Perspective screenshot!")
        ctx.set_ref("")
        ctx.yield_()
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_shift | Key.f, 1)
        capture = ctx.capture_args
        capture.in_output_file = "test_images/test_capture_front.png"
        capture.in_padding = 0
        ctx.capture_add_window("Viewport")
        ctx.capture_screenshot()
        ctx.yield_()
        ctx.capture_reset()
        ctx.log_warning("Capturing Front screenshot!")
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_shift | Key.l, 1)
        capture = ctx.capture_args
        capture.in_output_file = "test_images/test_capture_left.png"
        capture.in_padding = 0
        ctx.capture_add_window("Viewport")
        ctx.capture_screenshot()
        ctx.yield_()
        ctx.capture_reset()
        ctx.log_warning("Capturing Left screenshot!")
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_shift | Key.r, 1)
        capture = ctx.capture_args
        capture.in_output_file = "test_images/test_capture_right.png"
        capture.in_padding = 0
        ctx.capture_add_window("Viewport")
        ctx.capture_screenshot()
        ctx.yield_()
        ctx.capture_reset()
        ctx.log_warning("Capturing Right screenshot!")
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_shift | Key.t, 1)
        capture = ctx.capture_args
        capture.in_output_file = "test_images/test_capture_top.png"
        capture.in_padding = 0
        ctx.capture_add_window("Viewport")
        ctx.capture_screenshot()
        ctx.yield_()
        ctx.capture_reset()
        ctx.log_warning("Capturing Top screenshot!")
    test_multiple_screenshot.test_func = test_multiple_screenshot_test_func


    test_search = dag_imgui_te.register_test(engine, "AV smoke test", "Search bar")
    def test_search_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.set_ref("Assets Tree")
        ctx.mouse_move("/TabBar/All")
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/##searchInput")
        ctx.mouse_click(MouseButton.left)
        ctx.sleep(1.0)
        ctx.key_chars("test_building")
    test_search.test_func = test_search_func


    test_add_favorite = dag_imgui_te.register_test(engine, "AV smoke test", "Add asset to favorites")
    def test_add_favorite_test_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.set_ref("Assets Tree")
        ctx.mouse_move("/TabBar/All")
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/Root")
        ctx.yield_()
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/Root/assets/example/cdk_test_building_cmp")
        ctx.yield_()
        ctx.mouse_click(MouseButton.right)
        ctx.set_ref("//$FOCUSED")
        ctx.mouse_move("/Add to favorites")
        ctx.mouse_click(MouseButton.left)
    test_add_favorite.test_func = test_add_favorite_test_func


    test_remove_favorite = dag_imgui_te.register_test(engine, "AV smoke test", "Remove from favorites")
    def test_remove_favorite_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.set_ref("Assets Tree")
        ctx.mouse_move("/TabBar/Favorites")
        ctx.yield_()
        ctx.mouse_click(MouseButton.left)
        ctx.yield_()
        ctx.mouse_move("/**/Root/assets/example/cdk_test_building_cmp")
        ctx.yield_()
        ctx.mouse_click(MouseButton.right)
        ctx.set_ref("//$FOCUSED")
        ctx.mouse_move("Remove from favorites")
        ctx.mouse_click(MouseButton.left)
        ctx.set_ref("Assets Tree")
        ctx.mouse_move("/TabBar/All")
        ctx.mouse_click(MouseButton.left)
    test_remove_favorite.test_func = test_remove_favorite_func


    test_open_composit = dag_imgui_te.register_test(engine, "AV smoke test", "Open Composit editor, select component")
    def test_open_composit_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("Window")
        ctx.mouse_move("/##MainMenuBar/##menubar/View")
        ctx.mouse_click(MouseButton.left)
        ctx.mouse_move("/##Menu_00/Composit Editor")
        ctx.mouse_click(MouseButton.left)
        ctx.sleep(1.0)
        ctx.set_ref("Composit Outliner\/tree_EE9CF335\/5FB8E8AF")
        ctx.mouse_move("/cdk_test_building_cmp/cdk_ladder_cmp")
        ctx.mouse_click(MouseButton.left)
    test_open_composit.test_func = test_open_composit_func


    test_save_asset = dag_imgui_te.register_test(engine, "AV smoke test", "save_asset")
    def test_save_asset_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("")
        ctx.yield_()
        ctx.window_focus("Viewport")
        ctx.key_press(Key.mod_ctrl | Key.s, 1)
        ctx.yield_()
    test_save_asset.test_func = test_save_asset_func


    test_exit = dag_imgui_te.register_test(engine, "AV smoke test", "Exit AssetViewer")
    def test_exit_func(ctx: dag_imgui_te.TestContext) -> None:
        ctx.set_ref("Window")
        ctx.mouse_move("/##MainMenuBar/##menubar/Exit")
        ctx.mouse_click(MouseButton.left)
    test_exit.test_func = test_exit_func
