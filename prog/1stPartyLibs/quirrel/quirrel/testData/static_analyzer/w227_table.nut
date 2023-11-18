let getSoldierFace = @() 1

let fx = 10

return  {
    getSoldierFace = @() getSoldierFace()
    fx = 100
}