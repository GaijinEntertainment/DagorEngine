options gen2
options no_aot
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/ast_boost
require daslib/ast_cursor
require daslib/rtti
require strings

def fixture_path() : string {
    return "{get_das_root()}/tests/daslib/ast_cursor_fixture.das"
}

def compile_fixture(blk : block<(program : smart_ptr<Program>) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.export_all = true
            cop.no_optimizations = true   // cursor tests assert node positions; folding would erase the nodes
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    to_log(LOG_ERROR, "compile error: {issues}\n")
                    return
                }
                simulate(program) $(sok; _ctx; serrors) {
                    if (!sok) {
                        to_log(LOG_ERROR, "simulate error: {serrors}\n")
                        return
                    }
                    invoke(blk, program)
                }
            }
        }
    }
}

[test]
def test_find_var_at_cursor(t : T?) {
    t |> run("finds ExprVar at variable reference") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprVar(a) at 10:11-10:12
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(!empty(hits), "should find at least one hit")
            t2 |> success(hits[0].rtti == "ExprVar", "first hit should be ExprVar, got {hits[0].rtti}")
            t2 |> success(hits[0].name == "a", "should be named 'a', got '{hits[0].name}'")
        }
    }
}

[test]
def test_find_call_at_cursor(t : T?) {
    t |> run("finds ExprCall at function call") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprCall(to_log) at 19:4-19:10 (line 19 after macro expansion of string builder)
            let hits = find_at_cursor(program, "ast_cursor_fixture", 19, 5)
            var found_call = false
            for (h in hits) {
                if (h.rtti == "ExprCall" && h.name == "to_log") {
                    found_call = true
                    break
                }
            }
            t2 |> success(found_call, "should find ExprCall(to_log)")
        }
    }
}

[test]
def test_innermost_first(t : T?) {
    t |> run("innermost expression comes first") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprOp2(+) at 10:13-10:14 — contains just the '+', inside ExprBlock(9:26-11:1)
            // Hitting col 13 should find ExprOp2 inside ExprBlock
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 13)
            t2 |> success(length(hits) >= 2, "should find multiple hits, got {length(hits)}")
            // first should be the innermost ExprOp2
            t2 |> success(hits[0].rtti == "ExprOp2", "first should be ExprOp2, got {hits[0].rtti}")
            // ExprBlock should be further out
            var found_block = false
            for (i in range(1, length(hits))) {
                if (hits[i].rtti == "ExprBlock") {
                    found_block = true
                    break
                }
            }
            t2 |> success(found_block, "should find ExprBlock after ExprOp2")
        }
    }
}

[test]
def test_enclosing_function(t : T?) {
    t |> run("reports enclosing function") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprVar(a) at 10:11 is inside function 'add'
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(!empty(hits), "should find hits")
            t2 |> success(hits[0].func != null, "should have enclosing function")
            t2 |> success(hits[0].func.name == "add", "should be in function 'add', got '{hits[0].func.name}'")
        }
    }
}

[test]
def test_field_access(t : T?) {
    t |> run("finds ExprField for field access") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprField(x) at 21:24-21:26
            let hits = find_at_cursor(program, "ast_cursor_fixture", 21, 24)
            var found_field = false
            for (h in hits) {
                if (h.rtti == "ExprField" && h.name == "x") {
                    found_field = true
                    break
                }
            }
            t2 |> success(found_field, "should find ExprField(x)")
        }
    }
}

[test]
def test_no_hits_outside(t : T?) {
    t |> run("returns empty for position outside code") @(t2 : T?) {
        compile_fixture() $(program) {
            let hits = find_at_cursor(program, "ast_cursor_fixture", 999, 1)
            t2 |> success(empty(hits), "should find no hits")
        }
    }
}

[test]
def test_describe_hit(t : T?) {
    t |> run("describe produces readable output") @(t2 : T?) {
        compile_fixture() $(program) {
            // ExprVar(a) at 10:11
            let hits = find_at_cursor(program, "ast_cursor_fixture", 10, 11)
            t2 |> success(!empty(hits), "should find hits")
            let desc = describe(hits[0])
            t2 |> success(find(desc, "ExprVar") >= 0, "describe should contain ExprVar")
            t2 |> success(find(desc, "a") >= 0, "describe should contain name")
        }
    }
}

def has_variable_hit(hits : array<CursorHit>; name : string) : bool {
    for (h in hits) {
        return true if (h.variable != null && h.name == name && h.expr == null)
    }
    return false
}

[test]
def test_find_declaration_at_cursor(t : T?) {
    t |> run("a for loop variable in its header is a variable hit when declarations are asked for") @(t2 : T?) {
        compile_fixture() $(program) {
            // for (i in range(2)) at 24:9
            let hits = find_at_cursor(program, "ast_cursor_fixture", 24, 9, true)
            let seen <- [for (h in hits); describe(h)]
            t2 |> success(has_variable_hit(hits, "i"), "for variable i among {seen}")
            t2 |> success(!empty(hits) && hits[0].variable != null && hits[0].name == "i", "the variable hit is innermost: {seen}")
            t2 |> success(!empty(hits) && find(describe(hits[0]), "int") >= 0, "describe carries the variable's type: {seen}")
        }
    }
    t |> run("a function argument and a let variable are variable hits") @(t2 : T?) {
        compile_fixture() $(program) {
            // def add(a, b : int) at 9:8 / 9:11; let p at 20:8
            t2 |> success(has_variable_hit(find_at_cursor(program, "ast_cursor_fixture", 9, 8, true), "a"), "argument a")
            t2 |> success(has_variable_hit(find_at_cursor(program, "ast_cursor_fixture", 9, 11, true), "b"), "argument b")
            t2 |> success(has_variable_hit(find_at_cursor(program, "ast_cursor_fixture", 20, 8, true), "p"), "let variable p")
        }
    }
    t |> run("multi-line spans order by lines: the call, then the loop body block, then main's block") @(t2 : T?) {
        compile_fixture() $(program) {
            // to_log at 25:8 inside the for body block (24:24-26:5) inside main's block (18:11-27:1)
            let hits = find_at_cursor(program, "ast_cursor_fixture", 25, 8)
            var call_idx = -1
            var body_idx = -1
            var main_idx = -1
            for (idx in range(length(hits))) {
                if (hits[idx].rtti == "ExprCall" && hits[idx].name == "to_log") {
                    call_idx = idx
                } elif (hits[idx].rtti == "ExprBlock" && hits[idx].expr != null && int(hits[idx].expr.at.line) == 24) {
                    body_idx = idx
                } elif (hits[idx].rtti == "ExprBlock" && hits[idx].expr != null && int(hits[idx].expr.at.line) == 18) {
                    main_idx = idx
                }
            }
            t2 |> success(call_idx >= 0 && body_idx >= 0 && main_idx >= 0, "the call, the body block and main's block all contain the cursor: {[for (h in hits); describe(h)]}")
            t2 |> success(call_idx < body_idx && body_idx < main_idx, "fewer lines first: {[for (h in hits); describe(h)]}")
        }
    }
    t |> run("without the flag a declaration yields no variable hit") @(t2 : T?) {
        compile_fixture() $(program) {
            t2 |> success(!has_variable_hit(find_at_cursor(program, "ast_cursor_fixture", 24, 9), "i"), "no variable hit by default")
            t2 |> success(!has_variable_hit(find_at_cursor(program, "ast_cursor_fixture", 9, 8), "a"), "no argument hit by default")
        }
    }
}

[test]
def test_synthesized_members(t : T?) {
    t |> run("a cursor on a class name yields no hit from the members the compiler puts on that line") @(t2 : T?) {
        compile_fixture() $(program) {
            // class Cat : Animal at 35:6
            let hits = find_at_cursor(program, "ast_cursor_fixture", 35, 8, true)
            t2 |> success(empty(hits), "no expression hit on a class line: {[for (h in hits); describe(h)]}")
        }
    }
    t |> run("is_synthesized tells the compiler's members from the user's") @(t2 : T?) {
        compile_fixture() $(program) {
            var user = 0
            var synthesized = 0
            ast::for_each_function(get_this_module(program), "") $(var func : FunctionPtr) {
                let f = func
                if (is_synthesized(f)) {
                    synthesized++
                } else {
                    user++
                }
            }
            t2 |> success(synthesized > 0, "a class carries synthesized members: {synthesized}")
            t2 |> equal(user, 17, "add, greet, main, Animal`speak, Cat`speak, no_log, safe_x, use_twice, count_up, kennel, Puppy`bark, Puppy`speak, captured, the twice and each instances, the generator and lambda bodies - got {user}")
            var instances = 0
            var generators = 0
            ast::for_each_function(get_this_module(program), "") $(var func : FunctionPtr) {
                let f = func
                if (f.fromGeneric != null) {
                    instances++
                    t2 |> success(!is_synthesized(f), "a generic instance is the user's: {f.name}")
                }
                if (f.flags._generator) {
                    generators++
                    t2 |> success(!is_synthesized(f), "a generator body is the user's: {f.name}")
                }
            }
            t2 |> success(instances > 0, "the fixture instantiates twice")
            t2 |> success(generators > 0, "the fixture has a generator body")
        }
    }
}

[test]
def test_find_declaration(t : T?) {
    t |> run("a class name, its parent on the derived line, a function name") @(t2 : T?) {
        compile_fixture() $(program) {
            let cat = find_declaration_at_cursor(program, "ast_cursor_fixture", 35, 7, "class Cat : Animal \{")
            t2 |> success(cat.kind == DeclarationKind.structure && cat.name == "Cat", "class name: {cat.name}")
            t2 |> equal(cat.word_column, 6, "the name starts at column 6")
            let parent = find_declaration_at_cursor(program, "ast_cursor_fixture", 35, 14, "class Cat : Animal \{")
            t2 |> success(parent.kind == DeclarationKind.structure && parent.name == "Animal", "parent name resolves the parent: {parent.name}")
            t2 |> equal(int(parent.at.line), 31, "the parent's own position")
            let fn = find_declaration_at_cursor(program, "ast_cursor_fixture", 9, 6, "def add(a, b : int) : int \{")
            t2 |> success(fn.kind == DeclarationKind.func && fn.name == "add", "function name: {fn.name}")
            let any_file = find_declaration_at_cursor(program, "", 9, 6, "def add(a, b : int) : int \{")
            t2 |> success(any_file.kind == DeclarationKind.func && any_file.name == "add", "an empty file matches any file: {any_file.name}")
            let other_file = find_declaration_at_cursor(program, "no_such_fixture", 9, 6, "def add(a, b : int) : int \{")
            t2 |> equal(other_file.kind, DeclarationKind.none, "another file's line 9 is not this declaration")
            let method = find_declaration_at_cursor(program, "ast_cursor_fixture", 36, 20, "    def override speak() : string => \"meow\"")
            t2 |> success(method.kind == DeclarationKind.func && method.name == "speak", "method name is bare: {method.name}")
        }
    }
    t |> run("the cursor must be on the name token; a line-only lookup takes any declaration on the line") @(t2 : T?) {
        compile_fixture() $(program) {
            let off = find_declaration_at_cursor(program, "ast_cursor_fixture", 9, 1, "def add(a, b : int) : int \{")
            t2 |> success(off.kind == DeclarationKind.none, "cursor on `def` is not the declaration")
            let any = find_declaration_at_cursor(program, "ast_cursor_fixture", 9, 1, "")
            t2 |> success(any.kind == DeclarationKind.func && any.name == "add" && any.word_column < 0, "line-only lookup finds add with no column")
            let none = find_declaration_at_cursor(program, "ast_cursor_fixture", 19, 4, "")
            t2 |> success(none.kind == DeclarationKind.none, "a statement line declares nothing")
        }
    }
}

[test]
def test_text_helpers(t : T?) {
    t |> run("word_at, name_column, spells_name, source_name, bare_name") @(t2 : T?) {
        let w = word_at("    let dx = a.x - b.x", 15)
        t2 |> success(w.word == "x" && w.start == 15, "word under the cursor: {w.word} at {w.start}")
        let mid = word_at("class Animal \{", 9)
        t2 |> success(mid.word == "Animal" && mid.start == 6, "cursor mid-word: {mid.word} at {mid.start}")
        let tail = word_at("class Animal \{", 5)
        t2 |> success(tail.word == "class" && tail.start == 0, "the cursor right after a word is on it: {tail.word}")
        let gap = word_at("class  Animal", 6)
        t2 |> success(empty(gap.word), "cursor between spaces is no word: '{gap.word}'")
        t2 |> equal(name_column("class Cat : Animal \{", "Animal", -1), 12)
        t2 |> equal(name_column("def catalog(cat : Cat)", "cat", -1), 12, "a whole word, not the prefix of catalog")
        t2 |> equal(name_column("nothing here", "Cat", 7), 7, "the hint when the name is not spelled")
        t2 |> success(spells_name("class Cat : Animal \{", 12, "Animal"), "spelled at 12")
        t2 |> success(!spells_name("class Cat : Animal \{", 13, "Animal"), "not starting at 13")
        t2 |> equal(source_name("__i_rename_at_6_13"), "i")
        t2 |> equal(source_name("plain"), "plain")
        t2 |> equal(source_name("__this"), "__this", "a double underscore alone is not the rename wrap")
        t2 |> equal(name_column("Cat is first", "Cat", -1), 0, "a name at the start of the line")
        t2 |> equal(name_column("last is Cat", "Cat", -1), 8, "a name at the end of the line")
        t2 |> equal(bare_name("Animal`speak"), "speak")
        t2 |> equal(bare_name("speak"), "speak")
    }
}

[test]
def test_field_name_hit(t : T?) {
    t |> run("a cursor on the field name of a field access hits the access") @(t2 : T?) {
        compile_fixture() $(program) {
            // to_log(LOG_INFO, "{p.x + p.y}\n") at line 21: p.x with x at column 25
            let hits = find_at_cursor(program, "ast_cursor_fixture", 21, 25)
            var found = false
            for (h in hits) {
                found = found || (h.rtti == "ExprField" && h.name == "x")
            }
            t2 |> success(found, "ExprField(x) among {[for (h in hits); describe(h)]}")
        }
    }
}

def has_type_hit(hits : array<CursorHit>; name : string = "") : bool {
    for (h in hits) {
        return true if (h.type_decl != null && (empty(name) || h.name == name))
    }
    return false
}

[test]
def test_type_name_hit(t : T?) {
    t |> run("a cursor on a type name yields the type; on a forward-referenced class's new site too") @(t2 : T?) {
        compile_fixture() $(program) {
            // var pet = new Cat() at line 27, Cat at column 18
            let hits = find_at_cursor(program, "ast_cursor_fixture", 27, 19, true, "    var pet = new Cat()")
            t2 |> success(!empty(hits) && hits[0].type_decl != null && hits[0].name == "Cat", "type hit first: {[for (h in hits); describe(h)]}")
            t2 |> success(!empty(hits) && hits[0].type_decl != null && int(hits[0].type_decl.at.column) == 18, "at the name, not the new keyword: {[for (h in hits); describe(h)]}")
            t2 |> equal(length([for (h in hits); h; where h.type_decl != null]), 1, "one type hit for the several nodes that carry T: {[for (h in hits); describe(h)]}")
            t2 |> success(describe(hits[0]) |> starts_with("TypeDecl(Cat) : Cat at 27:18"), "describe names the type: {describe(hits[0])}")
            let on_var = find_at_cursor(program, "ast_cursor_fixture", 27, 8, true, "    var pet = new Cat()")
            t2 |> success(!has_type_hit(on_var), "the cursor on the variable, not the type name: {[for (h in on_var); describe(h)]}")
            let prim = find_at_cursor(program, "ast_cursor_fixture", 9, 16, true, "def add(a, b : int) : int \{")
            t2 |> success(!has_type_hit(prim), "no type hit on a builtin type: {[for (h in prim); describe(h)]}")
            let meth = find_at_cursor(program, "ast_cursor_fixture", 36, 20, true, "    def override speak() : string => \"meow\"")
            t2 |> success(!has_type_hit(meth, "Cat"), "the implicit self is not a type hit: {[for (h in meth); describe(h)]}")
            let cls = find_at_cursor(program, "ast_cursor_fixture", 35, 7, true, "class Cat : Animal \{")
            t2 |> success(!has_type_hit(cls, "Cat"), "the synthesized members' self on the class line is not a type hit: {[for (h in cls); describe(h)]}")
        }
    }
    t |> run("without the line text, without declarations or in another file there is no type hit") @(t2 : T?) {
        compile_fixture() $(program) {
            let hits = find_at_cursor(program, "ast_cursor_fixture", 27, 19, true)
            t2 |> success(!has_type_hit(hits), "no line text, no type hit")
            let no_decl = find_at_cursor(program, "ast_cursor_fixture", 27, 19, false, "    var pet = new Cat()")
            t2 |> success(!has_type_hit(no_decl), "declarations off, no type hit")
            let other = find_at_cursor(program, "no_such_fixture", 27, 19, true, "    var pet = new Cat()")
            t2 |> success(empty(other), "another file, no hit at all: {[for (h in other); describe(h)]}")
        }
    }
    t |> run("a user field's initializer after the compiler's fields, and a safe field access from its name") @(t2 : T?) {
        compile_fixture() $(program) {
            // class Dog : Animal { age : int = 3 } at lines 43-45; the 3 at 44:16
            let init = find_at_cursor(program, "ast_cursor_fixture", 44, 16)
            t2 |> success(!empty(init) && init[0].rtti == "ExprConstInt", "the initializer is a hit: {[for (h in init); describe(h)]}")
            // return p?.x ?? 0.0 at line 48, x at column 14
            let safe = find_at_cursor(program, "ast_cursor_fixture", 48, 14)
            t2 |> success(!empty(safe) && safe[0].rtti == "ExprSafeField" && safe[0].name == "x", "the safe field access from its name: {[for (h in safe); describe(h)]}")
        }
    }
    t |> run("a type's declaration is its class, with the name column spelled on the class line") @(t2 : T?) {
        compile_fixture() $(program) {
            let hits = find_at_cursor(program, "ast_cursor_fixture", 27, 19, true, "    var pet = new Cat()")
            let cat = declaration_of(program, hits[0].type_decl)
            t2 |> success(cat.kind == DeclarationKind.structure && cat.name == "Cat" && int(cat.at.line) == 35, "the class: {cat.name} at {int(cat.at.line)}")
            t2 |> equal(declared_name_column(cat.at, cat.name), 6, "the name, not the class keyword")
            t2 |> equal(declaration_of(program, null).kind, DeclarationKind.none)
        }
    }
    t |> run("a typedef another module declares resolves to that module's declaration") @(t2 : T?) {
        compile_fixture() $(program) {
            // def no_log() : file { at line 39, file at column 15; fio declares `typedef file = FILE const?` on its line 12
            let hits = find_at_cursor(program, "ast_cursor_fixture", 39, 16, true, "def no_log() : file \{")
            t2 |> success(has_type_hit(hits, "file"), "the alias is a type hit: {[for (h in hits); describe(h)]}")
            let alias = declaration_of(program, hits[0].type_decl)
            t2 |> success(alias.kind == DeclarationKind.alias && alias.alias_type != null, "resolved: {alias.name} kind {alias.kind}")
            t2 |> success(int(alias.at.line) == 12 && alias.at.fileInfo != null && find(string(alias.at.fileInfo.name), "fio") >= 0, "declared in fio at {int(alias.at.line)}")
            t2 |> success(typedef_named(program, "no_such_alias") == null, "an unknown alias is null")
        }
    }
}

[test]
def test_class_members(t : T?) {
    t |> run("a method call is a hit on the method's name, never on the conversions the call desugars to") @(t2 : T?) {
        compile_fixture() $(program) {
            // let heard = d->speak() at line 65, speak at column 19: the node spans the `->`, the name has its own position
            let arrow = find_at_cursor(program, "ast_cursor_fixture", 65, 21)
            t2 |> success(!empty(arrow) && arrow[0].rtti == "ExprField" && arrow[0].name == "speak", "the method field first: {[for (h in arrow); describe(h)]}")
            // def override speak() : string => bark() at line 71, the implicit-self call at column 37
            let self_call = find_at_cursor(program, "ast_cursor_fixture", 71, 38)
            t2 |> success(!empty(self_call) && self_call[0].rtti == "ExprField" && self_call[0].name == "bark", "the implicit self call names its method: {[for (h in self_call); describe(h)]}")
            for (h in self_call) {
                t2 |> success(h.rtti != "ExprRef2Value", "the compiler's ref-to-value conversion is never a hit")
            }
            t2 |> success(length(self_call) > 1 && find(describe(self_call[1]), "Expr") >= 0, "the call's conversion nodes follow the named field on the same span: {[for (h in self_call); describe(h)]}")
            // return d.age + length(heard) at line 66, age at column 13
            let data = find_at_cursor(program, "ast_cursor_fixture", 66, 14)
            t2 |> success(!empty(data) && data[0].rtti == "ExprField" && data[0].name == "age", "a data field from its name: {[for (h in data); describe(h)]}")
        }
    }
    t |> run("method_function follows a method field to the override the receiver's class declares, else the inherited one") @(t2 : T?) {
        compile_fixture() $(program) {
            let dog = module_find_structure(get_this_module(program), "Dog")
            let puppy = module_find_structure(get_this_module(program), "Puppy")
            let point = module_find_structure(get_this_module(program), "Point")
            t2 |> success(dog != null && puppy != null && point != null, "the fixture declares Dog, Puppy and Point")
            let inherited = find_method_function(dog, "speak")
            t2 |> success(inherited != null && inherited.name == "Animal`speak", "Dog inherits speak: {inherited != null ? string(inherited.name) : "null"}")
            let overridden = find_method_function(puppy, "speak")
            t2 |> success(overridden != null && overridden.name == "Puppy`speak", "Puppy overrides speak: {overridden != null ? string(overridden.name) : "null"}")
            t2 |> success(find_method_function(puppy, "age") == null, "a data field is no method")
            t2 |> success(find_method_function(point, "x") == null && find_method_function(point, "nope") == null && find_method_function(null, "x") == null, "a struct field, an unknown name and no struct are null")
        }
    }
    t |> run("a field is declared once: the class body's own line, shared by every class that inherits it") @(t2 : T?) {
        compile_fixture() $(program) {
            let dog = module_find_structure(get_this_module(program), "Dog")
            let puppy = module_find_structure(get_this_module(program), "Puppy")
            // age : int = 3 at line 44, column 4
            let decl = find_declaration_at_cursor(program, "ast_cursor_fixture", 44, 5, "    age : int = 3")
            t2 |> success(decl.kind == DeclarationKind.field && decl.name == "age" && decl.structure != null && decl.structure.name == "Dog", "the field declaration: {decl.name} of {decl.structure != null ? string(decl.structure.name) : "-"}")
            t2 |> success(decl.field_type != null && describe(unsafe(reinterpret<TypeDecl?>(decl.field_type))) == "int", "with its type")
            t2 |> equal(int(field_at(puppy, "age").line), 44, "the inherited copy keeps the parent's position")
            t2 |> equal(int(field_at(dog, "nope").line), 0, "no such field is a zero position")
            let own <- [for (fld in puppy.fields); string(fld.name); where is_own_field(fld)]
            t2 |> success(length(own) == 2 && own[0] == "speak" && own[1] == "bark", "Puppy's own fields are the override it declares and its new method; age is the parent's copy, __rtti and __finalize the compiler's: {own}")
            let method_line = find_declaration_at_cursor(program, "ast_cursor_fixture", 70, 9, "    def bark() : string => \"yip\"")
            t2 |> success(method_line.kind == DeclarationKind.func && method_line.name == "bark", "a method's def line is the function, not a field: {method_line.kind}")
            let off_name = find_declaration_at_cursor(program, "ast_cursor_fixture", 44, 10, "    age : int = 3")
            t2 |> equal(off_name.kind, DeclarationKind.none, "the cursor on the field's type is not its declaration")
            t2 |> equal(int(field_at(null, "age").line), 0, "no struct is a zero position")
        }
    }
    t |> run("a captured let is a variable, not a field of the lambda's capture struct; a function-pointer field is no method") @(t2 : T?) {
        compile_fixture() $(program) {
            // let base = 10 at line 79, captured by the lambda on line 80
            let decl = find_declaration_at_cursor(program, "ast_cursor_fixture", 79, 9, "    let base = 10")
            t2 |> equal(decl.kind, DeclarationKind.none, "the capture struct's field is not a declaration the source spells")
            let hits = find_at_cursor(program, "ast_cursor_fixture", 79, 9, true, "    let base = 10")
            t2 |> success(has_variable_hit(hits, "base"), "the let itself is the answer: {[for (h in hits); describe(h)]}")
            let greeter = module_find_structure(get_this_module(program), "Greeter")
            t2 |> success(greeter != null && find_method_function(greeter, "cb") == null, "a data field initialized with @@greet is not a method")
        }
    }
}
