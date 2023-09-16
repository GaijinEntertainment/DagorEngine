require daslib/decs_boost

[decs_template]
struct Text
    text: string
    color: uint

def game_end(win)
    var text = "1234"
    var color: uint;

    create_entity <| @ ( eid, cmp )
        cmp |> set("eid", eid)
        apply_decs_template(cmp, [[Text text = text, color = color]])

    create_entity <| @ ( eid, cmp )
        cmp |> set("eid", eid)
        apply_decs_template(cmp, [[Text text = text, color = color]])