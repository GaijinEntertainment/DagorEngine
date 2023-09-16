require fio
require strings

[export]
def test
    profile(1,"freq dictionary PG") <|
        let fname = get_das_root() + "/../pg/pg.txt"
        fopen(fname, "rb") <| $( f )
            if f != null
                var dict : table<string; int>
                fread(f) <| $(data)
                    var offset, length, index = 0
                    for ch,index in data,count()
                        if is_alpha(ch)
                            if length == 0
                                offset = index
                            length ++
                        elif length != 0
                            unsafe
                                var token = chop(data,offset,length) |> to_lower_in_place() // unsafe
                                dict[token]++
                            length = 0
                var arr : array<tuple<string;int>>
                reserve(arr,length(dict))
                for k,v in keys(dict),values(dict)
                    push(arr,[[auto k,v]])
                sort(arr) <| $(a,b)
                    return a._1 != b._1 ? a._1 > b._1 : a._0 < b._0
                fopen(get_das_root() + "/../pg/das.freq", "wb") <| $( fw )
                    if fw != null
                        for t in arr
                            var ts = "{t._1} {t._0}\n"
                            fwrite(fw,ts)
            else
                print("cant open {fname}\n")
    return true

