// options debugger=true
// options log = true

options persistent_heap = true

require fio
require daslib/json
require daslib/json_boost

[export]
def test_reader
    var error = ""
    var jsv = %json~
        {"widget": {
            "debug": "on",
            "window": {
                "title": "Sample Konfabulator Widget",
                "name": "main_window",
                "width": 500,
                "height": 500
            },
            "image": {
                "src": "Images/Sun.png",
                "name": "sun1",
                "hOffset": 250,
                "vOffset": 250,
                "alignment": "center"
            },
            "text": {
                "data": "Click Here",
                "size": 36,
                "style": "bold",
                "name": "text1",
                "hOffset": 250,
                "vOffset": 100,
                "alignment": "center",
                "onMouseUp": "sun1.opacity = (sun1.opacity / 100) * 90;"
            }
        }}
    %%
    print("{write_json(jsv)}\n")
    unsafe
        delete jsv

[export]
def test
    test_reader()
    for jidx in range(1,9)
        let fname = "D:/Work/daScript/examples/test/json/{jidx}.json"
        // let fname = "/Users/borisbatkin/Work/yzg/examples/test/json/{jidx}.json"
        fopen(fname, "rb") <| $ ( f )
            fread(f) <| $ ( data )
                print ( "\n{fname}\n" )
                // print("before read {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
                var error = ""
                var json = read_json(data, error)
                if json == null
                    panic("failed to parse, {error}\n")
                else
                    // macro test
                    if json is _object
                        print("object {int(heap_bytes_allocated())}\n")
                        (json as _object)["foo"] = JV("bar")
                    print("{write_json(json)}\n")
                    print("before delete {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
                    unsafe
                        delete json
                    print("before collect  {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
                    heap_report()
                    unsafe
                        heap_collect(true)
                    print("after collect {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
    print("allocated {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
    unsafe
        heap_collect(true)
    print("after collect {int(heap_bytes_allocated())} {int(string_heap_bytes_allocated())}\n")
    return true

[export]
def main
    test()
