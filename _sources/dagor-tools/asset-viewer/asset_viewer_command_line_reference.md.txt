# Asset Viewer Command-Line Reference

## Command-Line Arguments

| Argument | Description |
|---|---|
| `<path>.blk` | Path to `application.blk` to load. |
| `-ws:<name>` | Selects a named workspace by name. |
| `-async_batch:<file.dcmd>` | Runs the given `.dcmd` console-batch script asynchronously after startup. (The syntax is different compared to daEditorX.) |
| `-no_src_assets` | Skips scanning source asset folders (uses only cooked/prebuilt data). |
| `-drv:<name>` | Selects the 3D driver to use (e.g. `stub`, `auto`, `dx11`, `dx12`, `vulkan`). |
| `-skip_check` | Skips the startup "assets up-to-date" scan/check. |
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
assetViewer2-dev.exe
assetViewer2-dev.exe path-to-application.blk
assetViewer2-dev.exe path-to-application.blk -quiet -config:debug/profiler:t=all
assetViewer2-dev.exe path-to-application.blk -quiet -async_batch:path-to-script.dcmd
```
