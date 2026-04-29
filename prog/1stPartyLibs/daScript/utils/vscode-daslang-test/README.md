# daslang Test Explorer

VSCode extension that integrates daslang's `dastest` test framework with the native Test Explorer UI.

## Features

- Discovers `[test]` and `[benchmark]` annotated functions in `.das` files
- Run individual tests, entire files, or all tests at once
- Three run profiles: **Run All**, **Run Tests Only**, **Run Benchmarks Only**
- Benchmark timing stats shown in the Test Explorer sidebar
- Failed test messages with source locations (click to navigate)
- File watcher keeps the test tree in sync as you edit

## Requirements

- VSCode 1.85+
- A built daslang compiler (`bin/Release/daslang.exe` or `bin/Debug/daslang.exe`)
- `dastest/dastest.das` in the workspace

## Installation

### From source

```bash
cd utils/vscode-daslang-test
npm install
npm run compile
npx @vscode/vsce package
```

Then install the generated `.vsix`:

```
code --install-extension vscode-daslang-test-0.1.0.vsix
```

Or in VSCode: **Extensions** sidebar > `...` menu > **Install from VSIX...**

### Development

Open `utils/vscode-daslang-test/` as a workspace in VSCode, then press **F5** to launch the Extension Development Host.

## Configuration

| Setting | Default | Description |
|---|---|---|
| `dascript.compiler` | `""` | Path to daslang compiler (auto-detected from `bin/Release/daslang.exe`) |
| `dascript.dastestPath` | `""` | Path to `dastest/dastest.das` (auto-detected if in the daScript repo) |
| `dascript.testTimeout` | `120` | Test execution timeout in seconds |

## How it works

The extension scans `.das` files for `require dastest/testing_boost` and parses `[test]`/`[benchmark]` annotations via regex. When you run tests, it invokes `dastest` with `--json-file` for structured results and `--bench` for benchmarks. Test names are filtered with `--test-names`/`--bench-names` when running individual items.

Files with `expect` directives (negative compilation tests) are automatically excluded.
