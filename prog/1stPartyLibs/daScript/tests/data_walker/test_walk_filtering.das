// Tests for DapiDataWalker — canVisit* filtering
//
// Covers: canVisitStructure, canVisitArray, canVisitArrayData,
//         canVisitTableData, canVisitPointer, selective struct walking

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


// ============================================================
// Local walker classes for filtering tests
// ============================================================

// A walker that skips structures entirely
class SkipStructWalker : DapiDataWalker {
    log : string
    def override canVisitStructure(ps : void?; si : StructInfo) : bool {
        log += "canVisitStructure:false\n"
        return false
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
    def override Float(var value : float&) : void {
        log += "Float:{value}\n"
    }
    def override beforeStructure(ps : void?; si : StructInfo) : void {
        log += "beforeStruct\n"
    }
    def override afterStructure(ps : void?; si : StructInfo) : void {
        log += "afterStruct\n"
    }
}

// A walker that skips arrays
class SkipArrayWalker : DapiDataWalker {
    log : string
    def override canVisitArray(pa : void?; ti : TypeInfo) : bool {
        return false
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
    def override beforeArray(pa : DapiArray; ti : TypeInfo) : void {
        log += "beforeArray\n"
    }
}

// A walker that skips array data but still gets before/afterArray
class SkipArrayDataWalker : DapiDataWalker {
    log : string
    def override canVisitArrayData(ti : TypeInfo; count : uint) : bool {
        log += "canVisitArrayData:{int(count)}\n"
        return false
    }
    def override beforeArray(pa : DapiArray; ti : TypeInfo) : void {
        log += "beforeArray\n"
    }
    def override afterArray(pa : DapiArray; ti : TypeInfo) : void {
        log += "afterArray\n"
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
}

// A walker that skips table data
class SkipTableDataWalker : DapiDataWalker {
    log : string
    def override canVisitTableData(ti : TypeInfo) : bool {
        return false
    }
    def override beforeTable(pa : DapiTable; ti : TypeInfo) : void {
        log += "beforeTable\n"
    }
    def override afterTable(pa : DapiTable; ti : TypeInfo) : void {
        log += "afterTable\n"
    }
    def override String(var value : string&) : void {
        log += "String:{value}\n"
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
}

// A walker that skips pointer dereference
class SkipPointerWalker : DapiDataWalker {
    log : string
    def override canVisitPointer(ti : TypeInfo) : bool {
        return false
    }
    def override beforePtr(ps : void?; ti : TypeInfo) : void {
        log += "beforePtr\n"
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
    def override beforeStructure(ps : void?; si : StructInfo) : void {
        log += "beforeStruct\n"
    }
}

// A walker that selectively skips a named struct
class SelectiveStructWalker : DapiDataWalker {
    log : string
    skip_name : string
    def override canVisitStructure(ps : void?; si : StructInfo) : bool {
        if (si.name == skip_name) {
            return false
        }
        return true
    }
    def override beforeStructure(ps : void?; si : StructInfo) : void {
        log += "visit:{si.name}\n"
    }
    def override Int(var value : int&) : void {
        log += "Int:{value}\n"
    }
    def override Float(var value : float&) : void {
        log += "Float:{value}\n"
    }
    def override String(var value : string&) : void {
        log += "String:{value}\n"
    }
}


// ============================================================
// Tests
// ============================================================

[test]
def test_can_visit_filtering(t : T?) {
    t |> run("skip structures") @(t : T?) {
        var walker = new SkipStructWalker()
        var s = SimpleStruct(x = 10, y = 2.5)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        // Should see canVisitStructure but NOT beforeStruct/afterStruct or field values
        t |> equal(find(walker.log, "canVisitStructure:false") != -1, true)
        t |> equal(find(walker.log, "beforeStruct") == -1, true)
        t |> equal(find(walker.log, "Int:") == -1, true)
        t |> equal(find(walker.log, "Float:") == -1, true)
        unsafe { delete walker; }
    }
    t |> run("skip arrays") @(t : T?) {
        var walker = new SkipArrayWalker()
        var arr = [1, 2, 3]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        // Should NOT see any Int callbacks or beforeArray
        t |> equal(walker.log, "")
        unsafe { delete walker; }
    }
}


[test]
def test_can_visit_array_data(t : T?) {
    t |> run("skip array data but still get before/after array") @(t : T?) {
        var walker = new SkipArrayDataWalker()
        var arr = [1, 2, 3]
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
        }
        // Should see beforeArray/afterArray but no element callbacks
        t |> equal(find(walker.log, "beforeArray") != -1, true)
        t |> equal(find(walker.log, "afterArray") != -1, true)
        t |> equal(find(walker.log, "canVisitArrayData:3") != -1, true)
        t |> equal(find(walker.log, "Int:") == -1, true)
        unsafe { delete walker; }
    }
}


[test]
def test_can_visit_table_data(t : T?) {
    t |> run("skip table data entries") @(t : T?) {
        var walker = new SkipTableDataWalker()
        var tab : table<string; int>
        tab |> insert("a", 1)
        tab |> insert("b", 2)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(tab), typeinfo rtti_typeinfo(tab))
        }
        t |> equal(find(walker.log, "beforeTable") != -1, true)
        t |> equal(find(walker.log, "afterTable") != -1, true)
        // No keys or values should be walked
        t |> equal(find(walker.log, "String:") == -1, true)
        t |> equal(find(walker.log, "Int:") == -1, true)
        unsafe { delete walker; }
    }
}


[test]
def test_can_visit_pointer(t : T?) {
    t |> run("skip pointer dereference") @(t : T?) {
        var walker = new SkipPointerWalker()
        var s = SimpleStruct(x = 99, y = 1.0)
        var p : SimpleStruct? = unsafe(addr(s))
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(p), typeinfo rtti_typeinfo(p))
        }
        // Should NOT walk through the pointer
        t |> equal(walker.log, "")
        unsafe { delete walker; }
    }
}


[test]
def test_selective_struct_visit(t : T?) {
    t |> run("skip inner struct only") @(t : T?) {
        var walker = new SelectiveStructWalker()
        walker.skip_name = "SimpleStruct"
        var s = NestedStruct(inner = SimpleStruct(x = 1, y = 2.0), tag = "outer")
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
        }
        // Should visit NestedStruct but skip SimpleStruct
        t |> equal(find(walker.log, "visit:NestedStruct") != -1, true)
        t |> equal(find(walker.log, "visit:SimpleStruct") == -1, true)
        // tag field (string) should still be walked since it belongs to NestedStruct
        t |> equal(find(walker.log, "String:outer") != -1, true)
        // inner struct's fields should NOT appear
        t |> equal(find(walker.log, "Int:1") == -1, true)
        t |> equal(find(walker.log, "Float:2") == -1, true)
        unsafe { delete walker; }
    }
}
