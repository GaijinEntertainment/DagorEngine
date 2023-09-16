options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module uriparser_boost shared public

require uriparser public

require daslib/strings_boost
require daslib/sort_boost

def uri_split_full_path ( uri:Uri implicit )
    //! Split the full path of a URI into its components.
    var paths : array<string>
    var pcur = uri.uri.pathHead
    while pcur != null
        paths |> push ( string(pcur.text) )
        pcur = pcur.next
    return <- paths

def uri_compose_query ( query:table<string;string> )
    //! Compose a query string from a table of key-value pairs.
    return build_string <| $ ( var tw )
        var first = true
        for key,value in keys(query),values(query)
            if first
                first = false
            else
                tw |> write("&")
            tw |> write(key)
            if !empty(value)
                tw |> write("=")
                tw |> write(value)

def uri_compose_query_in_order ( query:table<string;string> )
    //! Compose a query string from a table of key-value pairs, in the sorted order.
    var vq : array<tuple<key:string;value:string>>
    for key,value in keys(query),values(query)
        vq |> push([[auto key,value]])
    sort(vq) <| $ ( a, b )
        if a.key==b.key
            return a.value < b.value
        else
            return a.key < b.key
        var first = true
    let res = build_string <| $ ( var tw )
        var first = true
        for kv in vq
            if first
                first = false
            else
                tw |> write("&")
            tw |> write(kv.key)
            if !empty(kv.value)
                tw |> write("=")
                tw |> write(kv.value)
    delete vq
    return res

def scheme ( uri:Uri implicit )
    //! Returns the scheme of a URI.
    return string(uri.uri.scheme)

def user_info ( uri:Uri implicit )
    //! Return the user info of a URI.
    return string(uri.uri.userInfo)

def host ( uri:Uri implicit )
    //! Return the host of a URI.
    return string(uri.uri.hostText)

def port ( uri:Uri implicit )
    //! Return the port of a URI.
    return string(uri.uri.portText)

def path ( uri:Uri implicit )
    //! Return the path of a URI.
    return build_string() <| $ ( var tw )
        var pcur = uri.uri.pathHead
        var first = true
        while pcur != null
            if first
                first = false
            else
                tw |> write("/")
            tw |> write ( string(pcur.text) )
            pcur = pcur.next

def query ( uri:Uri implicit )
    //! Return the query of a URI.
    return string(uri.uri.query)

def fragment ( uri:Uri implicit )
    //! Return the fragment of a URI.
    return string(uri.uri.fragment)

def uri_compose ( scheme, userInfo, hostText, portText, path, query, fragment : string ) : Uri
    //! Compose a URI from its components.
    let text = build_string <| $ ( var tw )
        if !empty(scheme)
            tw |> write(scheme)
            tw |> write("://")
        if !empty(userInfo)
            to_log(LOG_TRACE, "uri_compose: USER = {userInfo}\n")
            tw |> write(userInfo)
            tw |> write("@")
        if !empty(hostText)
            to_log(LOG_TRACE, "uri_compose: HOST = {hostText}\n")
            tw |> write(hostText)
        if !empty(portText)
            tw |> write(":")
            tw |> write(portText)
        if !empty(path)
            tw |> write("/")
            tw |> write(path)
        if !empty(query)
            tw |> write("?")
            tw |> write(query)
        if !empty(fragment)
            tw |> write("#")
            tw |> write(fragment)
    unsafe
        return <- Uri(text)
