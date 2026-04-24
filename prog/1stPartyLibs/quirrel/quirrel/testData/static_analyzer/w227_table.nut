if (__name__ == "__analysis__")
  return


let getSoldierFace = @() 1

let _fx = 10

// This is not a name shadowing
return  {
    getSoldierFace = @() getSoldierFace()
    _fx = 100
}
