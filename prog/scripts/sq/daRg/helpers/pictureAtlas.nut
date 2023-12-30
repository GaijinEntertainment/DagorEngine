from "daRg" import Picture

function PictureAtlas(atlas_path_base) {
  let pictures = {}

  let cls = class {
    function _get(key) {
      if (key in pictures)
        return pictures[key]
      let pic = Picture(atlas_path_base+key)
      pictures[key] <- pic
      return pic
    }
  }

  return cls()
}


return PictureAtlas
