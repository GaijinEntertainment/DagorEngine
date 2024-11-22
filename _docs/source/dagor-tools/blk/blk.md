# .blk File Format

The `.blk` ("block") is a *Data Block*, the primary configuration format in
*Dagor Engine*. It is a configuration file similar to `.ini`, `.cfg`, and other
configuration files you may have encountered in other systems.

The *Dagor Engine* uses `.blk` files for storing settings, parameters, and other
essential information.

There are two types of `.blk` files: *text* and *binary*.

## Text Format of .blk File

Text `.blk` files are regular text files with proprietary syntax.

### File Syntax

The `.blk` file consists of a *block name* and optional *parameters* enclosed in
curly braces.

```text
<block_name>{
  <parameter_name>:<type>=<value>[;]
}
```

where

- `<block name>`: The name of the block, which must start with a Latin letter or
an underscore and consist of Latin letters, digits, or underscores.
- `<parameter_name>`: The name of the parameter, which must start with a Latin
  letter or an underscore and consist of Latin letters, digits, or underscores.
- `<type>`: The type of the parameter, which is mandatory and can be one of the
  following:
    - **`t`**: *string*, the `<value>` is a text string in quotes. If the string
      consists of only one word, quotes may be omitted. The string should not
      contain `LF`, `CR`, or `TAB` symbols. Instead, use combinations of
      symbols: `~r` (`CR` symbol), `~n` (`LF` symbol), `~t` (`TAB` symbol).
    - **`b`**: *boolean*, the `<value>` is one of the strings: `yes`, `no`,
      `true`, `false`, `on`, `off`, `1`, `0`.
    - **`c`**: 32-bit *color (E3DCOLOR)*, the `<value>` is a sequence of
      comma-separated integer numbers representing the color the format `R,G,B`
      or in `R, G, B, A` with components ranging from `0` to `255`.
    - **`r`**: *floating-point number*, the `<value>` is a floating-point number
      with truncated trailing zeros.
    - **`m`**: `matrix 3x4`, the `<value>` is a string in the form of `[[first
      line][second line][third line][fourth line]]`. The lines are
      comma-separated floating-point numbers defining x,y,z vector (**`p3`** –
      see below).
    - **`p2`**: *Point2 vector*, the `<value>` is comma-separated floating-point
      numbers defining x,y vector.
    - **`p3`**: *Point3 vector*, the `<value>` is comma-separated floating-point
      numbers defining x,y,z vector.
    - **`p4`**: *Point4 vector*, the `<value>` is comma-separated floating-point
      numbers defining x,y,z,w vector.
    - **`ip2`**: *IPoint2 vector*, the `<value>` is comma-separated integer
      numbers defining x,y vector.
    - **`ip3`**: *IPoint3 vector*, the `<value>` is comma-separated integer
      numbers defining x,y,z vector.
- `<value>`: The value of the parameter, can be in quotes.
- `[;]`: If a semicolon is placed after the parameter, the next parameter may be
  on the same line. If the semicolon is omitted, the next parameter should be on
  a new line.

  ```{note}
  Use a tilde (`~`) in quotes to represent special characters such as
  quotes. To write a tilde in a string, use two tildes (`~~`).
  ```

Parameters can be defined as an array:

```text
<Parameter name>:<type>[] = [<values>]
```

where:

- `<type>`: The type of parameter,
- `<values>`: A list of values of the corresponding type separated by a
  semicolon or a new line.

**Example:**

```text
param1:i[]=[42; 43; 44;]
param2:t[]=[
  "parameter1"
  "parameter2"
  "parameter3"
]
```

### Directives and Comments

`.blk` files may contain *parser directives* and *comments*.

#### Directive @include

All parameters of the file will be included as if they were initially presented
in the current file. Included files may contain other `include` files.

Including another `.blk` file:

```text
include <file_name>
```

where `file name` is a path to the included `.blk` file relative to the current
`.blk` file path. If the `file_name` starts with a slash (`/`), the path is
considered from the root of the engine, otherwise from the directory where the
`.blk` file is located. In tools, starting with `#` means from the `develop`
directory.

#### Directive @override

```text
"@override:<name of block or parameter>"
"@override:block"{
  "@override:parameter"i=10`
}
```
There are rules to follow when *overriding*:

- Overriding a value that has not been declared before results in a log error.
- If it's unclear whether a value to be overridden has been declared before,
  it's common practice to delete it first. This ensures there is no such
  variable from that point, allowing you to add the variable normally.

**Example:**

```text
@override:video{
  @delete:vsync:b=no
  vsync:b=yes
}
```

The code above ensures that `vsync` is deleted first. If there was a `vsync`
before, it will be deleted; if not, then no action is taken. After that, `vsync`
does not need to be overridden and can be added normally, regardless of the
presence of `vsync` before it.

#### Directive @delete

```text
"@delete:<name of block or parameter>"
```

#### Directive @clone-last

```text
"@clone-last:<name of block or parameter>"
```

#### Comments

- `//` – single-line comment
- `/* ... */` – multi-line comment

### Block Types

Blocks can be of the following types: *sequential*, *nested* and combination of
*sequential* and *nested*.

#### Sequential

```text
block_name{
  param_name:type=value
}
block_name{
  param_name:type="value"
}
```

**Example:**

```text
lod{
  range:r=70;
}
lod{
  range:r=10000; fname:t="billboard_octagon_impostor.lod01.dag";
}
```

#### Nested

```text
block_name{
  param_name:type=value
  block_name{
    param_name:type="value"
  }
}
```

**Example:**

```text
contents{
  lod{
    range:r=70;
  }
}
```

#### Combination of sequential and nested

```text
block_name{
  param_name:type=value
  block_name{
    param_name:type="value"
  }
  block_name{
    param_name:type=value
  }
}
```

**Example:**

```text
contents{
  lod{
    range:r=70;
  }
  lod{
    range:r=10000; fname:t="billboard_octagon_impostor.lod01.dag";
  }
}
```

## Binary Format of .blk File

Binary `.blk` files are optimized for performance with a defined structure for
efficient data storage and retrieval.

### File Structure

The data in a binary `.blk` file consists of *header*, *name* and *string maps*,
*parameters*, and *blocks*. The data is ordered as follows:

- **Header**
- **Name map** (parameter and block names)
- **String map** (string parameters)
- **Root parameters:** (may be nested)
  - `paramCnt`
  - `paramNameId_1`
  - `paramType_1`
  - `param_1`
  - `...`
  - `paramNameId_Cnt`
  - `paramType_Cnt`
  - `param_Cnt`
- **Root blocks:** (may be nested)
  - `blockCnt`
  - **Block1 parameters:**
    - `paramCnt`
    - `paramNameId_1`
    - `paramTypeCnt_1`
    - `param_1`
    - `...`
    - `paramNameId_Cnt`
    - `paramType_Cnt`
    - `param_Cnt`
  - **Block1 blocks:**
    - `blockCnt`
    - `Block1_1`
    - `Block1_2`
    - `...`
    - `Block1_Cnt`
      - `...`
  - **Block_Cnt parameters**
  - **Block_Cnt blocks**

## Useful tools

The `dagor3_cdk` has some useful tools for working with `.blk`:

- `binBlk.exe`: Converts binary and text `.blk` files to and from each other.
- `blkDiff.exe`: Performs a syntactic diff between two `.blk` files.
- `blkEditor-dev.exe`: Facilitates the creation of a GUI editor with dynamic
  views for `.blk` files.


