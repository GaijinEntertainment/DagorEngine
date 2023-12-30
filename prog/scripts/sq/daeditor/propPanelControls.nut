let fieldEditText = require("components/apFieldEditText.nut")
let fieldBoolCheckbox = require("components/apFieldBoolCheckbox.nut")

let perCompEdits = {}

let perSqTypeCtors = {
  string  = fieldEditText
  integer = fieldEditText
  float   = fieldEditText
  Point2  = fieldEditText
  Point3  = fieldEditText
  DPoint3 = fieldEditText
  Point4  = fieldEditText
  IPoint2 = fieldEditText
  IPoint3 = fieldEditText
  E3DCOLOR= fieldEditText
  TMatrix = fieldEditText
  bool    = fieldBoolCheckbox
}

function registerPerCompPropEdit(compName, ctor) {
  perCompEdits[compName] <- ctor
}

function registerPerSqTypePropEdit(compName, ctor) {
  perSqTypeCtors[compName] <- ctor
}
let getCompNamePropEdit = @(compName) perCompEdits?[compName]
let getCompSqTypePropEdit = @(typ) perSqTypeCtors?[typ]

return {registerPerCompPropEdit, registerPerSqTypePropEdit, getCompSqTypePropEdit, getCompNamePropEdit}