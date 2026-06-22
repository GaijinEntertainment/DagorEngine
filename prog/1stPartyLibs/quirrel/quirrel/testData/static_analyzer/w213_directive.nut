function _foo(x) {
  if (x == 1) {
    #allow-delete-operator
    delete x.a
  }
  else {
    #allow-delete-operator
    delete x.a
  }
}
