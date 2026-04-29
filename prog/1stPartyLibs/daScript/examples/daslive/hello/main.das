options gen2

require live/glfw_live
require opengl/opengl_boost
require live/live_commands
require live/live_api
require daslib/json
require daslib/json_boost
require live_host

// --- State ---

var bg_r = 0.2f
var bg_g = 0.3f
var bg_b = 0.5f
var frame_count : int = 0

// --- Live commands ---

[live_command]
def set_color(input : JsonValue?) : JsonValue? {
    if (input != null && input.value is _object) {
        let tab & = unsafe(input.value as _object)
        var rv = tab?["r"] ?? null
        if (rv != null && rv.value is _number) {
            bg_r = float(rv.value as _number)
        }
        var gv = tab?["g"] ?? null
        if (gv != null && gv.value is _number) {
            bg_g = float(gv.value as _number)
        }
        var bv = tab?["b"] ?? null
        if (bv != null && bv.value is _number) {
            bg_b = float(bv.value as _number)
        }
    }
    return JV("\{\"r\": {bg_r}, \"g\": {bg_g}, \"b\": {bg_b}}")
}

// --- Lifecycle ---

[export]
def init() {
    live_create_window("Hello daslive", 640, 480)
    print("hello: init (is_reload={is_reload()})\n")
    if (!is_reload()) {
        frame_count = 0
    }
}

[export]
def update() {
    if (!live_begin_frame()) {
        return
    }
    frame_count++
    if (frame_count % 300 == 0) {
        print("hello: frame {frame_count}, dt={get_dt()}, uptime={get_uptime()}\n")
    }
    var w, h : int
    live_get_framebuffer_size(w, h)
    glViewport(0, 0, w, h)
    glClearColor(bg_r, bg_g, bg_b, 1.0)
    glClear(GL_COLOR_BUFFER_BIT)
    live_end_frame()
}

[export]
def shutdown() {
    print("hello: shutdown (frames={frame_count})\n")
    live_destroy_window()
}

// Dual-mode: also works with regular daslang.exe
[export]
def main() {
    init()
    while (!exit_requested()) {
        update()
    }
    shutdown()
}
