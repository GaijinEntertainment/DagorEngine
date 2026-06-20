// unterminated comment at end of file
// the nested comment construct /*//*/ triggers error 10007
options gen2
expect 10301

/*//*/
[export]
def test() {
    feint("ok")
}