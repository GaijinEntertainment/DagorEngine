options gen2
options persistent_heap = true
options gc

require imgui_app
require glfw/glfw_boost
require imgui/imgui_boost
require imgui/imgui_node_editor_boost
require opengl/opengl_boost
require daslib/static_let
require daslib/safe_addr

var window : GLFWwindow?

def imgui_app(title : string; blk : block) {
    if (glfwInit() == 0) {
        panic("can't init glfw")
    }
    glfwInitOpenGL(3, 3, false, false)
    window = glfwCreateWindow(1280, 720, title, null, null)
    if (window == null) {
        panic("can't create window")
    }
    glfwMakeContextCurrent(window)
    glfwSwapInterval(1)
    CreateContext(null)
    StyleColorsDark(null)
    ImGui_ImplGlfw_InitForOpenGL(window, true)
    ImGui_ImplOpenGL3_Init("#version 330")
    var clear_color = float4(0.45f, 0.55f, 0.60f, 1.00f)
    while (glfwWindowShouldClose(window) == 0) {
        glfwPollEvents()
        ImGui_ImplOpenGL3_NewFrame()
        ImGui_ImplGlfw_NewFrame()
        invoke(blk)
        var display_w, display_h : int
        glfwGetFramebufferSize(window, display_w, display_h)
        glViewport(0, 0, display_w, display_h)
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w)
        glClear(GL_COLOR_BUFFER_BIT)
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData())
        glfwMakeContextCurrent(window)
        glfwSwapBuffers(window)
    }
    ImGui_ImplOpenGL3_Shutdown()
    ImGui_ImplGlfw_Shutdown()
    DestroyContext(null)
    glfwDestroyWindow(window)
    glfwTerminate()
}

var g_Context : EditorContext?

def EditorDemo() {
    if (g_Context == null) {
        unsafe {
            var config = Config()
            g_Context = CreateEditor(addr(config))
        }
    }

    // Fullscreen imgui window (like the C++ example framework)
    var io & = unsafe(imgui::GetIO())
    SetNextWindowPos(float2(0., 0.))
    SetNextWindowSize(io.DisplaySize)
    let flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse | ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoBringToFrontOnFocus
    PushStyleVar(ImGuiStyleVar.WindowBorderSize, 0.0f)
    PushStyleVar(ImGuiStyleVar.WindowRounding, 0.0f)
    Begin("Content", null, flags) <| $() {
        imgui::PopStyleVar(2)

        SetCurrentEditor(g_Context)
        imgui_node_editor::Begin("My Editor", float2(0.0, 0.0f))

        static_let <| $() {
            var firstTime = true
        }

        let nodeA = 1
        let pinAI = 2
        let pinAO = 3
        if (firstTime) {
            SetNodePosition(nodeA, float2(10, 10))
        }
        BeginNode(nodeA) <| $() {
            Text("Node A")
            BeginPin(pinAI, PinKind.Input) <| $() {
                Text("-> In")
            }
            SameLine()
            BeginPin(pinAO, PinKind.Output) <| $() {
                Text("Out ->")
            }
        }

        let nodeB = 4
        let pinBI = 5
        let pinBO = 6
        if (firstTime) {
            SetNodePosition(nodeB, float2(300, 50))
        }
        BeginNode(nodeB) <| $() {
            Text("Node B")
            BeginPin(pinBI, PinKind.Input) <| $() {
                Text("-> In")
            }
            SameLine()
            BeginPin(pinBO, PinKind.Output) <| $() {
                Text("Out ->")
            }
        }

        let linkAB = 7
        Link(linkAB, pinAO, pinBI)

        if (firstTime) {
            NavigateToContent(0.0f)
        }
        firstTime = false

        imgui_node_editor::End()
        SetCurrentEditor(null)
    }
}

[export]
def main() {
    imgui_app("Testing IMGUI node editor") <| $() {
        NewFrame()
        EditorDemo()
        Render()
    }
    DestroyEditor(g_Context)
}
