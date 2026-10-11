options gen2
options indenting = 4
options rtti

module _stackwalk_common shared public

require strings
require daslib/rtti public
require daslib/debugger public

// records every walked frame and local; tests assert on frame names and on which
// locals report in scope. lives in its own module so each test file's own options
// (auto_inline_functions, debugger) govern only the shapes under test.
class CollectingWalker : DapiStackWalker {
    frames : array<string>
    locals : array<string>          // "frame|name|inScope"
    curFrame : string
    @safe_when_uninitialized walkAt : LineInfo  // empty: only the deepest frame's locals gate on it
    def override canWalkArguments : bool => false
    def override canWalkVariables : bool => true
    def override canWalkOutOfScopeVariables : bool => true
    def override onBeforeCall(pp : Prologue; sp : void?) : void {}
    def override onCallAOT(pp : Prologue; fileName : string#) : void {
        curFrame = "<AOT>"
        frames |> push(curFrame)
    }
    def override onCallJIT(pp : Prologue; fileName : string#) : void {
        curFrame = "<JIT>"
        frames |> push(curFrame)
    }
    def override onCallAt(pp : Prologue; info : FuncInfo; at : LineInfo) : void {
        curFrame = "{info.name}"
        frames |> push(curFrame)
    }
    def override onCall(pp : Prologue; info : FuncInfo) : void {
        curFrame = "{info.name}"
        frames |> push(curFrame)
    }
    def override onAfterPrologue(pp : Prologue; sp : void?) : void {}
    def override onArgument(info : FuncInfo; index : int; vinfo : VarInfo; arg : float4) : void {}
    def override onBeforeVariables : void {}
    def override onVariable(inf : FuncInfo; vinfo : LocalVariableInfo; arg : void?; inScope : bool) : void {
        locals |> push("{curFrame}|{vinfo.name}|{inScope}")
    }
    def override onAfterCall(pp : Prologue) : bool => true
    def override onCorruptStack(pp : Prologue) : void {}
}

def run_walk(var w : CollectingWalker?) {
    make_stack_walker(w) $(adapter) {
        adapter |> walk_stack(this_context(), w.walkAt)
    }
}

def walk_len(var w : CollectingWalker?; s : string) : int {
    run_walk(w)
    return length(s)
}

def churn_strings(tag : string) : int {
    var junk <- [for (i in range(200)); "junk/{tag}/segment_{i}/{i * 3}_tail"]
    var total = 0
    for (s in junk) {
        total += length(s)
    }
    delete junk
    return total
}

// substring match - inlined locals carry mangled names like _inl1_l_counter
def has_frame(w : CollectingWalker?; needle : string) : bool {
    for (f in w.frames) {
        return true if (find(f, needle) >= 0)
    }
    return false
}

// same, over the packed "frame|name|inScope" records; the local must report in scope
def has_live_local(w : CollectingWalker?; needle : string) : bool {
    for (l in w.locals) {
        if (find(l, needle) >= 0 && ends_with(l, "|true")) {
            return true
        }
    }
    return false
}
