options gen2
// A lint-skip-file directive is honored in the file HEADER only. The one below
// sits past the header window, so it is prose - this file must lint normally.
// Filler lines push the quoted directive out of the window:
//
//
//
//
//
//
//
//
//
//
//
//
// lint-skip-file: this deep occurrence must NOT unlint the file

def private twice(x : int) : int => x + x
