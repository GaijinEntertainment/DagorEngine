options gen2
expect 30113, 30158, 30166, 30192

require UnitTest
[export]
def a{[iterator for((r) in "{({for(i in{0=>0});SomeEnum.})}"); 1] as a; }
