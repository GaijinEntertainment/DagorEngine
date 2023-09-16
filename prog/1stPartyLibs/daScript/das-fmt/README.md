## Introduction

dasfmt is a tool that automatically formats daScript source code.

## Usage

- `dascript dasfmt.das -- --path folderWithScriptsOrSingleScript [--path anotherPath]`

### dastest.das arguments
- `--path`: Path to the folder with scripts or single script name
- `--exclude-mask <mask>`: A path with given masks as part of the path will be ignored
- `--verify`: (dry run) Doesn't change files, just make sure that all files are already formatted
- `--t`: Max number of used threads
