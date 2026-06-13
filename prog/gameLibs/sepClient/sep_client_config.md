# SEP Client - Configuration Reference

This document describes how to configure the SEP Client library,
where the configuration is stored, and how to override individual
settings from the command line.

---

## Configuration Sources and Merge Order

The library reads configuration from **two sources** that are merged together:

```
[1] Circuit BLK (network.blk, from CDN)       - primary source
        +
[2] dgs_get_settings()/debug/sep { ... }      - local override (optional)
        =
[merged result]  ->  SepClientInstance
```

The override block is merged on top of the circuit config.
Only keys present in the override block are overwritten; everything else is
taken from the circuit config unchanged. This makes it easy to change a
single setting without replacing the whole config.

> **Log output** at startup shows both input blocks before merging:
> ```
> [sep] from_circuit_blk_config: SEP circuit config block (before overriding):
> sepUsagePermille:i=1000
> ...
> [sep] from_circuit_blk_config: applying config overrides from "dgs_get_settings()/debug/sep" block:
> verboseLogging:b=yes
> ...
> ```

---

## BLK Config Structure

The SEP config lives inside a block named `sep` in the circuit config file
(typically `network.blk`):

```blk
sep {
  sepUsagePermille:i    = 1000       // 0-1000; 0 = disabled, 1000 = always use SEP
  supportCompression:b  = yes        // enable zstd response compression
  verboseLogging:b      = no         // full protocol dump to game log

  servers {
    url:t = "wss://sep-prod-1.example.com/sep"
    url:t = "wss://sep-prod-2.example.com/sep"
  }
}
```

---

## Parameters

| Parameter             | BLK type | Default   | Description |
|-----------------------|----------|-----------|-------------|
| `sepUsagePermille`    | `int`    | `0`       | Probability of enabling SEP for a game session, in permille (0-1000). `0` = always disabled, `1000` = always enabled. A random `[0..999]` value is drawn once at process start and compared against this limit. |
| `supportCompression`  | `bool`   | `yes`     | Enable zstd decompression of server responses. Reduces traffic by up to 20x. Disable only for debugging. |
| `verboseLogging`      | `bool`   | `no`      | Full JSON-RPC protocol dump to game log (every request and response). Use only for debugging. |
| `servers`             | `block`  | *(empty)* | WebSocket server URLs (see below). If this block is empty or absent, SEP is disabled. |

### `servers` block

Contains one or more string parameters (any name, typically `url`),
each holding a WebSocket URL (`ws://` or `wss://`):

```blk
servers {
  url:t = "wss://sep-prod-1.example.com/sep"
  url:t = "wss://sep-prod-2.example.com/sep"
}
```

The client picks server URLs randomly,
removing failed URLs from the candidate list and resetting the list once all URLs have been tried.

---

## Platform-Specific Overrides

Any parameter (and the `servers` block) can be given a platform-specific
value by prefixing its name with the platform string:

```
<platform>_<paramName>
```

The platform-prefixed key takes priority over the unprefixed one if present.

### Platform prefix mapping

| Platform prefix | Platforms                                      |
|-----------------|------------------------------------------------|
| `pc`            | Windows (win32/win64), Linux (linux64), macOS  |
| `pc_gdk`        | Windows GDK builds (takes priority over `pc_`) |
| `iOS`           | iOS                                            |
| `android`       | Android                                        |
| `xboxOne`       | Xbox One                                       |
| `xboxScarlett`  | Xbox Series X/S                                |
| `ps4`           | PlayStation 4                                  |
| `ps5`           | PlayStation 5                                  |
| `nswitch`       | Nintendo Switch                                |
| `tvOS`          | tvOS                                           |

### Example: disable SEP on a specific platform

```blk
sep {
  sepUsagePermille:i    = 1000        // enabled everywhere by default

  ps4_sepUsagePermille:i = 0          // disabled on PS4
  nswitch_sepUsagePermille:i = 0      // disabled on Switch

  supportCompression:b  = yes
  android_supportCompression:b = no   // no zstd on Android

  servers {
    url:t = "wss://sep.example.com/sep"
  }

  // Different server list for consoles:
  ps5_servers {
    url:t = "wss://sep-console.example.com/sep"
  }
}
```

---

## Command-Line Overrides

Individual parameters can be overridden at launch without editing any file.
The override goes into the `debug/sep` section of the main game settings:

```
-config:debug/sep/<paramName>:<type>=<value>
```

### Examples

```bash
# Enable SEP for all sessions in this run
-config:debug/sep/sepUsagePermille:i=1000

# Disable SEP completely
-config:debug/sep/sepUsagePermille:i=0

# Enable verbose logging
-config:debug/sep/verboseLogging:b=true

# Disable zstd compression
-config:debug/sep/supportCompression:b=false

# Override server URL (replaces the servers block entirely)
-config:debug/sep/servers/url:t='wss://sep-staging.example.com/sep'
```

> **Note:** Platform-specific prefixes also work in command-line overrides.
> The override block is first merged into the circuit config, and the
> platform prefix lookup runs on the already-merged result. So
> `-config:debug/sep/pc_verboseLogging:b=true` will override only on PC.

### Forcing local `network.blk`

To load `network.blk` from a local file instead of CDN:

```bash
-config:debug/forceLocalNetworkBlk:b=yes
```

---

## When SEP Is Disabled

SEP is **not initialised** (no WebSocket connection is attempted) if **any** of
the following is true:

- `sepUsagePermille` is `0` (or the rolled random value >= limit)
- The `servers` block is empty or missing

Both conditions are checked once at process startup.
If SEP is disabled, the library produces no network traffic and no stats.

---

## Implementation Reference

| File                                                                                      | Description                                          |
|-------------------------------------------------------------------------------------------|------------------------------------------------------|
| [blkConfigReader.cpp](../sepClientInstance/blkConfigReader.cpp)                           | BLK parsing, platform prefix resolution, merge logic |
| [blkConfigReader.h](../publicInclude/sepClientInstance/blkConfigReader.h)                 | `from_circuit_blk_config()` entry point              |
| [sepClientInstanceTypes.h](../publicInclude/sepClientInstance/sepClientInstanceTypes.h)   | `SepClientInstanceConfig` struct                     |
| [globalSepClientInstance.h](../publicInclude/sepClientInstance/globalSepClientInstance.h) | Global instance init (`global_init_once`)            |
