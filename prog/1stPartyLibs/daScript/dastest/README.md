# daScript testing framework

## Introduction

Inspired by [Golang testing framework](https://pkg.go.dev/testing), this framework provides a simple way to write unit tests for your scripts.

## Usage

- `dascript dastest.das -- --test folderWithScriptsOrSingleScript [--test anotherPath]`

### dastest.das arguments
- `--test`: Path to the folder with scripts or single script to test
- `--test-names <namePrefix>`: Run top-level tests matching "namePrefix", such as "namePrefix1"
- `--uri-paths`: Print uri paths instead of file paths (vscode friendly)
- `--color`: Print colored output
- `--verbose`: Print verbose output
- `--timeout <seconds>`: If a test binary runs longer than duration d, panic. If d is 0, the timeout is disabled. The default is 10 minutes
- `--isolated-mode`: Run tests in isolated processes, useful to catch crashes

#### Internal arguments
- `--run`: Path to the single script file to run tests in isolated mode
