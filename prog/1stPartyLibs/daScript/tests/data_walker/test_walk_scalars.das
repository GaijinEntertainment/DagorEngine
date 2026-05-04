// Tests for DapiDataWalker — scalar types
//
// Covers: int, uint, int64, uint64, int8, uint8, int16, uint16,
//         float, double, bool, string, enum, bitfield

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


[test]
def test_walk_int(t : T?) {
    t |> run("int") @(t : T?) {
        var walker = new LogWalker()
        var x = 42
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int:42\n")
        unsafe { delete walker; }
    }
    t |> run("uint") @(t : T?) {
        var walker = new LogWalker()
        var x = 0xFF_u
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "UInt:0xff\n")
        unsafe { delete walker; }
    }
    t |> run("int64") @(t : T?) {
        var walker = new LogWalker()
        var x = 100_000_000_000l
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int64:100000000000\n")
        unsafe { delete walker; }
    }
    t |> run("uint64") @(t : T?) {
        var walker = new LogWalker()
        var x = 99ul
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "UInt64:0x63\n")
        unsafe { delete walker; }
    }
    t |> run("int8") @(t : T?) {
        var walker = new LogWalker()
        var x = int8(127)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int8:127\n")
        unsafe { delete walker; }
    }
    t |> run("uint8") @(t : T?) {
        var walker = new LogWalker()
        var x = uint8(200)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "UInt8:200\n")
        unsafe { delete walker; }
    }
    t |> run("int16") @(t : T?) {
        var walker = new LogWalker()
        var x = int16(-1000)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Int16:-1000\n")
        unsafe { delete walker; }
    }
    t |> run("uint16") @(t : T?) {
        var walker = new LogWalker()
        var x = uint16(60000)
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "UInt16:60000\n")
        unsafe { delete walker; }
    }
}


[test]
def test_walk_float_double(t : T?) {
    t |> run("float") @(t : T?) {
        var walker = new LogWalker()
        var x = 3.14
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Float:3.14\n")
        unsafe { delete walker; }
    }
    t |> run("double") @(t : T?) {
        var walker = new LogWalker()
        var x = 2.718281828lf
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Double:2.718281828\n")
        unsafe { delete walker; }
    }
}


[test]
def test_walk_bool_string(t : T?) {
    t |> run("bool true") @(t : T?) {
        var walker = new LogWalker()
        var x = true
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Bool:true\n")
        unsafe { delete walker; }
    }
    t |> run("bool false") @(t : T?) {
        var walker = new LogWalker()
        var x = false
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "Bool:false\n")
        unsafe { delete walker; }
    }
    t |> run("string") @(t : T?) {
        var walker = new LogWalker()
        var x = "hello world"
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "String:hello world\n")
        unsafe { delete walker; }
    }
    t |> run("empty string") @(t : T?) {
        var walker = new LogWalker()
        var x = ""
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
        }
        t |> equal(walker.log, "String:\n")
        unsafe { delete walker; }
    }
}


[test]
def test_walk_enum(t : T?) {
    t |> run("enum value") @(t : T?) {
        var walker = new LogWalker()
        var c : Color = Color.Green
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(c), typeinfo rtti_typeinfo(c))
        }
        t |> equal(walker.log, "Enum:Color:1\n")
        unsafe { delete walker; }
    }
    t |> run("first enum value") @(t : T?) {
        var walker = new LogWalker()
        var c : Color = Color.Red
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(c), typeinfo rtti_typeinfo(c))
        }
        t |> equal(walker.log, "Enum:Color:0\n")
        unsafe { delete walker; }
    }
}


[test]
def test_walk_bitfield(t : T?) {
    t |> run("bitfield with flags set") @(t : T?) {
        var walker = new LogWalker()
        var p : Permissions = Permissions.read | Permissions.execute
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(p), typeinfo rtti_typeinfo(p))
        }
        // read=bit0 (1), execute=bit2 (4) → 5 = 0x5
        t |> equal(walker.log, "Bitfield:0x5\n")
        unsafe { delete walker; }
    }
    t |> run("empty bitfield") @(t : T?) {
        var walker = new LogWalker()
        var p : Permissions
        var inscope adapter <- make_data_walker(walker)
        unsafe {
            adapter |> walk_data(addr(p), typeinfo rtti_typeinfo(p))
        }
        t |> equal(walker.log, "Bitfield:0x0\n")
        unsafe { delete walker; }
    }
}
