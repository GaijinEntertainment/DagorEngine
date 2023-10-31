



let t = {
    rawget = function(k) {
        return $"overriden rawget({k})"
    }

    xxx = "RealValue"

    keys = function() {
        return [1, 2, 3]
    }

    rawset = function(k, v) {
        println($"overriden rawset({k} -> {v})")
    }

    xyz = "Original"
}


println($"t.rawget(\"xxx\") --> {t.rawget("xxx")}")
println($"call_type_method(t, \"rawget\", \"xxx\") --> {call_type_method(t, "rawget", "xxx")}")


let k = t.keys()

println($"t.keys()")
foreach (idx, v in k) {
    println($"{idx} --> {v}")
}

let b = call_type_method(t, "keys");

println($"call_type_method(t, keys)")
foreach (idx, v in b) {
    println($"{idx} --> {v}")
}

println("rawset(...) regular")
t.rawset("xyz", "ABC")
println($"t.xyz --> {t.xyz}")


println("rawset(...) call_type_method")
call_type_method(t, "rawset", "xyz", "Updated");
println($"t.xyz --> {t.xyz}")

try {
    call_type_method(t, "dummy", "xyz", "Updated");
    println("FAILED to call dummy method")
} catch (ex) {
    println($"OK: invokation of 'dummy()' raised exception \"{ex}\"")
}


try {
    call_type_method_safe(t, "dummy", "xyz", "Updated");
    println($"OK: safe call succeseded")
} catch (ex) {
    println("FAIL: Safe call raised exception")
}
