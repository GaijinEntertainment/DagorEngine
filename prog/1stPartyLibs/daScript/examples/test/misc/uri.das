require uriparser

[export]
def test
    let uri = "file:///d%3A/dev/cpp/daScript/cmake_temp/Debug/mod.das"
    let path = uri_to_file_name(uri)
    let buri = file_name_to_uri(path)
    let eburi = escape_uri(buri,true,true)
    let ueburi = unescape_uri(eburi)
    print("\nuri\t\t{uri}\npath\t\t{path}\nb-uri\t\t{buri}\ne-b-uri\t\t{eburi}\nu-e-b-uri\t{ueburi}\n")
    return true