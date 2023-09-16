let { format } = require("string")
let { scan_folder } = require("dagor.fs")
let { file } = require("io")
let { exit } = require("dagor.system")
let regexp2 = require("regexp2")
let utf8 = require("utf8")
let { tostring_r } = require("%sqstd/string.nut")
let { isEqual } = require("%sqstd/underscore.nut")

let DEBUG_MODE = false

local currencySymbols = {
  warthunder = [
    { symbol = "\u00A4", name = "Golden Eagles" },
    { symbol = "\u20AC", name = "Silver Lions" },
    { symbol = "\u2589", name = "Research Points, Exp" },
    { symbol = "\u2586", name = "Skill Points" },
    { symbol = "\u00A3", name = "Warbonds" },
    { symbol = "\u22EC", name = "Gaijin Coins" },
    { symbol = "\u2509", name = "Squadron Activity" },
  ]
}

local langsByPluralFormsCount = [
  /* 0 */ [],
  /* 1 */ [ "Chinese", "Japanese", "Vietnamese", "Korean", "TChinese", "HChinese" ],
  /* 2 */ [ "English", "French", "Italian", "German", "Spanish", "Turkish", "Portuguese", "Hungarian", "Georgian", "Greek" ],
  /* 3 */ [ "Russian", "Polish", "Czech", "Ukrainian", "Serbian", "Belarusian", "Romanian", "Croatian" ],
]

local totalRowsChecked = 0
local totalStats = { errors = {}, warnings = {} }
local lastErr = null
local curLangPluralFormsCount = 0
local verboseOutput = false

local function getPluralFormsCount(lang) {
  foreach (pluralFormsCount, langs in langsByPluralFormsCount)
    if (langs.indexof(lang) != null)
      return pluralFormsCount
  return 0
}

local function padText(multilineText, firstLinePad, nextLinesPad) {
  return "".concat(firstLinePad, $"\n{nextLinesPad}".join(multilineText.split("\n")))
}

local function _printErr(errorId, isError, filename, row, fileLn, fileCol, tableRow, tableCol, msg, fieldText) {
  local shouldSkip = lastErr?.filename == filename && lastErr?.fileLn == fileLn &&
    lastErr?.fileCol == fileCol-1 && lastErr?.errorId == errorId
  lastErr = { filename = filename, fileLn = fileLn, fileCol = fileCol, errorId = errorId }
  if (shouldSkip)
    return

  local stats = totalStats[isError ? "errors" : "warnings"]
  stats[errorId] <- (stats?[errorId] ?? 0) + 1

  local msgType = isError ? "ERROR" : "WARNING"
  local tableColChar = format("%c", 'A' + tableCol)
  local spreadsheetCellLabel = verboseOutput ? "CSV spreadsheet cell: " : ""
  local spreadsheetCellName = tableRow >= 0 && tableCol >= 0 ? $" ({spreadsheetCellLabel}{tableColChar}{tableRow+1})" : ""
  local errorLocation = $"{filename}:{fileLn+1}:{fileCol+1}{spreadsheetCellName}"

  if (verboseOutput) {
    local pad = "                  "
    local lines = [
      { label = "    TRANSLATION:  ", value = row?[1].val ?? "", show = row != null },
      { label = "    ENGLISH:      ", value = row?[2].val ?? "", show = row != null },
      { label = "    LOC_ID:       ", value = row?[0].val ?? "", show = row != null },
      { label = "    TEXT:         ", value = fieldText    ,     show = row == null && fieldText != "" },
      { label = "    FILE:         ", value = errorLocation,     show = true },
    ]
    print($"\n{msgType}: {msg}\n")
    foreach (l in lines)
      if (l.show)
        print("".concat(padText(l.value, l.label, pad), "\n"))
  }
  else {
    local pad = "    "
    print($"{msgType}: {errorLocation} - {msg}\n")
    if (fieldText != "")
      print("".concat(padText(fieldText, pad, pad), "\n"))
  }

  if (DEBUG_MODE)
    print("".concat("DEBUG: ", tostring_r(row), "\n"))
}

local function printParsingError(filename, fileLn, fileCol, id, msg) {
  _printErr(id, true,  filename, null, fileLn, fileCol, -1, -1, msg, "")
}

local function printError(filename, row, field, id, msg) {
  _printErr(id, true,  filename, row, field.fileLn, field.fileCol, field.tr, field.tc, msg, field.val)
}

local function printWarning(filename, row, field, id, msg) {
  _printErr(id, false, filename, row, field.fileLn, field.fileCol, field.tr, field.tc, msg, field.val)
}

local function arraySubstract(from, what) {
  local res = []
  foreach (v in from)
    if(what.indexof(v) == null)
      res.append(v)
  return res
}

local function reduceToDiff(arr1, arr2) {
  foreach (v in [].extend(arr1).extend(arr2))
    if (arr1.indexof(v) != null && arr2.indexof(v) != null)
      foreach (arr in [ arr1, arr2 ])
        arr.remove(arr.indexof(v))
}

local function readTextFileTo2DCharArray(fileName) {
  local lines = [ [] ]
  local fp = file(fileName, "rb")
  if (fp.len()) {
    local content = fp.readblob(fp.len())
    fp.close()
    local lineIdx = 0
    local line = lines[lineIdx]
    for (local i = 0; i < content.len(); i++) {
      line.append(format("%c", content.readn('c')))
      local lineLen = line.len()
      if (line?[lineLen - 2] == "\r" && line?[lineLen - 1] == "\n") {
        lines.append([])
        line = lines[++lineIdx]
      }
    }
    local lastLineIdx = lines.len() - 1
    if (lastLineIdx >= 0 && lines[lastLineIdx].len() == 0)
      lines.remove(lastLineIdx)
  }
  return lines
}

local utf8Idx = @(idx, charArr) utf8("".join(charArr.slice(0, idx)).replace("\r\n", "")).charCount()

/**
 * Parser for CSV (comma separated values) file format by RFC 4180 standard
 * (https://tools.ietf.org/html/rfc4180), with values separated by semicolon.
 * It treats all parsing errors is as fatal errors. Because our translators
 * uses spreadsheet software, which makes no errors, and all parsing errors
 * appear ONLY after editing the CSV files manually, it text editors.
 * The problem is that our in-game CSV parser is too "forgiving", it tries
 * to silently fix all errors, and sometimes it leads to unexpected results.
 */

local function readCsvFile(filePath, fn, shouldPrintWeakErrors = true) {
  local rows = []
  local cols = []
  local cell = []
  local cellL = 0
  local cellC = 0
  local headerColsCount = -1
  local isQuotedCell = false
  local lines = readTextFileTo2DCharArray(filePath)
  for (local l = 0; l < lines.len(); l++) {
    local line = lines[l]
    local lastEscapePos = -2
    for (local c = 0; c <= line.len(); c++) {
      local char = line?[c] ?? "\0"
      local charPrev = line?[c-1]
      local charNext = line?[c+1] ?? (char != "\0" ? "\0" : null)
      local isCR = char == "\r"
      local isLF = char == "\n"
      if (shouldPrintWeakErrors && ((isCR && charNext != "\n") || (isLF && charPrev != "\r")))
        printParsingError(fn, l, utf8Idx(c, line), "parser_crlf", "Wrong line ending, must be CR/LF.")
      local isNullTerm = c == line.len()
      if (shouldPrintWeakErrors && isQuotedCell && isNullTerm && lines?[l+1] == null)
        printParsingError(fn, l, utf8Idx(c, line), "parser_eof", "Unexpected end of file.")
      local isEscaped = isQuotedCell && char == "\"" && lastEscapePos == c - 1
      local isEscape  = isQuotedCell && !isEscaped && char == "\"" && charNext == "\""
      if (isEscape)
        lastEscapePos = c
      local isSeparator = !isQuotedCell && (char == ";" || isNullTerm)
      local isQuote = !isEscape && !isEscaped && char == "\""
      if (isQuote && !((!isQuotedCell && (c == 0 || charPrev == ";")) ||
        (isQuotedCell && [ ";", "\r", "\n", "\0" ].indexof(charNext) != null))) {
        local comment = c == 0 ? $" (probably missing quote in line {l})" : ""
        printParsingError(fn, l, utf8Idx(c, line), "parser_quote_break", $"Unexpected quote sign{comment}.")
        return rows // can't continue because table rows/cols order is broken.
      }
      if (isQuote)
        isQuotedCell = !isQuotedCell
      if (!isSeparator && !isQuote && !isEscape && !isNullTerm && !isCR && !(!isQuotedCell && isLF)) {
        if (cell.len() == 0) {
          cellL = l
          cellC = utf8Idx(c, line)
        }
        cell.append(char)
      }
      if (isSeparator) {
        cols.append({ fileLn = cellL, fileCol = cellC, tc = cols.len(), tr = rows.len(), val = "".join(cell) })
        cell = []
        if (isNullTerm) {
          local colsCount = cols.len()
          headerColsCount = headerColsCount == -1 ? colsCount : headerColsCount
          if (headerColsCount != -1 && colsCount > headerColsCount)
            printParsingError(fn, l, utf8Idx(c, line), "parser_columns",
              $"Wrong columns count ({colsCount}, must be {headerColsCount}).")
          rows.append(cols)
          cols = []
        }
      }
    }
  }
  return rows
}

/**
 * Writes data to disk as CSV file.
 */

local function writeCsvFile(rows, filePath) {
  local reIsFieldNumeric = regexp2(@"^\d+$")
  local fp = file(filePath, "wb")
  foreach(row in rows) {
    local line = "".concat(";".join(row.map(function(field) {
        local txt = field.val.replace("\n", "\r\n").replace("\"", "\"\"")
        return txt.len() != 0 && !reIsFieldNumeric.match(txt) ? $"\"{txt}\"" : ""
      })), "\r\n")
    foreach (char in line)
      fp.writen(char, 'c')
  }
  fp.close()
}

//-------------------------------------------------------------------------------------------------

/**
 * Localization texts technical syntax check. Checks translations for all kinds
 * of wrong usage of variable placeholders (C++ printf format specifiers,
 * Quirrel script string.subst variables, input device control shortcuts),
 * daGUI/daRG text formatting tags usage, language plural forms count,
 * in-game currency symbols usage, HTML entities, and the like.
 */

local reGetFormatSpecifiers = regexp2(@"(%(?:\d+)?(?:\.\d+)?[sdfFiouxXaAeEgGcp])")
local reGetQStringVarNames = regexp2(@"\{(\w+?)\}")
local reGetPluralForms = regexp2(@"\{(\w+?=[^\}]*?)\}")
local reGetShortcutIds = regexp2(@"\{{2}([\w=]+?)\}{2}")
local reGetDaguiTextTags = regexp2(@"(<\/?([^>\s]+?)?(?:=([^>]*?))?>)")
local reIsColorCssConst = regexp2(@"^@\w+?$")
local reIsColorHexCode = regexp2(@"^#[0-9a-fA-F]{6}$")
local reIsUrl = regexp2(@"^[a-z]+:\/\/[\w\-]+\.[\w\-:%&;\?\#\/\.=]+$")
local reIsUrlShort = regexp2(@"^[\w\-]+\.[\w\-:%&;\?\#\/\.=]+$")
local reIsCurlyVar = regexp2(@"^\{\w+?\}$")
local reIsLocalizationKey = regexp2(@"^[\w\-\+\.\/<>&:\s]+$")

local function checkLocalizationSyntax(csv, fn) {
  foreach (tr, row in csv) {
    if (row?[3].val.contains("SKIP_VALIDATION"))
      continue

    // Header row checks
    if (tr == 0) {
      if (row[0].val != "<ID|readonly|noverify>")
        printError(fn, null, row[0], "header_not_found", $"No header row in this file: \"{row[0].val}\".")
      else {
        local langName = (row?[1].val ?? "<>").slice(1, -1)
        curLangPluralFormsCount = getPluralFormsCount(langName)
        if (curLangPluralFormsCount == 0)
          printError(fn, null, row?[1] ?? row[0], "lang_unknown", $"Unknown language (need support?): <{langName}>.")
      }
    }
    if (tr == 0)
      continue

    totalRowsChecked++

    local key = row[0] // locId field
    local fields = [ // text fields
      row?[1] ?? key.__merge({ val = "", tc = 1 }),
      row?[2] ?? key.__merge({ val = "", tc = 2 }),
    ]
    local T = 0 // translation field id
    local O = 1 // original field id
    local FIDS  = [ T, O ] // field ids map
    local tran = fields[T] // translation field

    // Localization ID keys
    {
      if (key.val != "" && !reIsLocalizationKey.match(key.val))
        printError(fn, row, key, "loc_key", $"Forbidden characters in loc key (maybe cyrillic).")
    }

    // Skipping untranslated or empty rows
    if (fields[T].val == fields[O].val)
      continue

    // C++ printf format specifiers
    {
      local specsByField = FIDS.map(@(fid) reGetFormatSpecifiers.multiExtract("\\1", fields[fid].val))
      if (!isEqual(specsByField[T], specsByField[O])) {
        specsByField = specsByField.map(@(v) ",".join(v))
        local comment = specsByField[T] == "" ? $"lost {specsByField[O]}" : " vs ".join(specsByField)
        printError(fn, row, tran, "format_match", $"Printf format specifiers do not match ({comment}).")
      }
    }

    // Quirrel string.subst variable placeholders
    {
      local varNamesByField  = FIDS.map(@(fid) reGetQStringVarNames.multiExtract("\\1", fields[fid].val))
      local shortcutsByField = FIDS.map(@(fid) reGetShortcutIds.multiExtract("\\1", fields[fid].val))
      foreach (fid, _varNames in varNamesByField)
        varNamesByField[fid] = arraySubstract(varNamesByField[fid], shortcutsByField[fid])
      foreach (varName in varNamesByField[O])
        if (varNamesByField[T].indexof(varName) == null)
          printError(fn, row, tran, "var_lost", $"Variable value placeholder is lost: \{{varName}\}.")
      foreach (varName in varNamesByField[T])
        if (varNamesByField[O].indexof(varName) == null)
          printError(fn, row, tran, "var_irrelevant", $"Variable value placeholder is irrelevant: \{{varName}\}.")
    }

    // Dagor core localization language plural forms
    if (curLangPluralFormsCount != 0) {
      local pluralsByField = FIDS.map(@(fid) reGetPluralForms.multiExtract("\\1", fields[fid].val))
      local shortcutsByField = FIDS.map(@(fid) reGetShortcutIds.multiExtract("\\1", fields[fid].val))
      local expectedPluralForms = curLangPluralFormsCount > 1 ? curLangPluralFormsCount.tostring() : "1 or none"
      foreach (fid, _plurals in pluralsByField)
        pluralsByField[fid] = arraySubstract(pluralsByField[fid], shortcutsByField[fid])

      local pluralNames = FIDS.map(@(_fid) {})
      foreach (fid, expressions in pluralsByField) {
        foreach (expression in expressions) {
          local parts = expression.split("=")
          local varName = parts?[0] ?? ""
          local pluralForms = (parts?[1] ?? "").split("/")
          pluralNames[fid][varName] <- (pluralNames[fid]?[varName] ?? []).append(pluralForms)
        }
      }

      foreach (varName, valuesList in pluralNames[O]) {
        local isFound = varName in pluralNames[T]
        if (!isFound && curLangPluralFormsCount > 1)
          printError(fn, row, tran, "plural_lost", $"Plural forms set is lost: \{{varName}=...\} (expected {expectedPluralForms} forms for your language).")
        if (!isFound)
          continue
        local isUntranslatedReported = false
        foreach (pluralForms in pluralNames[T][varName]) {
          if (pluralForms.len() == curLangPluralFormsCount)
            continue
          if (!isUntranslatedReported) {
            local isUntranslated = false
            foreach (origPluralForms in valuesList)
              if (isEqual(pluralForms, origPluralForms))
                isUntranslated = true
            if (isUntranslated) {
              printError(fn, row, tran, "plural_not_translated",
                $"Plural forms not translated: \{{varName}=...\} (expected {expectedPluralForms} forms).")
              isUntranslatedReported = true
            }
          }
          if (isUntranslatedReported)
            continue
          if (curLangPluralFormsCount == 1)
            printWarning(fn, row, tran, "plural_excessive",
              $"Plural forms count is excessive: \{{varName}=...\} (found {pluralForms.len()}, expected {expectedPluralForms}).")
          else
            printError(fn, row, tran, "plural_count",
              $"Plural forms count is wrong: \{{varName}=...\} (found {pluralForms.len()}, expected {expectedPluralForms}).")
        }
      }
    }

    // daGUI bhvHint behavour control shortcuts placeholders
    {
      local shortcutsByField = FIDS.map(@(fid) reGetShortcutIds.multiExtract("\\1", fields[fid].val))
      foreach (shortcutId in shortcutsByField[O])
        if (shortcutsByField[T].indexof(shortcutId) == null)
          printError(fn, row, tran, "shortcut_lost", $"Shortcut placeholder is lost: \{\{{shortcutId}\}\}.")
      foreach (shortcutId in shortcutsByField[T])
        if (shortcutsByField[O].indexof(shortcutId) == null)
          printError(fn, row, tran, "shortcut_irrelevant", $"Shortcut placeholder is irrelevant: \{\{{shortcutId}\}\}.")
    }

    // daGUI/daRG textArea behavour text tags
    {
      local tagsByField = FIDS.map(@(fid) reGetDaguiTextTags.multiExtract("\\1", fields[fid].val))
      local levels = FIDS.map(@(_fid) {})
      local mixup  = FIDS.map(@(_fid) {})
      local syntax = FIDS.map(@(_fid) {})
      local counts = FIDS.map(@(_fid) {})
      local tprops = FIDS.map(@(_fid) {})

      foreach (fid, tags in tagsByField) {
        foreach (tag in tags) {
          local isOpening = !tag.startswith("</")
          local tagName = reGetDaguiTextTags.multiExtract("\\2", tag)?[0].tolower() ?? ""
          local tagProp  = reGetDaguiTextTags.multiExtract("\\3", tag)?[0] ?? ""
          if (tagName != "") {
            levels[fid][tagName] <- (levels[fid]?[tagName] ?? 0) + (isOpening ? 1 : -1)
            if (levels[fid][tagName] < 0)
              mixup[fid][tagName] <- true
            if (!isOpening && tag.indexof("="))
              syntax[fid][tagName] <- true
            counts[fid][tagName] <- (counts[fid]?[tagName] ?? 0) + 1
            if (isOpening) {
              tprops[fid][tagName] <- (tprops[fid]?[tagName] ?? {})
              tprops[fid][tagName][tagProp] <- (tprops[fid][tagName]?[tagProp] ?? 0) + 1
            }
          }
        }
      }

      foreach (tagName, count in counts[O]) {
        if ((syntax[T]?[tagName] ?? false) != false)
          printError(fn, row, tran, "tag_syntax", $"Tag has invalid syntax: </{tagName}=>.")
        if ((levels[T]?[tagName] ?? 0) != 0)
          printError(fn, row, tran, "tag_pair_unclosed", $"Tag pair unclosed: <{tagName}></{tagName}>.")
        else if ((mixup[T]?[tagName] ?? false) == true)
          printError(fn, row, tran, "tag_pair_mixup", $"Tag pair order mixed up: </{tagName}><{tagName}>.")
        else if (count != counts[T]?[tagName] && (levels[T]?[tagName] ?? 0) == 0)
          printError(fn, row, tran, "tag_count", $"Tags counts don't match: <{tagName}> ({counts[T]?[tagName] ?? 0} vs {count}).")
        else if (!isEqual(tprops[T]?[tagName], tprops[O]?[tagName]) && [ "url", "link" ].indexof(tagName) == null) {
          local propsByField = FIDS.map(@(fid) tprops[fid]?[tagName].topairs()
            .reduce(@(arr, v) arr.extend(array(v[1], v[0])), []).sort())
          reduceToDiff(propsByField[T], propsByField[O])
          propsByField = propsByField.map(@(p) ",".join(p))
          local comment = propsByField[T] == "" ? $"lost {propsByField[O]}" : " vs ".join(propsByField)
          printError(fn, row, tran, "tag_values", $"Tags values usage don't match: <{tagName}> ({comment}).")
        }
        else {
          if (tprops[T]?.color)
            foreach (prop, _c in tprops[T].color)
              if (!reIsColorCssConst.match(prop) && !reIsColorHexCode.match(prop) && !reIsCurlyVar.match(prop))
                printError(fn, row, tran, "tag_val_color", $"Tag <color> value is invalid: \"{prop}\".")
          if (tprops[T]?.url)
            foreach (prop, _c in tprops[T].url)
              if (!reIsUrl.match(prop) && !reIsCurlyVar.match(prop))
                printError(fn, row, tran, "tag_val_url", $"Tag <url> value is invalid: \"{prop}\".")
          if (tprops[T]?.link)
            foreach (prop, _c in tprops[T].link)
              if (!reIsUrl.match(prop) && !reIsUrlShort.match(prop) && !reIsCurlyVar.match(prop))
                printWarning(fn, row, tran, "tag_val_link", $"Tag <link> value is not URL: \"{prop}\".")
          if (tprops[T]?.b)
            foreach (prop, _c in tprops[T].b)
              if (prop != "")
                printError(fn, row, tran, "tag_val_p", $"Tag <b> value is invalid: \"{prop}\".")
        }
        if ([ "color", "url", "link", "b" ].indexof(tagName) == null)
          printError(fn, row, tran, "tag_unknown", $"Unknown tag: <{tagName}>.")
      }
    }

    // In-game currencies symbols
    {
      foreach (currency in currencySymbols.warthunder)
        if (fields[O].val.indexof(currency.symbol) != null && fields[T].val.indexof(currency.symbol) == null)
          printError(fn, row, tran, "currency_lost", $"Currency symbol is lost: '{currency.symbol}' ({currency.name}).")
    }

    // HTML entities
    {
      foreach (field in fields)
        foreach (entity in [ "&lt;", "&gt;", "&quot;", "&amp;", "&#39;", "&nbsp;" ])
          if (field.val.indexof(entity) != null)
            printError(fn, row, field, "html_entities", $"HTML entities found: {entity}.")
    }
  }
}

//-------------------------------------------------------------------------------------------------

local function validateLangs(cfg) {
  local rootDir     = cfg?.root ?? "."
  local scanDirs    = cfg?.scan ?? [ "" ]
  local excludeDirs = cfg?.exclude ?? []
  local isDebugTest = cfg?.isDebugTest ?? false
  verboseOutput     = cfg?.verbose ?? false

  local makeFullPath = @(r, sub) "/".join([ r, sub ], true)

  local filesList = []
  foreach (dir in scanDirs)
    filesList.extend(scan_folder({
      root = makeFullPath(rootDir, dir),
      files_suffix = "*.csv",
      recursive = true,
      vromfs = false,
      realfs = true
    }))
  filesList = filesList.sort().filter(function(p) {
    foreach (dir in excludeDirs)
      if (p.startswith("".concat(makeFullPath(rootDir, dir), "/")))
        return false
    return true
  })

  local scanDirsStr = ";".join(scanDirs)
  print($"Please read the Localization Guidelines to find out how to fix errors:\n")
  print($"https://wiki.warthunder.com/Localization_Guidelines\n")
  print("\n")
  print($"Localization files validation ({scanDirsStr}, {filesList.len()} files) ...\n")

  foreach (fileFullPath in filesList) {
    local fn = fileFullPath.replace($"{rootDir}/", "")
    local csv = readCsvFile(fileFullPath, fn)
    checkLocalizationSyntax(csv, fn)
  }

  local sumValues = @(tbl) tbl.values().reduce(@(sum, v) sum + v, 0)
  local totalErrors   = sumValues(totalStats.errors)
  local totalWarnings = sumValues(totalStats.warnings)

  if (totalErrors || totalWarnings) {
    local getSortedArray = @(tbl) tbl.topairs().sort(@(a, b) a[0] <=> b[0])

    print("\nStatistics\n")
    if (totalErrors)
      foreach (v in getSortedArray(totalStats.errors))
        print($"    errors {v[0]}: {v[1]}\n")
    if (totalWarnings)
      foreach (v in getSortedArray(totalStats.warnings))
        print($"    warnings {v[0]}: {v[1]}\n")
  }
  print($"Total rows checked: {totalRowsChecked}\n")

  if (isDebugTest) {
    local issues = totalErrors + totalWarnings
    local isTestPassed = issues == totalRowsChecked
    local result = isTestPassed ? "PASSED" : "FAILED"
    local sign   = isTestPassed ? "==" : "!="
    print($"Test {result} ({issues} issues {sign} {totalRowsChecked} rows)\n")
    totalErrors   = isTestPassed ? 0 : 1
    totalWarnings = 0
  }

  print(totalErrors == 0 && totalWarnings == 0
    ? $"Completed successfully\n"
    : $"Completed with {totalErrors} errors, {totalWarnings} warnings\n")
  print("\n")

  exit(totalErrors > 0 ? 1 : 0)
}


return {
  validateLangs = validateLangs
  readCsvFile   = readCsvFile
  writeCsvFile  = writeCsvFile
}
