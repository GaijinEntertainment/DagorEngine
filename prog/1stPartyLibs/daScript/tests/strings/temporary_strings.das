options gen2
require dastest/testing_boost
require daslib/temp_strings
require daslib/lpipe

var payload = "long str"

[test]
def test_temps_strings(t : T?) {

    t |> run("no heap allocations") @@(t) {

        temp_string("long str {payload}\n") $(s) {
            t |> equal(s, "long str long str\n")
        }

        build_temp_string() $(sb) {
            sb |> write("long builder str ")
            sb |> write(payload)
            sb |> write("\n")
        }
        lpipe() $(s) {
            t |> equal(s, "long builder str long str\n")
        }

        t |> equal(string_heap_bytes_allocated(), 0ul)

        var arr <- [ uint8('h'), uint8('e'), uint8('l'), uint8('l'), uint8('o')]
        temp_string(arr) $(s) {
            t |> equal(s, "hello")
        }

        t |> equal(string_heap_bytes_allocated(), 0x0ul)
    }

    t |> run("heap allocations") @@(t) {

        feint("long long str {payload}\n")
        t |> equal(string_heap_bytes_allocated(), 0x18ul)

        var str = build_string() $(sb) {
            sb |> write("long builder str ")
            sb |> write(payload)
            sb |> write("\n")
        }
        feint(str)

        t |> equal(string_heap_bytes_allocated(), 0x34ul)
    }
}
