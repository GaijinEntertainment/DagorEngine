require fio
require daslib/strings_boost
require daslib/regex_boost

struct Node
    name : string
    text : string
    subnodes : array<Node?>

var ast_verbose = false
var ast_process_verbose = false
var ast_line = 0
var ast_depth = 0

def parse_node ( f:FILE const?; header:string; offset:int )
    ast_depth ++
    var node = new [[Node
        text = slice(header, offset)
    ]]
    for ch,sp in node.text,count()
        if ch==' ' || ch=='\n' || ch=='\r'
            node.name = slice(node.text,0,sp)
            break
    if node.name==""
        print("expecting node name at line {ast_line}\n")
        panic("expecting node name")
    if ast_verbose
        print("{repeat("  ",ast_depth)} {ast_line} {node.name}\n")
    while true
        let tpos = ftell(f)
        let tline = ast_line
        let subtext = fgets(f)
        ast_line ++
        if length(subtext)<=offset
            fseek(f, tpos, seek_set)
            ast_line = tline
            break
        let CH = character_at(subtext, offset)
        if CH=='|'   // expecting |- next node
            if character_at(subtext, offset+1)!='-'
                if ast_verbose
                    print("expecting - at line {ast_line}\n")
                panic("expecting -")
            node.subnodes |> push <| parse_node(f, subtext, offset+2)
        elif CH=='`'    // expecting `- next node and done
            if character_at(subtext, offset+1)!='-'
                if ast_verbose
                    print("expecting - at line {ast_line}\n")
                panic("expecting -")
            node.subnodes |> push <| parse_node(f, subtext, offset+2)
            break
        else
            fseek(f, tpos, seek_set)
            ast_line = tline
            break
    ast_depth --
    return node

def parse_ast_file ( fname : string )
    var offset = 0
    var node : Node?
    fopen(fname, "rb") <| $ ( f )
        if f != null
            ast_line = 1
            node = parse_node(f, fgets(f), 0)
    return node

def process_typedef ( node : Node? )
    pass

/*
| | |-CXXMethodDecl 0x17612fe54c0 <line:2181:1, col:150> col:6 vprorvd 'void (const Xbyak::Xmm &, const Xbyak::Xmm &, const Xbyak::Operand &)'
| | | |-ParmVarDecl 0x17612fe5330 <col:14, col:25> col:25 used x1 'const Xbyak::Xmm &'
| | | |-ParmVarDecl 0x17612fe53a8 <col:29, col:40> col:40 used x2 'const Xbyak::Xmm &'
| | | |-ParmVarDecl 0x17612fe5420 <col:44, col:59> col:59 used op 'const Xbyak::Operand &'
*/

var reg_method_decl <- %regex~CXXMethodDecl 0x[0-9a-z]+\s*<line:\d+:\d+,\s*col:\d+>\scol:\d+\s(used\s)?(\w+)\s+'(.*)'%%

def process_method_decl ( node : Node? )
    assert(node.name=="CXXMethodDecl")
    if regex_match(reg_method_decl, node.text) != -1

        let method = regex_group(reg_method_decl, 1, node.text)
        let method_decl = regex_group(reg_method_decl, 3, node.text)
        print("method {method} as {method_decl}\n")
    else
        print("UNMATCHED {node.text}")

def process_ast ( node : Node? )
    ast_depth ++
    if ast_process_verbose
        print("{repeat("  ",ast_depth)} {ast_line} {node.name}\n")
    if node.name=="TypedefDecl"
        process_typedef(node)
    elif node.name=="CXXMethodDecl"
        process_method_decl(node)
    else
        for sn in node.subnodes
            process_ast(sn)
    ast_depth --

[export]
def main
    let xbfile = "{get_das_root()}/modules/dasXbyak/src/xbyak.h.ast"
    var node = parse_ast_file(xbfile)
    print("\n\nAND DONE PARSING\n\n")
    process_ast(node)
    print("\n\nAND DONE PROCESSING\n\n")
