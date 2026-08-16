# Node Editor Example

Demonstrates [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) bindings via the `dasImguiNodeEditor` daspkg package (`dasImgui` itself ships built-in with the daslang tree).

## Setup

```bash
cd examples/node-editor
daslang.exe ../../utils/daspkg/main.das -- install
```

This installs `dasImguiNodeEditor` into `modules/` and builds the C++ shared modules automatically (`dasImgui` is part of this daslang tree — nothing to install).

## Run

```bash
daslang.exe -project_root . imgui_node_editor_basic.das
```

## Requirements

- daslang SDK (built with `DAS_GLFW_DISABLED=OFF`)
- CMake 3.16+
- C++17 compiler
- OpenGL
