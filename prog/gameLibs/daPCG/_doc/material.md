# material DSL sheet

`require material` gives a `Solid` or a `Node` model its .dag materials for a
game build: a row per id with its name, shader class, textures and script. An
id with no row gets the default of dsl.md (class "simple").

```das
options gen2
require csg
require material

[export] def model() : Solid {
    material(0, "body_paint", "dynamic_simple").tex(0, "bike_body_d").tex(2, "bike_body_n").script("atest=0")
    material(3, "glass", "dynamic_simple_glass").two_sided().look("glass").diffuse(0.5, 0.5, 0.5)
    return box(1.0, 1.0, 1.0) | box(0.5, 0.5, 0.5).at(0.0, 0.6, 0.0).mat("glass")
}
```

| call | notes |
|---|---|
| `material(id, name, shader)` | a row; `shader`: a class of the game's shader dump |
| `m.tex(slot, name)` | the texture asset name of slot 0..15 (`dynamic_simple`: 0 diffuse, 2 normal) |
| `m.script(line)` | a `name=value` line of the class, appended |
| `m.two_sided()` | two-sided faces |
| `m.look(name)` | dagrender's look, one of paint, chrome, matte, glass, metallic; a `look=` line after the script lines, so it wins over a script one; with no look line, by the name |
| `m.diffuse(r, g, b)` | 0..1, default 1; dagrender scales the base color by it |
| `a.mat(name)` | `a.mat(id)` of the row with this name, made before |

## Rules

Each setter returns `m`. Ids (0..65535) and names are unique. A default name
of dsl.md (paint to metallic, `mat<id>`) takes only its own id. Names (printable
ASCII), shaders (`A-Za-z0-9_`) and texture names (no line break) are 1 to 255
bytes. Generated faces tile textures: a primitive's uv chart (0..1 per face) or
a `uv_planar`, `uv_box` or `uv_cyl` projection at world scale; no atlas unwrap
yet. A `mesh()` file keeps its uvs. Console: `-check` prints a line per row.
