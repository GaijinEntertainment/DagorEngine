var ctor_count = 0
var to_upper_count = 0
var utf8_count = 0

class MyString
	def MyString
		ctor_count ++
	def to_upper(): MyString?
		to_upper_count ++
		return addr(self)
	def utf8(): MyString?
		utf8_count ++
		return addr(self)

[export]
def test
    let res = new MyString()->to_upper()->utf8()
    assert(1 == ctor_count)
    assert(1 == to_upper_count)
    assert(1 == utf8_count)
    return true


