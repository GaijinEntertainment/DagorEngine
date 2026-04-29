# Node Editor Example

Demonstrates [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) bindings via the `dasImguiNodeEditor` daspkg package.

## Setup

```bash
cd examples/node-editor
daslang.exe ../../utils/daspkg/main.das -- install
```

This installs `dasImgui` and `dasImguiNodeEditor` into `modules/` and builds the C++ shared modules automatically.

## Run

```bash
daslang.exe -project_root . imgui_node_editor_basic.das
```

## Requirements

- daslang SDK (built with `DAS_GLFW_DISABLED=OFF`)
- CMake 3.16+
- C++17 compiler
- OpenGL
