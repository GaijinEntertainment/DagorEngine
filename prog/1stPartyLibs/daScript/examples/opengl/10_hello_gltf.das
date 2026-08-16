options gen2
options persistent_heap
options gc
require glfw/glfw_boost
require opengl/opengl_boost
require gltf/gltf_boost
require gltf/gltf_gl
require gltf/gltf_pbr
require math
require daslib/math_boost
require strings
require live/live_gc

// dasGLTF — minimal static glTF viewer (OpenGL). Loads a .glb, uploads it to GL, and renders it with
// the metallic-roughness PBR shader under an orbit camera (drag to rotate, scroll to zoom). Sibling to
// 09_hello_mesh but for full glTF PBR. Defaults to the shipped BoomBox; pass a path as argv[1].

def model_path : string {
    // get_command_line_arguments() includes the interpreter + script path, so pick the first
    // argument that actually names a glTF file; otherwise fall back to the shipped BoomBox.
    for (a in get_command_line_arguments()) {
        if (ends_with(a, ".glb") || ends_with(a, ".gltf")) {
            return a
        }
    }
    return "{get_das_root()}/examples/gltf/media/BoomBox.glb"
}

var window : GLFWwindow?
var scene : GltfScene
var model : GltfGlModel
var renderer : GltfPbrRenderer
var center : float3
var radius = 0.001
var camera_distance : float
var camera_yaw = 0.6
var camera_pitch = 0.3
var cursor_prev : float2
var dragging = false
let light = GltfPbrLight()

[export]
def init() {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3)
    window = glfwCreateWindow(1280, 720, "dasGLTF - Hello glTF (PBR)", null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwMakeContextCurrent(window)

    let path = model_path()
    scene <- load_gltf(path)
    if (empty(scene.meshes)) {
        panic("no meshes loaded from {path}")
    }
    model <- upload_gltf(scene)
    renderer <- gltf_pbr_create()

    // auto-frame: orbit the world-space bounding sphere so any model (BoomBox is ~0.02m) fills the view
    let bounds = gltf_scene_bounds(scene)
    center = (bounds._0 + bounds._1) * 0.5
    radius = max(length(bounds._1 - bounds._0) * 0.5, 0.001)

    camera_distance = radius * 2.2
    cursor_prev = glfwGetCursorPos(window)

    unsafe {
        glfwSetScrollCallback(window) @(w, x, y) {
            camera_distance = clamp(camera_distance * (y > 0.0lf ? 0.9 : 1.1), radius * 1.1, radius * 40.0)
        }
        glfwSetMouseButtonCallback(window) @(w, b, a, m) {
            if (b == int(GLFW_MOUSE_BUTTON_1)) {
                dragging = a == int(GLFW_PRESS)
                cursor_prev = glfwGetCursorPos(window)
            }
        }
        glfwSetCursorPosCallback(window) @(w, x, y) {
            let cur = glfwGetCursorPos(window)
            if (dragging) {
                let d = cur - cursor_prev
                camera_yaw -= float(d.x) * 0.01
                camera_pitch = clamp(camera_pitch + float(d.y) * 0.01, -1.5, 1.5)
            }
            cursor_prev = cur
        }
    }
}

[export]
def update() {
    glfwPollEvents()
    var display_w, display_h : int
    glfwGetFramebufferSize(window, display_w, display_h)
    let aspect = display_h != 0 ? float(display_w) / float(display_h) : 1.

    let cp = cos(camera_pitch)
    let eye = center + float3(cp * sin(camera_yaw), sin(camera_pitch), cp * cos(camera_yaw)) * camera_distance
    let view = look_at_rh(eye, center, float3(0., 1., 0.))
    let proj = perspective_rh_minus1_to_1(45.0f * PI / 180., aspect, radius * 0.05, radius * 60.0)

    glViewport(0, 0, display_w, display_h)
    glClearColor(0.10, 0.10, 0.12, 1.0)
    glClearDepth(1.0lf)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LESS)

    gltf_pbr_render(renderer, model, scene, view, proj, eye, light)

    glfwMakeContextCurrent(window)
    glfwSwapBuffers(window)
}

[export]
def shutdown() {
    delete renderer
    delete model
    delete scene
    if (window != null) {
        glfwDestroyWindow(window)
        window = null
    }
    glfwTerminate()
}

var max_frame_limit = 0

def parse_args() {
    let args <- get_command_line_arguments()
    for (i in range(length(args) - 1)) {
        if (args[i] == "--max-frames") {
            max_frame_limit = to_int(args[i + 1])
        }
    }
}

// on web the wasm host auto-drives init/update/shutdown per frame and never calls main
[export]
def main() {
    parse_args()
    init()
    var frame_count = 0
    while (glfwWindowShouldClose(window) == 0) {
        update()
        maybe_collect_gc()
        frame_count ++
        if (max_frame_limit > 0 && frame_count >= max_frame_limit) {
            break
        }
    }
    shutdown()
}
