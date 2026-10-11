/**
 * Parse SubRip (.srt) subtitle text.
 * Returns cues sorted by start time: [{ from, to, text }], where from and to
 * are milliseconds and text holds the cue lines verbatim, joined with "\n".
 * A leading BOM, CRLF line ends and a missing cue index line are tolerated,
 * both "," and "." are accepted as the second fraction separator, and a cue
 * that does not parse is skipped.
 */

const TIME_SEPARATOR = "-->"
const BOM = "\xEF\xBB\xBF"

function [pure] isDigits(str: string): bool {
  if (str.len() == 0)
    return false
  foreach (ch in str)
    if (ch < '0' || ch > '9')
      return false
  return true
}

function [pure] parseTimeMsec(str: string): int|null {
  let hms = str.strip().split(":")
  if (hms.len() != 3)
    return null
  let secMsec = hms[2].replace(",", ".").split(".")
  let msec = secMsec.len() > 1 ? secMsec[1] : "0"
  if (!isDigits(hms[0]) || !isDigits(hms[1]) || !isDigits(secMsec[0]) || !isDigits(msec))
    return null
  return ((hms[0].tointeger() * 60 + hms[1].tointeger()) * 60 + secMsec[0].tointeger()) * 1000
    + msec.tointeger()
}

function [pure] parseSrt(text: string): array {
  let lines = (text.startswith(BOM) ? text.slice(BOM.len()) : text).split("\n")
  let cues = []
  local idx = 0
  while (idx < lines.len()) {
    let timing = lines[idx].strip()
    idx++
    let separatorAt = timing.indexof(TIME_SEPARATOR)
    if (separatorAt == null)
      continue

    let from = parseTimeMsec(timing.slice(0, separatorAt))
    let to = parseTimeMsec(timing.slice(separatorAt + TIME_SEPARATOR.len()))
    let textLines = []
    while (idx < lines.len()) {
      let raw = lines[idx]
      let cueLine = raw.endswith("\r") ? raw.slice(0, -1) : raw
      if (cueLine.strip() == "")
        break
      textLines.append(cueLine)
      idx++
    }

    if (from != null && to != null && to > from && textLines.len() > 0)
      cues.append({ from, to, text = "\n".join(textLines) })
  }
  cues.sort(@(a, b) a.from <=> b.from)
  return cues
}

return freeze({
  parseSrt
})
