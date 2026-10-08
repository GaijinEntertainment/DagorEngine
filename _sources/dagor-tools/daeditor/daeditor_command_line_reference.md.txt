# daEditorX Command-Line Reference

## Command-Line Arguments

| Argument | Description |
|---|---|
| `<path>.level.blk` | Level to open on startup. |
| `<path>.dcmd` | Console-batch script to run. |
| `-ws:<name>` | Selects a named workspace by name. |
| `-async_batch` | Runs the positional `.dcmd` batch asynchronously. Without it, the batch runs synchronously and daEditorX exits when done. (The syntax is different compared to Asset Viewer.) |
| `-no_dabuild` | Disables daBuild-cache/on-demand asset building entirely. |
| `-min_dabuild` | Minimizes daBuild usage: only unscanned textures get built, everything else reads from `.grp` packs. |
| `-no_src_assets` | Skips scanning source asset folders (uses only cooked/prebuilt data). |
| `-enable:<plugin>` (repeatable) | Force-enables a named editor plugin regardless of auto-detection/config. |
| `-disable:<plugin>` (repeatable) | Force-disables a named editor plugin. |
| `-include_dll_re:<regex>` (repeatable) | Restricts plugin-DLL loading to DLLs whose name matches this regex. |
| `-exclude_dll_re:<regex>` (repeatable) | Excludes plugin DLLs whose name matches this regex from loading. |
| `-drv:<name>` | Selects the 3D driver to use (e.g. `stub`, `auto`, `dx11`, `dx12`, `vulkan`). `stub` also forces `-min_dabuild` behavior. |
| `-lateSceneSelect` | Defers **Scene View** streaming folder selection until after level load. |
| `-quiet` | Suppresses interactive dialogs (headless/batch-safe). |
| `-no_level_file` | Disables per-debug level (e.g.: warning, error, fatal) debug log files. |
| `-copy_log_to_stdout` | Writes the debug log messages to stdout. |
| `-fatals_to_stderr` | Also prints fatal error message + callstack to stderr. |
| `-logerr_to_stderr` | Mirrors `logerr()` messages to stderr as they happen. |
| `-attach_parent_console` | Attaches to the parent process's console (paired with `copy_log_to_stdout` for `cmd.exe` redirection). Windows-only. |
| `-noeh` | Disables the top-level SEH wrapper; crashes propagate instead of being caught. |

## `-config:` Settings

| Setting | Default | Description |
|---|---|---|
| `-config:debug/profiler:t=<all\|gpu\|cpu\|platform\|off>` | off | Legacy switch for which daProfiler event groups are captured: `all` enables platform+CPU+GPU events, sampling, and spike saving; `gpu` enables CPU+GPU events; `cpu` enables CPU events only; `platform` enables platform events only; `off` disables profiling. |
| `-config:debug/profiler/auto_dump_startup:b=true` | false | Dumps a daProfiler capture immediately after startup loading completes. It is saved beside the log file. |
| `-config:debug/profiler/auto_dump:b=true` | false | Dumps a daProfiler capture on application shutdown, before the final tick. It is saved beside the log file. |
| `-config:debug/dontUseCpuInBackground:b=<...>` | true | Allows disabling the performance limitations that are set by default if the application is not active. |
| `-config:limitFps:b=<...>` | true | Toggles the software sleep-based frame limiter in the main loop that paces draws to the act-rate. When it's false the engine renders as fast as it can. |
| `-config:video/driver:t=<...>` | "auto" | Selects the 3D driver to use (e.g. `stub`, `auto`, `dx11`, `dx12`, `vulkan`). Same effect as `-drv:`. |

## Usage Example

```
daEditor3x-dev.exe
daEditor3x-dev.exe path-to-application.blk
daEditor3x-dev.exe path-to-application.blk path-to-level.blk
daEditor3x-dev.exe path-to-application.blk -quiet -config:debug/profiler:t=all path-to-script.dcmd -async_batch
```
