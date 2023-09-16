expect 40300:4  // 40300: duplicate key

[export]
def test
    var tab_i_s <- {{
        1 => "one";
        2 => "two";
        3 => "three";
        1 => "one again"
    }}
    print("tab = {tab_i_s}\n")
    var tab_s_s <- {{
        "1" => "one";
        "2" => "two";
        "3" => "three";
        "1" => "one again"
    }}
    print("tab = {tab_s_s}\n")
    var tab_i <- {{ 1; 2; 3; 1 }}
    print("tab = {tab_i}\n")
    var tab_s <- {{ "1"; "2"; "3"; "1" }}
    print("tab = {tab_s}\n")


