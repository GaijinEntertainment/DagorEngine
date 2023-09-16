let { readCsvFile } = require("tools/localization_tools/langs_validation.nut")
let { get_time_msec } = require("dagor.time")


local function loadLangFromFiles(lang_dir, lang_files, skip_last_cols = 0) {
  local fullLoadedLang = {}

  let startTime = get_time_msec()
  local prevTime = startTime
  print("loadLangFromFiles - start loading\n")

  foreach (langFile in lang_files) {
    local fileName = $"{lang_dir}{langFile}"
    local file = readCsvFile(fileName, langFile, false /*shouldPrintWeakErrors*/)
    local fileLanguages = {}

    foreach (tr, row in file) {
      if (tr == 0) {
        foreach (_n, v in row) {
          if (v.tc > 0 && v.tc < (row.len() - skip_last_cols)) {
            fileLanguages[v.tc] <- v.val.replace("<", "").replace(">", "")
          }
        }
      }
      else {
        local key = ""
        foreach (_n, v in row) {
          if (v.tc == 0) {
            key = v.val
            fullLoadedLang[key] <- {}
          }
          else if (v.tc < (row.len() - skip_last_cols)) {
            fullLoadedLang[key][fileLanguages[v.tc]] <- v.val
          }
        }
      }
    }

    let now = get_time_msec()
    print($"  processed file {langFile} in {now - prevTime} ms\n")
    prevTime = now
  }

  let now = get_time_msec()
  print($"loadLangFromFiles - finished in {now - startTime} ms\n")

  return fullLoadedLang
}

return {
  loadLangFromFiles
}
