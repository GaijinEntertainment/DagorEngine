let { intToStrWithDelimiter } = require("string.nut")
let { roundToDigits } = require("math.nut")

function simpleKeys(table) {
  let res = {}
  foreach(keys, v in table)
    foreach(k in keys)
      res[k] <- v
  return res
}

let decimalFormatDefault = @(value) intToStrWithDelimiter(value, " ")
let decimalFormatByLangs = simpleKeys({
  [["German", "Italian", "Spanish", "Turkish"]] = @(value) intToStrWithDelimiter(value, "."),
  [["English", "Japanese", "Korean"]] = @(value) intToStrWithDelimiter(value, ","),
  [["Chinese", "TChinese", "HChinese"]] = @(value) intToStrWithDelimiter(value, ",", 4),
})

function shortTextFromNumDefault(num) {
  let needSymbol = num >= 9999.5
  let roundNum = roundToDigits(num, needSymbol ? 3 : 4)
  if (!needSymbol)
    return roundNum.tostring()
  if (roundNum >= 1000000000)
    return $"{0.000000001 * roundNum}G"
  else if (roundNum >= 1000000)
    return $"{0.000001 * roundNum}M"
  return $"{0.001 * roundNum}K"
}

let mkChineseStyleNumberCut = @(char10k, char100m) function(num) {
  let needSymbol = num >= 99999.5
  let roundNum = roundToDigits(num, needSymbol ? 4 : 5)
  if (!needSymbol)
    return roundNum.tostring()
  if (roundNum >= 100000000)
    return $"{0.00000001 * roundNum}{char100m}"
  return $"{0.0001 * roundNum}{char10k}"
}

let shortTextFromNumByLangs = simpleKeys({
  [["Chinese", "TChinese", "HChinese"]] = mkChineseStyleNumberCut("万", "亿"),
  [["Japanese"]] = mkChineseStyleNumberCut("万", "億"),
})

return {
  getDecimalFormat = @(language) decimalFormatByLangs?[language] ?? decimalFormatDefault
  getShortTextFromNum = @(language) shortTextFromNumByLangs?[language] ?? shortTextFromNumDefault
}