require ecs
require math

[sideeffects]
def get_izero() : int
  return 0

[sideeffects]
def get_zero() : float
  return 0.

[es(on_event=EventStart)]
def test_division_by_zero(evt : Event)
  let izero = get_izero()
  verify(izero == 0)
  var izeroDivIsError = false
  try
    print("div by izero")
    verify(1 / izero == 0)
  recover
    print("div by izero - error")
    izeroDivIsError = true
  verify(izeroDivIsError)

/*
  let zero = get_zero()
  verify(zero == 0.)
  let zeroDiv = 1. / zero
  verify(!is_finite(zeroDiv))

  let zero2 = get_zero()
  let notanum = zero / zero2
  verify(is_nan(notanum))
*/


[es(on_event=EventStart)]
def test_nan(evt : Event)
  var notanum : float = unsafe(reinterpret<float> 0x7faa81da)
  verify(is_nan(notanum))
  verify(is_nan(1. / notanum))
  verify(is_nan(1. * notanum))
/*
  verify(!(1. > notanum))
  verify(!(1. < notanum))
  verify(!(0. > notanum))
  verify(!(0. < notanum))
*/
