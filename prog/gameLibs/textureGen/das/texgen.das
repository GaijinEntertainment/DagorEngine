options gen2

require texgen
require math

// An out pin of a node; id 0 is unconnected. A muted handle connects as a muted edge.
struct Tex {
    id : int
    pin : int
    muted : bool
}

// A scalar out pin. A pin that takes a texture or a scalar is a Tex parameter, one that takes only a texture is
// `Tex explicit`, so a Val goes only where a scalar may.
struct Val : Tex {
}

// A size rule: parent(), graph_size(), px(n), parent() * n, parent() / n, as_width() (height and depth only).
struct Size {
    kind : int
    value : int
}

def texgraph(size : int = 1024; height : int = 0; fmt : string = "R16F"; wrap : bool = true; render_dir : string = ""; entity_dir : string = ""; depth : int = 0) {
    texgen_graph_options(size, height != 0 ? height : size, depth != 0 ? depth : size, fmt, wrap, render_dir, entity_dir)
}

// min and cell left out stay unset
def heightmap(scale : float; min : float = 3.4028235e38; cell : float = 3.4028235e38) {
    texgen_heightmap(scale, min, cell)
}

def muted(t : Tex) : Tex => Tex(id = t.id, pin = t.pin, muted = true)
def muted(v : Val) : Val => Val(id = v.id, pin = v.pin, muted = true)

def parent() : Size => Size(kind = TEXGEN_SIZE_PARENT)
def graph_size() : Size => Size(kind = TEXGEN_SIZE_GRAPH)
def px(n : int) : Size => Size(kind = TEXGEN_SIZE_EXPLICIT, value = n)
def as_width() : Size => Size(kind = TEXGEN_SIZE_WIDTH)

def operator *(s : Size; n : int) : Size {
    if (s.kind == TEXGEN_SIZE_EXPLICIT) {
        return px(s.value * n)
    }
    if (s.kind != TEXGEN_SIZE_PARENT) {
        texgen_error("only parent() and px(n) scale")
    }
    return Size(kind = TEXGEN_SIZE_PARENT_MUL, value = n)
}

def operator /(s : Size; n : int) : Size {
    if (n <= 0) {
        texgen_error("a size divides by a count above zero")
    }
    if (s.kind == TEXGEN_SIZE_EXPLICIT) {
        return px(s.value / n)
    }
    if (s.kind != TEXGEN_SIZE_PARENT) {
        texgen_error("only parent() and px(n) scale")
    }
    return Size(kind = TEXGEN_SIZE_PARENT_DIV, value = n)
}

def size(t : Tex; w : Size) : Tex {
    texgen_size(t.id, 0, w.kind, w.value)
    return t
}

def size(t : Tex; w, h : Size) : Tex {
    texgen_size(t.id, 0, w.kind, w.value)
    texgen_size(t.id, 1, h.kind, h.value)
    return t
}

def depth(t : Tex; d : Size) : Tex {
    texgen_size(t.id, 2, d.kind, d.value)
    return t
}

def fmt(t : Tex; f : string) : Tex {
    texgen_fmt(t.id, TEXGEN_RULE_EXPLICIT, f)
    return t
}

def fmt_graph(t : Tex) : Tex {
    texgen_fmt(t.id, TEXGEN_RULE_GRAPH, "")
    return t
}

def wrap(t : Tex; w : bool) : Tex {
    texgen_wrap(t.id, TEXGEN_RULE_EXPLICIT, w)
    return t
}

def wrap_graph(t : Tex) : Tex {
    texgen_wrap(t.id, TEXGEN_RULE_GRAPH, true)
    return t
}

// The texture rules of a node call: fmt "" is the parent format, "graph" the graph format; wrap_mode "", "graph",
// "wrap" or "clamp".
def rules(t : Tex; width, height, depth : Size; fmt, wrap_mode : string) : Tex {
    texgen_size(t.id, 0, width.kind, width.value)
    texgen_size(t.id, 1, height.kind, height.value)
    texgen_size(t.id, 2, depth.kind, depth.value)
    if (fmt == "") {
        texgen_fmt(t.id, TEXGEN_RULE_PARENT, "")
    } elif (fmt == "graph") {
        texgen_fmt(t.id, TEXGEN_RULE_GRAPH, "")
    } else {
        texgen_fmt(t.id, TEXGEN_RULE_EXPLICIT, fmt)
    }
    if (wrap_mode == "") {
        texgen_wrap(t.id, TEXGEN_RULE_PARENT, true)
    } elif (wrap_mode == "graph") {
        texgen_wrap(t.id, TEXGEN_RULE_GRAPH, true)
    } elif (wrap_mode == "wrap" || wrap_mode == "clamp") {
        texgen_wrap(t.id, TEXGEN_RULE_EXPLICIT, wrap_mode == "wrap")
    } else {
        texgen_error("wrap_mode '{wrap_mode}' is not \"\", \"graph\", \"wrap\" or \"clamp\"")
    }
    return t
}

// A node of any template by its uid, out pin 0; name is the IR label, inputs sizes the in pin list the IR keeps for its template.
def node(uid : string; name : string = ""; inputs : int = 0) : Tex => Tex(id = texgen_node(uid, name, inputs))
def out(t : Tex; pin : int) : Tex => Tex(id = t.id, pin = pin, muted = t.muted)
def val(t : Tex; pin : int) : Val => Val(id = t.id, pin = pin, muted = t.muted)

def param(t : Tex; name : string; v : int) : Tex {
    texgen_param_int(t.id, name, v)
    return t
}

def param(t : Tex; name : string; v : float) : Tex {
    texgen_param_float(t.id, name, v)
    return t
}

def param(t : Tex; name : string; v : bool) : Tex {
    texgen_param_bool(t.id, name, v)
    return t
}

def param(t : Tex; name : string; v : float2) : Tex {
    texgen_param_floats(t.id, name, 2, float4(v.x, v.y, 0.0, 0.0))
    return t
}

def param(t : Tex; name : string; v : float3) : Tex {
    texgen_param_floats(t.id, name, 3, float4(v.x, v.y, v.z, 0.0))
    return t
}

def param(t : Tex; name : string; v : float4) : Tex {
    texgen_param_floats(t.id, name, 4, v)
    return t
}

def param(t : Tex; name : string; v : string) : Tex {
    texgen_param_string(t.id, name, v)
    return t
}

// In pin `pin` counts the template's in pins in descriptor order; an unconnected src leaves it open.
def input(t : Tex; pin : int; src : Tex) : Tex {
    if (src.id != 0) {
        texgen_connect(t.id, pin, src.id, src.pin, src.muted)
    }
    return t
}

// Both return the final, for param().
def final_output(uid, name : string; t : Tex) : Tex => Tex(id = texgen_final(uid, name, t.id, t.pin, t.muted))

def final_export(uid, file, fmt : string; skip : bool; t : Tex) : Tex => Tex(id = texgen_export_(uid, file, fmt, skip, t.id, t.pin, t.muted))
