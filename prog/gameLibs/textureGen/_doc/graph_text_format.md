# texgen::Graph: two front-ends, one light-weight graph, one executor (design)

Pattern: the daCSG das module (Gerrit change 812086, `prog/gameLibs/daCSG/das`).
A program builds a light-weight graph and executes nothing; the executor
runs the graph. For textures the same split gives:

```
 das script  --(texgen das module)-->  texgen::Graph  --(lower)-->  command BLK --> TextureGenerator
 editor BLK  --(GraphData compile)-->  texgen::Graph
                                       texgen::Graph  --(export)-->  das script (straight line)
                                       texgen::Graph  --(export)-->  editor BLK (+ auto layout)
```

Two front-ends write the IR, one back-end executes it, and the two exporters
convert between the front-end formats through the IR. The script carries no
layout; the BLK exporter lays the graph out.

## 1. The IR: `texgen::Graph`

A plain data structure in `prog/gameLibs/textureGen/graph/` with no tools
and no das dependency (same rule as `gameLibs/daCSG.lib`, which holds the
graph and no kernel, while each kernel set and `daCSG/das.lib` are libs of
their own):

```
struct Graph {
  struct Node   { NodeId id; TemplateId tmpl; Vector<Param> params; Vector<PinRef> inputs; SizeRule size; FmtRule fmt; WrapRule wrap; string label; };
  struct Param  { string name; Value value; };            // typed: int, float, bool, float2..4, string, gradient, curve, path
  struct PinRef { NodeId node; uint16_t pin; bool muted; };  // an input: which out pin feeds it; node 0 = unconnected
  struct Export { string file; Fmt fmt; bool skipSaving; };  // typed from day one; never a string to parse
  struct Final  { PinRef src; string name; optional<Export> exportSpec; };
  Vector<Node> nodes; Vector<Final> finals; GraphOptions options;   // size, fmt, wrap, renderDir, entityDir, heightmap
  Vector<Subgraph> subgraphs;                                        // boundary names + a nested Graph, instantiated by tmpl
};
```

- `TemplateId` is the descriptor identity (`templateUid` in `base_nodes.blk`,
  the builtin kind for `const_*`, `output`, math nodes). The IR does not
  carry descriptors; it refers to them.
- Node ids are stable handles given out by the builder, as `csg_graph()`
  gives out ids: a front-end never sees indices.
- Everything the editor needs to show a node again is here (template,
  params, edges, finals, options, subgraph instances); everything that is
  canvas state (x, y, blocks, pin comments) is not. `label` keeps the
  user's node name for error texts and the raw `node()` call on export.
- The IR is the unit the dry-run tests build fixtures in
  (`unittests/textureGen` can construct it directly or run a script).

### 1.1 As built (step 1, 2026-10-25)

`prog/gameLibs/textureGen/graph/texgenGraph_api.h` and the conversions in
`prog/tools/libTools/graphEditor/graph_ir.cpp` follow the sketch above
with these choices, taken where the editor model decided:

- An IR node id is the GraphData node id + 1: GraphData ids start at 0
  and `NodeId` 0 means "none".
- A `Final` is a template node with one in pin carrying `data{final:b=yes}`
  and no out pins ("result", "export texture"); it keeps its own id,
  template and leftover params because it is a real node. A node
  whose out pin is final ("export entities") stays a plain `Node`.
- `Export` is read when the template declares "file format" and the node
  stores "channels" ("texture name" + "file format" give the file,
  "channels" the format text, "skip when exporting" the flag); only the
  compile builds the `export: ...` string.
- `PinRef.pin` counts the template's out pins and `Node.inputs[k]` its in
  pins, each in descriptor order (separators skipped); a saved node's own
  pin order does not matter.
- `SizeRule` has `Explicit`, `Parent`, `Graph`, `ParentMul`, `ParentDiv`
  and `Width` (height and depth that copy the node's width); no graph
  multiply or divide item exists in the editor.
- `Value` text is canonical (shortest float text that parses back,
  `true`/`false`, comma joined components); a gradient, curve or path and
  a text that does not parse for its declared type stay a `String`. A
  property not stored on a node has no IR param; back in GraphData it
  gets its descriptor default.
- Not converted: node labels, pin comments, the node `plugin`, per
  instance `Pin::data`, block and comment annotation nodes with their
  edges, subgraph separators, and `Graph::subgraphs` (one GraphData is
  one file; an instance becomes a `Node` with template `subgraph::<stem>`).
- Hard errors include a node with no template, an edge that does not join
  an out pin to an in pin, an in pin fed twice, an unknown texture size
  or wrap item (also the graph's wrap) and a `validate()` failure; an
  explicit format is kept as text, as the compile keeps it. Edges to
  removed nodes are skipped, as the compile skips them.
- `auto_layout`: Kahn order, column = longest path from a source, rows by
  the mean row of the inputs, pitch 260 x 140.

## 2. Front-end A: the `texgen` das module

Layout mirrors `daCSG/das`: `prog/gameLibs/textureGen/das/texgenModule.cpp`
(the bound builder, `texgen_*` functions private to the module),
`texgenModule_api.h` (`texgen::Graph &texgen_graph(); void texgen_graph_reset();`),
`texgen.das` (the DSL the sheet names), `texgen_nodes.das` and `dsl.md`
(the node functions and the sheet, which `texgraph -gen` writes), and a
`jamfile` producing `gameLibs/textureGen/das.lib`.

```das
// the design sketch, not the built DSL: the generated dsl.md holds a script that compiles
options gen2
require texgen

[export] def graph() : Graph {
    var g = texgraph("terrain_masks", 2048, Fmt.R16F)      // options; wrap and dirs have defaults
    let base   = perlin(octaves = 6, persistence = 0.55, scale = 4.0)
    let ridges = abs(base * 2.0 - 1.0)                      // operators are math nodes, literals const nodes
    let e      = erosion(autolevels(ridges), hardness = const(0.3), drops_multiplier = 2)
    let rock   = rocks(e.height, scale = 1.5).mask          // a subgraph instance
    let slope  = curvature(e.height).size(parent / 2)       // size, fmt, wrap as attribute calls
    g |> output("heightmap", e.height)
    g |> export("masks.tif", Fmt.ARGB8, pack(r = rock, g = slope, b = e.flow_x))
    return g
}

[subgraph] def rocks(height : Tex; scale : float = 1.0) : tuple<mask : Tex; normal : Tex> {
    let n = normal_from_height(height, strength = scale)
    return (threshold(n.z, value = 0.7), n)
}
```

The DSL is in two modules:

1. `texgen.das`, hand written, like `csg.das`: the handles `struct Tex { id,
   pin; muted }` and `struct Val : Tex` (a scalar pin handle), `texgraph(...)`,
   `heightmap(...)`, `final_output`, `final_export`, `muted(t)`, the `Size`
   type with `parent() * 2` and `/ 2` operators, `.size()/.fmt()/.wrap()/.rules()`
   attribute calls and the raw `node()` API.
2. `texgen_nodes.das`, generated node functions, one per descriptor of
   `base_nodes.blk` that a script can call (hidden, block, comment,
   `<graph properties>` and final descriptors, and a name whose identifier
   is empty or taken, get none), each over the public `node()` API of
   `texgen.das`, `output`/`export` over the first final templates of each
   kind, and the `Tex` operators that a descriptor's `operator:t` names:
   `def perlin(octaves : int = 2; persistence : float = 0.5; ...; ofs : Val = Val()) : Tex`.
   `texgraph -gen` writes it from the `base_nodes.blk` the editor loads, so
   it does not drift from the descriptors the editor shows. Multi output nodes
   return a tuple whose field names are the out pin names.

Naming rules, as in the earlier draft: descriptor names become identifiers
by `space`/`-` to `_`; a descriptor's `alias:t`, when set, replaces the
name; a name equal to a core call or type (`size`, `fmt`, `node`, `Tex`, ...)
gets a trailing `_`; when a pin and a property share a name the property keeps
it and the pin gets `_in` (a literal sets the property, a handle connects
the pin, the wire wins as in the editor). Named arguments in daScript are a
strict ordered suffix, so the generator orders parameters main input,
properties by descriptor order, scalar pins, and the DSL sheet tells the
author to keep that order; a `[function_macro]` `transform` that reorders
named arguments is the fallback if authors (or LLMs) trip on it.

The das front-end is a generator: loops and helper functions in the script
are invisible in the IR, by design. A host (editor, tool, test) calls
`texgen_graph_reset()`, compiles the script in a context that has only
`$`, `builtin`, `texgen`, `texgen_nodes`, `math` and `strings`, calls
`graph()`, and reads `texgen_graph()`.

### 2.1 As built (step 2, 2026-10-25)

`prog/gameLibs/textureGen/das` (module, `texgen.das`, script host),
`prog/tools/libTools/graphEditor/graph_das_gen.cpp` (generator, sheet,
`graph_to_das`) and `prog/tools/texgraph` (the command line host) follow
section 2 with these choices, taken where daScript decided:

- A private extern is visible in its own module only, so the generated
  `texgen_nodes.das` cannot call `texgen_node`. The externs stay private
  and `texgen.das` has a small public raw API instead: `node(uid, name,
  inputs)`, `.param()`, `.input()`, `.out()`, `.val()`, `.rules()`,
  `final_output()`, `final_export()`. The generated functions call it, and
  the exporter uses it for a node no generated function can carry (a
  `subgraph::` template, a label, an untyped param).
- daScript allows no default on an OR type parameter, so `Val` derives from
  `Tex`: a pin that takes a texture or a scalar is `Tex`, a texture only pin
  is `Tex explicit` (rejects a `Val`), a scalar pin is `Val`. `muted` is a
  third field of the handle.
- `texgraph(...)` takes no name (the IR options have none) but takes
  `depth`; `heightmap` leaves `min` and `cell` unset by default. Generated
  functions pass an empty label; the height default is `as_width()`, as in
  the descriptors; `node()` takes the in pin count so `inputs` has one entry
  per template pin.
- `graph_to_das` writes the rules as named arguments (a node with several
  out pins returns a tuple) and one `rules()` call for a raw node; it
  returns false with errors on a loop or when `validate` fails.
- `builtin.das` from daslib is stringified into the library next to
  `texgen.das` (both `.das.inl` are gitignored) and served by the host, so
  a host needs no das root on disk. The test and the tool take the exe
  hooks `daScript.lib` leaves open from `dagorAdapter/da.cpp`; `das.lib`
  holds none, so an exe with its own das setup is not affected.
- The sandbox is a `FileAccess`: it reads only the given script from disk,
  serves `texgen_nodes.das` and `builtin.das` from text, allows only `$`,
  `builtin`, `math`, `strings`, `texgen`, `texgen_nodes`, and lets the
  script set only the `gen2` and `indenting` options; policies are gen2,
  `no_unsafe` and no auto inline, so an error in a helper names its own
  line. `graph()` takes no arguments and returns no structure.
- `texgraph -nodes <base_nodes.blk> -gen <dir>` writes the sheet as
  `dsl.md`; no `_doc/dsl.md` is committed, as the project
  `base_nodes.blk` is not in this checkout.
- Not built: `const()`, `note()`, `placeholder()`/`bind()`, `Fmt` (a
  format is a string), `[subgraph]` script functions and operators with a
  number operand (only `Tex op Tex`, from a descriptor's `operator`).

## 3. Front-end B: the editor graph

`compile_graph_to_blks` today lowers `GraphData` straight to the texgen
command BLK. It splits in two: `GraphData -> texgen::Graph` (templates,
property values, edges with their muted flag, finals, options, subgraph
instances) and `texgen::Graph -> command BLK` (section 4), which applies
the muted edges and drops the dead paths.
The first half is small because `GraphData` already is the IR plus canvas
state. The second half is the existing compile logic (size propagation,
substitutions, multipass, bypass, cache nodes) moved under
`prog/gameLibs/textureGen/compile/`, so it no longer needs the editor and
the das host gets it too. Until the move is complete the das host can
produce `GraphData` from the IR and call the editor compile.

## 4. Back-end: lowering and the executor

`lower(const texgen::Graph &, const Descriptors &) -> DataBlock` produces
what `TextureGenerator::start` consumes: nodes with shader, params, typed
register sources, sizes. The descriptors (pins, data flags, blk_code
templates, shader bodies) come from `base_nodes.blk` and the shader
library, loaded by the host; the IR only names them. Lowering is where the
fusion of pointwise nodes and the register lifetimes belong, because it
sees the whole graph once.

Decisions (2026-10-25):

- The compile is an explicit library module, `prog/gameLibs/textureGen/compile`
  (`gameLibs/textureGen/compile.lib`), with the descriptor model it needs and
  no editor or das dependency; the editor plugin and the das host both link
  it. This is step 4 below; until it lands the das host hands its IR to the
  editor compile through `ir_to_graph_data`.
- Exports are typed in the IR (`Export {file, fmt, skipSaving}`), in the das
  module (`export(file, fmt, tex, skip)` fills the struct) and in the editor's
  `GraphData -> IR` step. Only the lowering turns an `Export` into the
  `export: <file>; <fmt>` register name and the `skip_saving` param that the
  service parses today, and that string generation is removed when the
  service reads the typed `output { export { } }` block.
  Nothing above the lowering ever builds or parses the string.

## 5. Exporters

- `graph_to_das(IR)`: `options gen2`, `require texgen` and `texgen_nodes`,
  one `[export] def graph()` that starts with `texgraph(...)` and, when set,
  `heightmap(...)`, `let v<IR id>` per node in topological order with the
  first texture input positional and everything else named (the raw
  `node()` form for a node no generated function can carry),
  `output`/`export` (else `let f<id> = final_output/final_export(...)` with
  `param()` lines), `muted`. `g.subgraphs` is left out. Deterministic, so
  two exports diff cleanly.
- `ir_to_graph_data(IR)`: builds `GraphData` and runs an auto layout
  (layer = longest path from sources, order by barycenter of inputs, fixed
  pitch); then the editor's save writes the BLK. Positions of an earlier
  layout can be reused by matching `label`/template when a script is
  re-imported over an open graph.

## 6. The DSL sheet (`dsl.md`)

As in daCSG, the sheet is the whole reference a session needs: how a script
starts, the core calls, the node list by category with signatures in one
line each (`perlin(octaves = 2, persistence = 0.5, scale = 1.0) : Tex`),
the rules (argument order, literal vs handle, sizes relative to the parent,
finals), and one example. It is generated with the module from
`base_nodes.blk`, so a node added to the editor is in the sheet the same
day. With the project registry it is about 14.5 KB for 79 nodes; every
node stays in, as an author needs each one.

## 7. Steps

| Step | Content | Depends on |
|------|---------|------------|
| 1 | `texgen::Graph` IR + `GraphData -> IR` + `IR -> GraphData` with auto layout; unit tests on the round trip | - |
| 2 | das module (`texgenModule.cpp`, core `texgen.das`), generator for node functions and the sheet, host in a command line tool (`das -> IR -> editor BLK`); done, see 2.1 | 1 |
| 3 | Editor: Import script / Export script / Paste script on the canvas; the node sheet to clipboard | 2 |
| 4 | `gameLibs/textureGen/compile` module: the compile (IR -> command BLK) moves out of the plugin; the das tool executes without the editor; the export string is generated only here | 1 |
| 5 | Lowering gains fusion and lifetimes; the IR is the test fixture format of the dry-run suite | 4 |

Reference implementation to copy the binding style from: daCSG change
812086 (`das/csgModule.cpp`, `das/csg.das`, `_doc/dsl.md`, `CLAUDE.md`).
