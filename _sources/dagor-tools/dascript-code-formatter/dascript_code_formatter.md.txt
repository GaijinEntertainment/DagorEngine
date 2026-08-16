# daScript Code Formatter

## Manual Execution

To automatically format project code and its dependencies, run the following
script:

```sh
$<project_name>/prog/das-fmt.bat
```

## IDE Integration

Any IDE can execute the same batch file:

```sh
$<project_name>/prog/das-fmt.bat
```

You can also specify only the currently opened file. For example, `${file}`
represents the file currently opened in the editor:

```sh
"../tools/das-aot/<project_name>_aot_compiler.exe" "../../prog/1stPartyLibs/daScript/das-fmt/dasfmt.das" -- ^
--path ${file} --base-path ../../prog/1stPartyLibs/daScript/das-fmt --das-root ../../prog/1stPartyLibs/daScript
```

Alternatively, you can use the common `das-dev.exe` from the `util` folder:

```sh
pushd d:\<engine_root>\prog\1stPartyLibs\daScript && "../../../tools/util/das-dev.exe" "das-fmt/dasfmt.das" -- --path ${file} && popd
```

### Visual Studio Code Plugin

Starting from version `0.10.10`, the Visual Studio Code [daScript language
support
plugin](https://marketplace.visualstudio.com/items?itemName=profelis.dascript-plugin)
includes built-in support for code formatting using the default shortcut
`Shift`+ `Alt` + `F`.

## Git Pre-Commit Hook

To enforce formatting before commits, add a `pre-commit` script to the
`$<engine_root>/.git/hooks` folder with the following content:

```sh
#!/bin/sh

root="$(git rev-parse --show-toplevel)"

files=$((git diff --cached --name-only --diff-filter=ACMR | grep -Ei "\.das$") || true)

if [ ! -z "${files}" ]; then
    result=0
    paths=""
    for it in $files; do
        paths+="--path ${root}/${it} "
    done

    pushd $root/prog/1stPartyLibs/daScript > /dev/null
    "$root/tools/util/das-dev.exe" "das-fmt/dasfmt.das" -- $paths --exclude-mask 1stPartyLibs/daScript || result=1
    popd > /dev/null

    for it in $files; do
        git add "${it}"
    done

    exit $result
fi
```

```{note}
Ensure that this file is saved with LF line endings.
```

The path to the daScript compiler (`$root/tools/util/das-dev.exe`) can be
replaced with the project-specific Ahead-Of-Time (AOT) compiler.

## Exceptions

To disable automatic formatting or validation for specific files, use one of the
following methods:

- Exclude an entire folder using the `--exclude-mask` argument (as shown in the
  pre-commit hook example). The validator uses
  `$<project_name>/prog/das-fmt-test.bat` files.
- Add the following token anywhere in a `.das` file:

  ```text
  //fmt:ignore-file
  ```

## Visual Studio Code Manual Integration

```{hint}
The VS Code [daScript language
support plugin](https://marketplace.visualstudio.com/items?itemName=profelis.dascript-plugin) (version `0.10.10` and later) provides built-in formatting support (`Shift` + `Alt` + `F`).
```

For manual formatting in VS Code, create a task in
`$<project_name>/prog/.vscode/tasks.json`. Ensure the correct path to the
compiler for your project:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Format current das",
      "type": "shell",
      "command": "../tools/das-aot/<project_name>_aot_compiler.exe",
      "args": [
        "../../prog/1stPartyLibs/daScript/das-fmt/dasfmt.das",
        "--",
        "--verbose",
        "--path",
        "\"${file}\"",
        "--base-path",
        "../../prog/1stPartyLibs/daScript/das-fmt",
        "--das-root",
        "../../prog/1stPartyLibs/daScript"
      ],
      "presentation": {
        "reveal": "never"
      },
      "problemMatcher": []
    }
  ]
}
```

```{note}
If you use the `das_vscode_workspace` utility to generate a VS Code workspace,
this task will be included by default (if the `.vscode` folder does not already
exist).
```

To execute this task for the currently open file:

- Navigate to **Terminal > Run Task... > Format current das**.
- Alternatively, bind a custom shortcut in `keybindings.json` (**File >
  Preferences > Keyboard Shortcuts**, click on icon on top right corner that
  says **Open Keyboard Shortcuts(JSON)**):

```json
[
  {
    "key": "ctrl+alt+d",
    "command": "workbench.action.tasks.runTask",
    "args": "Format current das"
  }
]
```


