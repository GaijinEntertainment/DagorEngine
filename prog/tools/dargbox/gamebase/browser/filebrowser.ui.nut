from "frp"import set_subscriber_validation, warn_on_deprecated_methods
from "%darg/ui_imports.nut" import *
from "math" import min
import "samples_prog/_basic/components/textInput.nut" as textInput
from "dagor.clipboard" import set_clipboard_text
from "dagor.workcycle" import resetTimeout
from "dagor.fs" import scan_folder

let scrollbar = require("scrollbar.nut")
let fs_node_button = require("fs_node.nut")
let { listdir, startPath, startFile } = require("filetree.nut")
let { is_in_vr } = require("dargbox.vr")

set_subscriber_validation( true )
warn_on_deprecated_methods( true )

enum BM {
  ADDITIVE
  ALPHA_BLEND
  ADDITIVE_PREMULTIPLY
  TOTAL
}

let thumbSizes = [
  { size = hdpxi(50),  border = hdpxi(1), text = "small" }
  { size = hdpxi(100), border = hdpxi(2), text = "normal" }
  { size = hdpxi(200), border = hdpxi(3), text ="big" }
]

let backgroundColors = is_in_vr() ? [0x00000000]
  : [0xFF404040, 0xFF000000, 0xFFFFFFFF, 0xFF800000, 0xFF008000, 0xFF000080]
let hoverColor = 0xC07BFFFF
let pushedColor = 0xFFFFFFFF

let panelW = min(sh(30), sw(30))
let margin = 2 * (hdpxi(20) / 2 + 0.5).tointeger()
let wndWidth = sw(100) - panelW - margin

let curPath = mkWatched(persist, "curPath", startPath)
let curFile = mkWatched(persist, "curFile", startFile)
let curFileImageSize = mkWatched(persist, "curFileImageSize", null)
let imageMouseCoords = Watched(null)
let blendMode = mkWatched(persist, "blendMode", BM.ALPHA_BLEND)
let curThumbIdx = mkWatched(persist, "curThumbIdx", 1)
let backgroundColorIdx = mkWatched(persist, "backgroundColorIdx", 0)
let needBorders = mkWatched(persist, "needBorders", false)

let curThumbCfg = Computed(@() thumbSizes[curThumbIdx.get() % thumbSizes.len()])
let thumbSize = Computed(@() curThumbCfg.get().size)
let thumbBorder = Computed(@() curThumbCfg.get().border)
let backgroundColor = Computed(@() backgroundColors[backgroundColorIdx.get() % backgroundColors.len()])
let nextBgColor = Computed(@() backgroundColors[(backgroundColorIdx.get() + 1) % backgroundColors.len()])

let imageByBlendMode = {
  [BM.ADDITIVE] = @(path, svgSize) path.endswith(".svg") ? $"{path}:{svgSize[0]}:{svgSize[1]}" : path,
  [BM.ADDITIVE_PREMULTIPLY] = @(path, svgSize) path.endswith(".svg") ? $"{path}:{svgSize[0]}:{svgSize[1]}:P" : $"{path}:0:P",
  [BM.ALPHA_BLEND] = @(path, svgSize) path.endswith(".svg") ? $"!{path}:{svgSize[0]}:{svgSize[1]}" : $"!{path}"
}

let blendModeNames = {
  [BM.ADDITIVE] = "additive",
  [BM.ADDITIVE_PREMULTIPLY] = "premultiplied",
  [BM.ALPHA_BLEND] = "alphaBlend",
}

let mkPicture = @(bMode, path, svgSize) Picture(imageByBlendMode[bMode](path, svgSize))

function transformPath(path){
  if (path.contains("..")){
    let patharr = path.split("/")
    let res = []
    foreach(i, p in patharr) {
      if (p == ".." && ![".",".."].contains((patharr?[i-1] ?? ".."))) {
        res.append(null)
        res[i-1] = null
      }
      else
        res.append(p)
    }
    path = "/".join(res.filter(@(v) v!=null))
  }
  return path
}

let stub_component = {
  size = flex() rendObj = ROBJ_SOLID color = Color(50,50,50)
  halign = ALIGN_CENTER valign=ALIGN_CENTER
  flow = FLOW_VERTICAL
  children = [
    {
      rendObj = ROBJ_TEXT
      text = "Please select sample on the right side"
    }
    {
      rendObj = ROBJ_TEXT
      halign = ALIGN_CENTER
      valign = ALIGN_BOTTOM
      size = [flex(), SIZE_TO_CONTENT]
      color = Color(120,120,120)
      fontSize = 14
      text = "(F7 to hotreload)"
    }
  ]
}

function isImage(path) {
  let images_file_types = const [".png",".jpg",".tga",".svg",".dds",".ddsx", ".avif"]
  foreach (ft in images_file_types) {
    if (path.endswith(ft))
      return true
  }
  return false
}

function isVideo(path) {
  let video_file_types = const [".ivf",".ogm",".ogg"]
  foreach (ft in video_file_types) {
    if (path.endswith(ft))
      return true
  }
  return false
}

function isUi(path) {
  if (type(path) =="string" && path.endswith(".ui.nut"))
    return true
  return false
}

function isShader(path) {
  return type(path) == "string" && (path.endswith("_darg.hlsl") || path.endswith("_ui.hlsl"))
}

function isText(path){
  let images_file_types = const [".nut",".txt"]
  foreach (ft in images_file_types) {
    if (path.slice(-ft.len() ) == ft)
      return true
  }
  return false
}

let curFilePicture = Computed(@() curFile.get() == null || !isImage(curFile.get()) ? null
  : mkPicture(blendMode.get(), curFile.get(), [sw(50), sh(50)]))

function setCurFileImageSize(v){
  imageMouseCoords.set(null)
  curFileImageSize.set(v)
}

function updateFileImageSize() {
  if (curFilePicture.get() == null || curFile.get().endswith(".svg")) {
    setCurFileImageSize(null)
    return
  }
  let size = curFilePicture.get().getLoadedPicSize()
  if (size.x != 0 && size.y != 0)
    setCurFileImageSize([size.x, size.y])
  else
    resetTimeout(0.2, updateFileImageSize)
}
function setCurFile(v) {
  curFile.set(v)
  updateFileImageSize()
}
updateFileImageSize()

let textarea = @(text) {
  size = [flex(), SIZE_TO_CONTENT]
  rendObj = ROBJ_TEXTAREA
  behavior = Behaviors.TextArea
  halign = ALIGN_CENTER
  text
}
function loadUiComp(path) {
  if (!isUi(path))
    return null
  else if (path.endswith("filebrowser.ui.nut"))
    return textarea("Can't recursively load browser")
  else {
    try{
      return require(curFile.get())
    }
    catch(e){
      return textarea($"error loading {curFile.get().split("/").top()}:\n{e}")
    }
  }
}
loadUiComp(curFile.get())

let imagePreview = @() {
  watch = curFilePicture
  key = "imagePreview"
  size = flex()
  rendObj = ROBJ_IMAGE
  image = curFilePicture.get()
  keepAspect = true

  behavior = Behaviors.ProcessPointingInput
  function onPointerMove(evt) {
    if (curFileImageSize.get() == null)
      return 0
    let { r = 0, l = 0, t = 0, b = 0 } = gui_scene.getCompAABBbyKey("imagePreview")
    if (r == l || t == b)
      return 0
    let texSize = curFileImageSize.get()
    let scrnSize = [r - l, b - t]
    let sizeMul = min(scrnSize[0].tofloat() / texSize[0], scrnSize[1].tofloat() / texSize[1])
    let offset = texSize.map(@(v, a) ((scrnSize[a] - v * sizeMul) / 2 + 0.5).tointeger())
    let { x, y } = evt
    let pos = [x - l, y - t]
    let texPos = pos.map(@(v, a) ((v - offset[a]) / sizeMul + 0.5).tointeger())
    if (texPos[0] > 0 && texPos[0] <= texSize[0] && texPos[1] > 0 && texPos[1] <= texSize[1])
      imageMouseCoords.set(texPos)
    else
      imageMouseCoords.set(null)
    return 0
  }
}

function previewComp() {
  let res = {
    watch = curFile
    size = flex()
    behavior = Behaviors.Button
    onClick = @() setCurFile(null)
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    clipChildren = true
  }

  let path = curFile.get()
  if (type(path) != "string")
    return res

  if (isUi(path))
    return res.__update({
      key = curFile.get()
      behavior = null
      children = loadUiComp(curFile.get())
    })

  if (isImage(path))
    return res.__update({ children = imagePreview })

  if (isShader(path))
    return res.__update({
      children = {
        size = flex() halign = ALIGN_CENTER valign = ALIGN_CENTER
        children = {
          size = [sw(50), sh(50)]
          rendObj = ROBJ_SHADER
          shaderSource = path.split("/").top()
          key = path
        }
      }
    })

  if (isVideo(path))
    return res.__update({
      children = {
        size=flex() halign=ALIGN_CENTER valign=ALIGN_CENTER
        children = @(){
          size = [sw(60), sh(60)]
          keepAspect = true
          rendObj = ROBJ_MOVIE
          behavior = Behaviors.Movie
          movie = path.startswith("./") ? path.slice(2) : path
          key = path
        }
      }
    })

  if (isText(path))
    //todo load text
    return res.__update({
      padding = sh(2)
      gap = sh(2)
      flow= FLOW_VERTICAL
      children = [
        {rendObj =ROBJ_TEXT text=path color=Color(128,128,128)}
        {
          rendObj = ROBJ_TEXTAREA
          textOverflowY = TOVERFLOW_LINE
          halign = ALIGN_LEFT
          ellipsis = true
          ellipsisSepLine = false
          text = $"we do not have function to load file to text: {path}"
          behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
          maxWidth = flex()
          maxHeight = flex()
        }
      ]
    })

  return res
}


let searchString = mkWatched(persist, "searchString", "")

let sortedCurDirContent = Computed(function sorted_files(){
  let list = listdir(curPath.get())
  let files = list.map(function(v){
    if (v.isDir)
      throw null
    return {id = v.name, type="file"}
  })
  let dirs = list.map(function(v){
    if (!v.isDir)
      throw null
    return {id = v.name, type="dir"}
  })
  return {files, dirs}
})

let curDirFiles = Computed(@() sortedCurDirContent.get().files)

let curDirImages = Computed(@() curDirFiles.get().map(@(v) "/".concat(curPath.get(), v.id)).filter(@(v) isImage(v) || isVideo(v) ))

function mkImageThumb(file, size, border, bMode) {
  let imgComp = {
    size = [size, size]
    rendObj = ROBJ_IMAGE
    image = isVideo(file) ? Picture($"browser/images/video-file.avif:{size}:{size}:P")
      : mkPicture(bMode, file, [size, size])
    keepAspect = true
    vplace = ALIGN_CENTER
    behavior = Behaviors.Button
    onClick = @() setCurFile(file)
  }
  return @() {
    watch = [needBorders, nextBgColor]
    padding = border
    rendObj = needBorders.get() ? ROBJ_BOX : null
    borderWidth = border
    borderColor = nextBgColor.get()
    children = imgComp
  }
}

let changeBackgroundColorBtn = @() {
  watch = backgroundColorIdx
  behavior = Behaviors.Button
  onClick = @() backgroundColorIdx.set((backgroundColorIdx.get() + 1) % backgroundColors.len())
  size = [hdpx(50), hdpx(50)]
  rendObj = ROBJ_FRAME
  fillColor = backgroundColors[(backgroundColorIdx.get() + 1) % backgroundColors.len()]
  color     = backgroundColors[(backgroundColorIdx.get() + 2) % backgroundColors.len()]
  borderWidth = hdpx(4)
}

let premultipleAlphImagesBtn = {
  behavior = Behaviors.Button
  onClick = @() blendMode.set((blendMode.get() + 1) % BM.TOTAL)
  rendObj = ROBJ_BOX
  children = @() {
    watch = blendMode
    rendObj = ROBJ_TEXT
    text = blendModeNames[blendMode.get()]
  }
}

let needBordersBtn = {
  behavior = Behaviors.Button
  onClick = @() needBorders.set(!needBorders.get())
  rendObj = ROBJ_BOX
  children = @() {
    watch = needBorders
    rendObj = ROBJ_TEXT
    text = needBorders.get() ? "with border" : "no border"
  }
}

let thumbSizeBtn = {
  behavior = Behaviors.Button
  onClick = @() curThumbIdx.set((curThumbIdx.get() + 1) % thumbSizes.len())
  rendObj = ROBJ_BOX
  children = @() {
    watch = curThumbCfg
    rendObj = ROBJ_TEXT
    text = curThumbCfg.get().text
  }
}

let mkGallery = @(images) scrollbar.makeVertScroll(@() {
  watch = [thumbSize, thumbBorder, blendMode]
  clipChildren = true
  children = wrap(images.map(@(path) mkImageThumb(path, thumbSize.get(), thumbBorder.get(), blendMode.get())),
    { width = wndWidth, hGap = hdpx(10), vGap = hdpx(10), halign = ALIGN_CENTER })
  rendObj = ROBJ_FRAME
  color = Color(60,60,60)
  padding = margin
  hplace = ALIGN_CENTER
})

function fs_node(filename, path, isDir=false, fullpath=false) {
  let id = fullpath ? path : (path!="" ? "/".concat(path, filename) : filename)
  local fileType = null
  if ( isDir)
    fileType = "dir"
  else if (isUi(id))
    fileType = "ui"
//  else if (isText(id))
//    fileType = "txt"
  else if (isShader(id))
    fileType = "shader"
  else if (isImage(id) || isVideo(id))
    fileType = "img"
  function onclick() {
    if (filename == "..") {
      curPath.set(transformPath($"{path}/.."))
      setCurFile(null)
    }
    else if (curFile.get() == id)
      setCurFile(null)
    else if (isDir) {
      curPath.set(transformPath(id))
      setCurFile(null)
    }
    else if (fileType)
      setCurFile(transformPath(id))
    else
      vlog("file: ", path, "/", filename)
  }
  if (!isDir && fileType==null)
    return null //skip unknown formats
  let current = curFile.get() == id
  return fs_node_button(filename, {onclick, current, isDir, image=(fileType=="ui") ? "ui" : (fileType=="shader") ? "shader" : null})
}

function file_browser() {
  let cpath = curPath.get()
  let children = []
  let {files, dirs} = sortedCurDirContent.get()
  children.append(fs_node("..", cpath, true))
  foreach (dir in dirs) {
    children.append(fs_node(dir.id, cpath, true))
  }
  foreach (file in files) {
    children.append(fs_node(file.id, cpath, false))
  }
  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Color(0,0,0,120)
    watch =  [sortedCurDirContent, curFile, curPath]
    key = cpath
    children = scrollbar.makeVertScroll({
      flow = FLOW_VERTICAL
      size = FLEX_H
      clipChildren = true
      halign = ALIGN_LEFT
      padding = sh(0.5)
  //    gap = sh(0.1)
      children
    })
  }
}

let searchStringNonEmpty = Computed(@() (searchString.get() ?? "") != "")

function search_results(){
  if (!searchStringNonEmpty.get())
    return { watch = searchStringNonEmpty }
  return {
    flow = FLOW_VERTICAL
    gap = hdpx(1)
    watch = searchStringNonEmpty
    size = flex()
    children = [
      function() {
        let cpath = curPath.get()
        let searchS = searchString.get() ?? ""
        let children = []
        let files = searchS == "" ? [] : scan_folder({path=cpath, recursive=true, realfs=true, vromfs=false}).filter(@(v) v.contains(searchS)).sort(@(a,b) a<=>b)
        foreach (file in files) {
          children.append(fs_node(file.split("/").top(), file, false, true))
        }

        return {
          size = flex(1)
          watch = [searchString, curPath]
          flow = FLOW_VERTICAL

          children = [
            {rendObj = ROBJ_TEXT, text = $"found files: {files.len()}"}

            scrollbar.makeVertScroll({
              flow = FLOW_VERTICAL
              size = FLEX_H
              clipChildren = true
              halign = ALIGN_LEFT
              padding = sh(0.5)
          //    gap = sh(0.1)
              children
            })
          ]
        }
      }
      @() {
        size = flex(2)
        children = file_browser
      }
    ]

  }
}


function files_panel() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    size = [panelW, flex()]
    gap = sh(0.5)
    watch = searchString
    margin = [60,0,0,0]
    hplace =ALIGN_RIGHT
    flow = FLOW_VERTICAL
    children = [
      textInput(searchString, {placeholder="find files"}),
      @(){
        flow = FLOW_VERTICAL
        gap = sh(0.5)
        size = flex()
        watch = searchString
        children = (searchString.get() ?? "") != "" ? search_results : [
          {
            rendObj = ROBJ_TEXT
            halign = ALIGN_CENTER
            valign = ALIGN_BOTTOM
            size = [flex(), SIZE_TO_CONTENT]
            maxHeight = hdpx(30)
            color = Color(120,120,120)
            text = "Select file to show"
          }
          file_browser
        ]
      }
    ]
  }
}

let componentPreview = @() {
  watch = [curFile, curDirImages]
  size = [sw(100)-panelW, flex()]
  rendObj = ROBJ_FRAME
  color  = Color(55,55,50)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {
    size = flex()
    children = curFile.get() != null ? previewComp
      : curDirImages.get().len() > 0 ? mkGallery(curDirImages.get())
      : stub_component
  }
}

function previewPanel() {
  let fileNameSf = Watched(0)
  let pathSf = Watched(0)
  let sizeSf = Watched(0)
  return {
    flow = FLOW_VERTICAL
    size = flex()
    children = [
      {
        flow = FLOW_HORIZONTAL
        size = [flex(), SIZE_TO_CONTENT]
        gap = 2 * margin
        valign = ALIGN_CENTER
        children = [
          changeBackgroundColorBtn
          premultipleAlphImagesBtn
          thumbSizeBtn
          needBordersBtn
        ]
      }
      componentPreview
      {
        size = [flex(), SIZE_TO_CONTENT]
        padding = margin / 2
        children = [
          {
            flow = FLOW_HORIZONTAL
            gap = hdpx(20)
            children = [
              @() {
                watch = [curFile, fileNameSf, curDirFiles, curDirImages]
                behavior = curFile.get() != null ? Behaviors.Button : null
                onElemState = @(sf) fileNameSf.set(sf)
                onClick = @() curFile.get() != null ? set_clipboard_text(curFile.get().split("/").top()) : null
                rendObj = ROBJ_TEXT
                text = curFile.get() != null ? $"file: {curFile.get().split("/").top()}"
                  : $"images: {curDirImages.get().len()}, other files: {curDirFiles.get().len()-curDirImages.get().len()}"
                color = fileNameSf.get() & S_ACTIVE ? pushedColor
                  : fileNameSf.get() & S_HOVER ? hoverColor
                  : null
              }
              @() {
                watch = [curFileImageSize, sizeSf]
                behavior = curFileImageSize.get() != null ? Behaviors.Button : null
                onElemState = @(sf) sizeSf.set(sf)
                onClick = @() curFileImageSize.get() == null ? null
                  : set_clipboard_text($"{curFileImageSize.get()[0]}, {curFileImageSize.get()[1]}")
                rendObj = ROBJ_TEXT
                text = curFileImageSize.get() == null ? null : $"{curFileImageSize.get()[0]}, {curFileImageSize.get()[1]}"
                color = sizeSf.get() & S_ACTIVE ? pushedColor
                  : sizeSf.get() & S_HOVER ? hoverColor
                  : null
              }
              @() {
                watch = imageMouseCoords
                rendObj = ROBJ_TEXT
                text = imageMouseCoords.get() == null ? null : $"(mouse: {imageMouseCoords.get()[0]}, {imageMouseCoords.get()[1]})"
              }
            ]
          }
          @() {
            watch = [curPath, pathSf]
            hplace = ALIGN_RIGHT
            behavior = Behaviors.Button
            onElemState = @(sf) pathSf.set(sf)
            onClick = @() set_clipboard_text(curPath.get())
            rendObj = ROBJ_TEXT
            text = curPath.get()
            color = pathSf.get() & S_ACTIVE ? pushedColor
              : pathSf.get() & S_HOVER ? hoverColor
              : null
          }
        ]
      }
    ]
  }

}

return {
  size = flex()
  margin = [0, 1, 1, 0]
  cursor = Cursor({
    rendObj = ROBJ_VECTOR_CANVAS
    size = [sh(2.5), sh(2.5)]
    commands = [
      [VECTOR_WIDTH, hdpx(1.4)],
      [VECTOR_FILL_COLOR, Color(10,10,10,200)],
      [VECTOR_COLOR, Color(180,180,180,100)],
      [VECTOR_POLY, 0,0, 80,65, 46,65, 35,100],
    ]
  })
  stopMouse = true //< for VR mainly

  children = [
    @() {
      watch = backgroundColor
      size = flex()
      color = backgroundColor.get()
      rendObj = ROBJ_SOLID
    }
    {
      size = flex()
      flow = FLOW_HORIZONTAL
      children = [
        previewPanel
        files_panel
      ]
    }
  ]
}
