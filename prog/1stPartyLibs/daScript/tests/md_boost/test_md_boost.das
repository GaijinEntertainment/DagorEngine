options gen2

require daslib/md_boost
require dastest/testing_boost public

[test]
def test_md_primitives(t : T?) {
    t |> run("md_bold + md_escape") @(t : T?) {
        t |> equal(md_bold("0.61x"), "**0.61x**")
        t |> equal(md_escape("a|b|c"), "a\\|b\\|c")
        t |> equal(md_escape("no pipe"), "no pipe")
    }
}

[test]
def test_render_md_table(t : T?) {
    t |> run("render_md_table: aligned columns, GitHub separators, min-width 3") @(t : T?) {
        let headers <- ["a", "bb"]
        let aligns <- [ColAlign.left, ColAlign.right]
        let rows <- [["x", "10"], ["yyy", "2"]]
        let got = render_md_table(headers, aligns, rows)
        let expected = "| a   |  bb |\n| :-- | --: |\n| x   |  10 |\n| yyy |   2 |"
        t |> equal(got, expected)
    }
}

[test]
def test_render_md_table_bold_aligns(t : T?) {
    t |> run("bolded winner still aligns (** counted in width)") @(t : T?) {
        let headers <- ["file", "das/cli"]
        let aligns <- [ColAlign.left, ColAlign.right]
        let rows <- [["jfk.wav", md_bold("0.61x")], ["gb1.wav", "1.02x"]]
        let got = render_md_table(headers, aligns, rows)
        // das/cli column width = len("**0.61x**") = 9; "1.02x" right-pads to 9
        let expected = "| file    |   das/cli |\n| :------ | --------: |\n| jfk.wav | **0.61x** |\n| gb1.wav |     1.02x |"
        t |> equal(got, expected)
    }
}
