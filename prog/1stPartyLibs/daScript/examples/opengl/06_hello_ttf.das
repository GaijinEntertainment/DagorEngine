options gen2
require glfw/glfw_boost
require opengl/opengl_boost
require opengl/opengl_ttf
require daslib/defer
require math
require daslib/math_boost

var font : Font

[export]
def main {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    defer <| $() {
        glfwTerminate()
    }
    glfwInitOpenGL(3, 3)
    var window = glfwCreateWindow(1280, 720, "OpenGL - Hello opengl_ttf", null, null)
    if (window == null) {
        panic("can't create window")
    }
    defer <| $() {
        glfwDestroyWindow(window)
    }
    glfwMakeContextCurrent(window)
    // create ttf internals, load font with oversampling=2 for GPU rendering
    create_ttf_objects()
    font <- load_ttf("{get_das_root()}/examples/opengl/droidsansmono.ttf", [oversampling = 2])
    if (!!empty(font.bitmap.bytes)) {
        font <- load_ttf("{get_das_root()}/examples/opengl/media/droidsansmono.ttf", [oversampling = 2])
    }
    if (!!empty(font.bitmap.bytes)) {
        panic("can't load droidsansmono.ttf")
    }
    upload_font_texture(font)
    // create vertex arrays with quads, which render particular text
    var hw_text <- font |> create_quads("Hello, world")
    // make a translate-scale matrix, so that text is in the middle, and 1.0 wide
    let dim = quads_dim(hw_text)
    let model = quads_view(hw_text, float2(3. / (dim.vmax.x - dim.vmin.x)))
    eval_main_loop <| $() {
        if (glfwWindowShouldClose(window) != 0) {
            return false
        }
        glfwPollEvents()
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.
        // render
        glViewport(0, 0, display_w, display_h)
        glClearColor(0.2, 0.2, 0.2, 1.0)
        glClearDepth(1.0lf)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        // model-view-projection setup
        let view = translation(float3(0.0f, 0.0f, -4.0f))
        let projection = perspective_rh(45.0f * PI / 180., aspect, 0.1f, 50.0f)
        let mvp = projection * view * model
        font |> draw_quads(hw_text, mvp)
        font |> draw_quads_2d(hw_text, display_w, display_h, 100, 100)
        // swap buffers
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
        return true
    }
}
