function [pure] armorClass(armor) {
  return armor > 50 ? "heavy" : "light"
}

function penetration(caliber, speed) {
  return caliber * speed
}

function mkCrate(slots) {
  return array(slots)
}

return { armorClass, penetration, mkCrate }
