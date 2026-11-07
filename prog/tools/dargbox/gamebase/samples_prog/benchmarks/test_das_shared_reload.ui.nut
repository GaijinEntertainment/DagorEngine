// dargbox shares its das environment with daRg. A script loaded after the reload cleanup
// makes daRg own a shared module, a soft reload deletes that module, and the next compile
// must not find it in the daRg module group.
//
// Prints TEST_RESULT ok / TEST_RESULT fail:<reason> and exits with 1 on failure.
// See MULTI_FRAME_TESTS in test.py for the frame budget this needs.
from "%darg/ui_imports.nut" import *
from "dagor.system" import exit
from "console" import command

let state = persist("state", @() { reloaded = false })

function finish(msg, ok) {
  println(msg)
  exit(ok ? 0 : 1)
}

function probeAndReload() {
  local value
  try {
    value = DasFunction(load_das("samples_prog/benchmarks/test_das_shared_reload.das"), "probe")()
  }
  catch (e) {
    return finish($"TEST_RESULT fail:{e}", false)
  }
  if (value != 42)
    return finish($"TEST_RESULT fail:probe returned {value}", false)
  if (state.reloaded)
    return finish("TEST_RESULT ok", true)
  state.reloaded = true
  if (!command("scripts.soft_reload"))
    finish("TEST_RESULT fail:no scripts.soft_reload console command", false)
}

return {
  size = flex()
  rendObj = ROBJ_SOLID
  color = Color(10, 10, 10)
  onAttach = @() gui_scene.setTimeout(0.5, probeAndReload)
}
