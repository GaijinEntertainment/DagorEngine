from "string" import regexp, format
from "iostream" import blob
from "math" import clamp, log10, min
import "string" as string

let regexp2 = require_optional("regexp2")
let utf8 = require_optional("utf8")

//pairs list taken from http://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/nls/rbagslowtoupmaptable.htm
const CASE_PAIR_LOWER = "abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿāăąćĉċčďđēĕėęěĝğġģĥħĩīĭįıĳĵķĺļľŀłńņňŋōŏőœŕŗřśŝşšţťŧũūŭůűųŵŷźżžƃƅƈƌƒƙơƣƥƨƭưƴƶƹƽǆǉǌǎǐǒǔǖǘǚǜǟǡǣǥǧǩǫǭǯǳǵǻǽǿȁȃȅȇȉȋȍȏȑȓȕȗɓɔɗɘəɛɠɣɨɩɯɲɵʃʈʊʋʒάέήίαβγδεζηθικλμνξοπρσςτυφχψωϊϋόύώϣϥϧϩϫϭϯабвгдежзийклмнопрстуфхцчшщъыьэюяёђѓєѕіїјљњћќўџѡѣѥѧѩѫѭѯѱѳѵѷѹѻѽѿҁґғҕҗҙқҝҟҡңҥҧҩҫҭүұҳҵҷҹһҽҿӂӄӈӌӑӓӕӗәӛӝӟӡӣӥӧөӫӯӱӳӵӹաբգդեզէըթժիլխծկհձղճմյնշոչպջռսվտրցւփքօֆაბგდევზთიკლმნოპჟრსტუფქღყშჩცძწჭხჯჰჱჲჳჴჵḁḃḅḇḉḋḍḏḑḓḕḗḙḛḝḟḡḣḥḧḩḫḭḯḱḳḵḷḹḻḽḿṁṃṅṇṉṋṍṏṑṓṕṗṙṛṝṟṡṣṥṧṩṫṭṯṱṳṵṷṹṻṽṿẁẃẅẇẉẋẍẏẑẓẕạảấầẩẫậắằẳẵặẹẻẽếềểễệỉịọỏốồổỗộớờởỡợụủứừửữựỳỵỷỹἀἁἂἃἄἅἆἇἐἑἒἓἔἕἠἡἢἣἤἥἦἧἰἱἲἳἴἵἶἷὀὁὂὃὄὅὑὓὕὗὠὡὢὣὤὥὦὧᾀᾁᾂᾃᾄᾅᾆᾇᾐᾑᾒᾓᾔᾕᾖᾗᾠᾡᾢᾣᾤᾥᾦᾧᾰᾱῐῑῠῡⓐⓑⓒⓓⓔⓕⓖⓗⓘⓙⓚⓛⓜⓝⓞⓟⓠⓡⓢⓣⓤⓥⓦⓧⓨⓩａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
const CASE_PAIR_UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞŸĀĂĄĆĈĊČĎĐĒĔĖĘĚĜĞĠĢĤĦĨĪĬĮIĲĴĶĹĻĽĿŁŃŅŇŊŌŎŐŒŔŖŘŚŜŞŠŢŤŦŨŪŬŮŰŲŴŶŹŻŽƂƄƇƋƑƘƠƢƤƧƬƯƳƵƸƼǄǇǊǍǏǑǓǕǗǙǛǞǠǢǤǦǨǪǬǮǱǴǺǼǾȀȂȄȆȈȊȌȎȐȒȔȖƁƆƊƎƏƐƓƔƗƖƜƝƟƩƮƱƲƷΆΈΉΊΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΣΤΥΦΧΨΩΪΫΌΎΏϢϤϦϨϪϬϮАБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯЁЂЃЄЅІЇЈЉЊЋЌЎЏѠѢѤѦѨѪѬѮѰѲѴѶѸѺѼѾҀҐҒҔҖҘҚҜҞҠҢҤҦҨҪҬҮҰҲҴҶҸҺҼҾӁӃӇӋӐӒӔӖӘӚӜӞӠӢӤӦӨӪӮӰӲӴӸԱԲԳԴԵԶԷԸԹԺԻԼԽԾԿՀՁՂՃՄՅՆՇՈՉՊՋՌՍՎՏՐՑՒՓՔՕՖႠႡႢႣႤႥႦႧႨႩႪႫႬႭႮႯႰႱႲႳႴႵႶႷႸႹႺႻႼႽႾႿჀჁჂჃჄჅḀḂḄḆḈḊḌḎḐḒḔḖḘḚḜḞḠḢḤḦḨḪḬḮḰḲḴḶḸḺḼḾṀṂṄṆṈṊṌṎṐṒṔṖṘṚṜṞṠṢṤṦṨṪṬṮṰṲṴṶṸṺṼṾẀẂẄẆẈẊẌẎẐẒẔẠẢẤẦẨẪẬẮẰẲẴẶẸẺẼẾỀỂỄỆỈỊỌỎỐỒỔỖỘỚỜỞỠỢỤỦỨỪỬỮỰỲỴỶỸἈἉἊἋἌἍἎἏἘἙἚἛἜἝἨἩἪἫἬἭἮἯἸἹἺἻἼἽἾἿὈὉὊὋὌὍὙὛὝὟὨὩὪὫὬὭὮὯᾈᾉᾊᾋᾌᾍᾎᾏᾘᾙᾚᾛᾜᾝᾞᾟᾨᾩᾪᾫᾬᾭᾮᾯᾸᾹῘῙῨῩⒶⒷⒸⒹⒺⒻⒼⒽⒾⒿⓀⓁⓂⓃⓄⓅⓆⓇⓈⓉⓊⓋⓌⓍⓎⓏＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
local INVALID_INDEX = -1

local intRegExp = null
local possibleNotStrRegExp = null
local floatRegExp = null
local trimRegExp = null
local stripTagsConfig = null
local escapeConfig = null

/**
 * Joins array elements into a string with the glue string between each element.
 * This function is a reverse operation to g_string.split()
 * @param {string[]} pieces - The array of strings to join.
 * @param {string}   glue - glue string.
 * @return {string} - String containing all the array elements in the same order,
 *                    with the glue string between each element.
 */
// Reverse operation to split()
function implode(pieces = [], glue = "") {
  return glue.join(pieces, true)
}

/**
 * Joins array elements into a string with the glue string between each element.
 * Like implode(), but doesn't skip empty strings, so it is lossless
 * and safe for packing string data into a string, when empty sub-strings are important.
 * This function is a reverse operation to g_string.split()
 * @param {string[]} pieces - The array of strings to join.
 * @param {string}   glue - glue string.
 * @return {string} - String containing all the array elements in the same order,
 *                    with the glue string between each element.
 */
function join(pieces, glue="") {
  return glue.join(pieces)
}

/**
 * Splits a string into an array of sub-strings.
 * Like Squirrel split(), but doesn't skip empty strings, so it is lossless
 * and safe for extracting string data from a string, when empty sub-strings are important.
 * This function is a reverse operation to g_string.join()
 * @param {string} joined - The string to split.
 * @param {string} glue - glue string.
 * @return {string[]} - Array of sub-strings.
 */
function split(joined, glue, isIgnoreEmpty = false) {
  return (!isIgnoreEmpty) ? joined.split(glue)
            : joined.split(glue).filter(@(v) v!="")
}

const intre = @"^-?\d+$"
const floatre = @"^-?\d+\.?\d*([eE][-+]?\d{1,3})?$"
let notstringre = @"(^-?\d+$)|(^-?\d+\.?\d*([eE][-+]?\d{1,3})?$)|^(null|true|false)$"
if (regexp2 != null) {
  intRegExp = regexp2(intre)
  floatRegExp  = regexp2(floatre)
  trimRegExp = regexp2(@"^\s+|\s+$")
  possibleNotStrRegExp = regexp2(notstringre)
  stripTagsConfig = [
    {
      re2 = regexp2("~")
      repl = "~~"
    }
    {
      re2 = regexp2("\"")
      repl = "~\""
    }
    {
      re2 = regexp2("\r")
      repl = "~r"
    }
    {
      re2 = regexp2("\n")
      repl = "~n"
    }
    {
      re2 = regexp2("\'")
      repl = "~\'"
    }
  ]
  escapeConfig = [
    { re2 = regexp2(@"\\"), repl = @"\\\\" }
    { re2 = regexp2(@""""), repl = @"\\""" }
    { re2 = regexp2(@"\n"), repl = @"\\n"  }
    { re2 = regexp2(@"\r"), repl = @"\\r"  }
  ]
  for (local ch = 0; ch < 32; ch++)
    escapeConfig.append({
      re2 = regexp2(format(@"\x%02X", ch))
      repl = format(@"\\u%04X", ch)
    })
}
else if (regexp != null) {
  intRegExp = regexp(intre)
  possibleNotStrRegExp = regexp(notstringre)
  floatRegExp  = regexp(floatre)
  trimRegExp = regexp(@"^(\s+)|(\s+)$")
  stripTagsConfig = [
    {
      re2 = regexp(@"~")
      repl = "~~"
    }
    {
      re2 = regexp("\"")
      repl = "~\""
    }
    {
      re2 = regexp(@"\r")
      repl = "~r"
    }
    {
      re2 = regexp(@"\n")
      repl = "~n"
    }
    {
      re2 = regexp(@"\'")
      repl = "~\'"
    }
  ]
  escapeConfig = [
    { re2 = regexp(@"\\"), repl = @"\\\\" }
    { re2 = regexp(@""""), repl = @"\\""" }
    { re2 = regexp(@"\n"), repl = @"\\n"  }
    { re2 = regexp(@"\r"), repl = @"\\r"  }
  ]
  for (local ch = 0; ch < 32; ch++)
    escapeConfig.append({
      re2 = regexp(format(@"\x%02X", ch))
      repl = format(@"\\u%04X", ch)
    })
}

function [pure] isStringObviousString(str) {
  if (possibleNotStrRegExp != null)
    return !possibleNotStrRegExp.match(str)
  return false
}

let defTostringParams = freeze({
  maxdeeplevel = 4
  compact=true
  tostringfunc= {
    compare=@(_val) false
    tostring=@(val) val.tostring()
  }
  separator = " "
  indentOnNewline = "  "
  newline="\n"
  splitlines = true
  showArrIdx=false
})

function func2str_compact(func) {
  local { native, name } = func.getfuncinfos()
  return native ? $"(nativefunc): {name}"
    : name.slice(0,1) == "(" ? "@()" : $"{name}()"
}

function func2str(func, p={}) {
  local compact = p?.compact ?? false
  local showsrc = p?.showsrc ?? false
  local showparams = p?.showparams ?? compact
  local showdefparams = p?.showdefparams ?? compact
  local tostr_func = p?.tostr_func ?? @(v) $"{v}"

  local out = []
  local info = func.getfuncinfos()

  if (!info.native) {
    local defparams = info?.defparams ?? []
    local params = info.parameters.slice(1)
    local reqparams = params.slice(0, params.len() - defparams.len())
    local optparams = params.slice(reqparams.len())
    local params_str = []
    if (params.len()>0) {
      if (reqparams.len()>0)
        params_str.append(", ".join(reqparams))
      if (optparams.len()>0) {
        foreach (i, op in optparams) {
          params_str.append(op)
          if (showdefparams)
            params_str.append(" = ", tostr_func(defparams?[i] ?? ""))
          if (i+1 < optparams.len())
            params_str.append(", ")
        }
      }
    }
    local fname = $"{info.name}"
    if (fname.slice(0,1)=="(")
      fname = "@"
    if (showsrc)
      out.append("(func): ", (info?.src ?? ""), " ")
    out.append(fname, "(")
    if (!showparams)
      out.extend(params_str)
    out.append(")")
  } else if (info.native) {
    out.append("(nativefunc): ", info.name)

  } else {
    out.append(func.tostring())
  }
  return "".join(out)
}

let simple_types = ["string", "float", "bool", "integer", "null", "userdata", "thread"]
  .reduce(@(res, v) res.$rawset(v, true), {})
let function_types = ["function", "generator"]
  .reduce(@(res, v) res.$rawset(v, true), {})

function tostring_any(input, tostringfunc=null, compact=true) {
  local typ = type(input)
  if (tostringfunc!=null) {
    if (type(tostringfunc) == "table")
      tostringfunc = [tostringfunc]
    if (type(tostringfunc) == "array") {
      foreach (tf in tostringfunc){
        if (tf?.compare != null && tf.compare(input)){
          return tf.tostring(input)
        }
      }
    }
  }
  else if (typ in function_types){
    return compact ? func2str_compact(input) : func2str(input, { compact })
  }
  else if (typ == "string"){
    if (input=="")
      return "\"\""
    if (!compact || !isStringObviousString(input))
      return $"\"{input}\""
    return input
  }
  else if (typ == "null"){
    return "null"
  }
  else if (typ == "float"){
    local r = input.tostring()
    if (compact || input != input.tointeger().tofloat())
      return r
    return r.contains(".") || r.contains("e") ? r : $"{r}.0"
  }
  else if (typ=="instance"){
    return $"\"{input.tostring()}\""
  }
  else if (typ == "userdata"){
    return "#USERDATA#"
  }
  else if (typ == "weakreference"){
    return "#WEAKREF#"
  }
  if (typ=="thread") {
    return $"thread: {input.getstatus()}"
  }
  return input.tostring()
}
let FOO = {}
function tableLen(t){
  return FOO.len.call(t)
}

let table_types = ["table","class","instance"]
  .reduce(@(res, v) res.$rawset(v, true), {})
let openSymByType = {
  ["array"] = "[",
  ["class"] = "class {",
  ["instance"] = "instance {",
}

let openSym = @(value) openSymByType?[type(value)] ?? "{"
let closeSym = @(value) type(value) == "array" ? "]" : "}"

function tostring_r(inp, params=defTostringParams) {
  local newline = params?.newline ?? defTostringParams.newline
  local maxdeeplevel = params?.maxdeeplevel ?? defTostringParams.maxdeeplevel
  local separator = params?.separator ?? defTostringParams.separator
  local showArrIdx = params?.showArrIdx ?? defTostringParams.showArrIdx
  local tostringfunc = params?.tostringfunc
  local indentOnNewline = params?.indentOnNewline ?? defTostringParams.indentOnNewline
  local splitlines = params?.splitlines ?? defTostringParams.splitlines
  local compact = params?.compact ?? defTostringParams.compact

  function tostringLeaf(val) {
    local typ =type(val)
    if (tostringfunc!=null) {
      if (type(tostringfunc) == "table")
        tostringfunc = [tostringfunc]
      foreach (tf in tostringfunc)
        if (tf.compare(val))
          return [true, tf.tostring(val)]
    }

    if (typ in simple_types || typ in function_types)
      return [true, tostring_any(val, null, compact)]
    if (typ == "table" && tableLen(val) == 0)
      return [true, "{}"]
    if (typ == "array" && val.len() == 0)
      return [true, "[]"]
    if (typ == "instance") {
      let str = val?.tostring()
      return [str != null && str.indexof("(instance : 0x") != 0, str]
    }
    return [false, null]
  }

  local arrSep = separator
  if (!splitlines) {
    newline = " "
    indentOnNewline = ""
  }
  let stream = blob()
  function write_to_string(input, indent, curdeeplevel, arrayElem = false, sep = newline, arrInd=null) {
    if (arrInd==null)
      arrInd=indent
    local li = 0
    local maxind = -1
    try {
      if (type(input?.len)=="function") {
        let info = input.len.getfuncinfos()
        if (!info.native && info.parameters.len()==1)
          maxind = input.len()-1
        else if (info.native && info.typecheck.len()==1 && (info.typecheck[0]==32|| info.typecheck[0]==64)) //this is instance
          maxind = input.len()-1
      }
    }
    catch(e){}
    foreach (key, value in input) {
      local typ = type(value)
      local isArray = typ=="array"
      let [isProcessed, resultStr] = tostringLeaf(value)
      if (isProcessed) {
        if (!arrayElem) {
          stream.writestring(sep)
          stream.writestring(indent)
          stream.writestring(tostring_any(key, null, compact))
          stream.writestring(" = ")
        }
        stream.writestring(resultStr)
        if (arrayElem && li != maxind)
          stream.writestring(sep)
      }
      else if (maxdeeplevel != null && curdeeplevel == maxdeeplevel) {
        local brOp = openSym(value)
        local brCl = closeSym(value)
        if (!arrayElem) {
          stream.writestring(newline)
          stream.writestring(indent)
          stream.writestring(tostring_any(key, null, compact))
          stream.writestring(" = ")
        }
        else if (arrayElem && showArrIdx) {
          stream.writestring(tostring_any(key, null, compact))
          stream.writestring(" = ")
        }
        stream.writestring(brOp)
        stream.writestring("...")
        stream.writestring(brCl)
      }
      else if (isArray && !showArrIdx) {
        if (!arrayElem) {
          stream.writestring(newline)
          stream.writestring(indent)
          stream.writestring(tostring_any(key, null, compact))
          stream.writestring(" = ")
        }
        stream.writestring("[")
        write_to_string(value, $"{indent}{indentOnNewline}", curdeeplevel+1, true, arrSep, indent) //warning disable: -param-pos
        stream.writestring("]")
        if (arrayElem && li!=maxind)
          stream.writestring(sep)
      }
      else if (typ in table_types || (isArray && showArrIdx )) {
        local brOp = openSym(value)
        local brCl = closeSym(value)
        stream.writestring(newline)
        stream.writestring(indent)
        if (!arrayElem) {
          stream.writestring(tostring_any(key,null, compact)," = ")
        }
        stream.writestring(brOp)
        write_to_string(value, $"{indent}{indentOnNewline}", curdeeplevel+1)
        stream.writestring(newline)
        stream.writestring(indent)
        stream.writestring(brCl)
        if (arrayElem && li==maxind ){
          stream.writestring(newline)
          stream.writestring(arrInd)
        }
        else if (arrayElem && li < maxind && type(input[li+1]) != "table"){
          stream.writestring(newline)
          stream.writestring(indent)
        }
      }
      li += 1
    }
  }
  write_to_string([inp], "", 0,true)
  return stream.as_string()
}


/**
 * Retrieves a substring from the string. The substring starts and ends at a specified indexes.
 * Like Python operator slice.
 * @param {string}  str - Input string.
 * @param {integer} start - Substring start index. If it is negative, the returned string will
 *                          start at the start'th character from the end of input string.
 * @param {integer} [end] - Substring end index.  If it is negative, the returned string will
 *                          end at the end'th character from the end of input string.
 * @return {string} - substring, or on error - part of substring or empty string.
 */
function slice(str, start = 0, end = null) {
  str = str ?? ""
  return str.slice(start, end ?? str.len())
}

/**
 * Retrieves a substring from the string. The substring starts at a specified index
 * and has a specified length.
 * Like PHP function substr().
 * @param {string}  str - Input string.
 * @param {integer} start - Substring start index. If it is negative, the returned string will
 *                          start at the start'th character from the end of input string.
 * @param {integer} [length] - Substring length. If it is negative, the returned string will
 *                             end at the end'th character from the end of input string.
 * @return {string} - substring, or on error - part of substring or empty string.
 */
function substring(str, start = 0, length = null) {
  local end = length
  if (length != null && length >= 0) {
    str = str ?? ""
    local total = str.len()
    if (start < 0)
      start += total
    start = clamp(start, 0, total)
    end = start + length
  }
  return slice(str, start, end)
}

/**
 * Determines whether the beginning of the string matches a specified substring.
 * Like C# function String.StartsWith().
 * @param {string}  str - Input string.
 * @param {string}  value - Matching substring.
 * @return {boolean}
 */
function startsWith(str, value) {
  str = str ?? ""
  value = value ?? ""
  return str.startswith(value)
}

/**
 * Determines whether the end of the string matches the specified substring.
 * Like C# function String.EndsWith().
 * @param {string}  str - Input string.
 * @param {string}  value - Matching substring.
 * @return {boolean}
 */
function endsWith(str, value) {
  str = str ?? ""
  value = value ?? ""
  return str.endswith(value)
}

/**
 * Reports the index of the first occurrence in the string of a specified substring.
 * Like C# function String.IndexOf().
 * @param {string}  str - Input string.
 * @param {string}  value - Searching substring.
 * @param {integer} [startIndex=0] - Search start index.
 * @return {integer} - index, or -1 if not found.
 */
function indexOf(str, value, startIndex = 0) {
  str = str ?? ""
  value = value ?? ""
  local idx = str.indexof(value, startIndex)
  return idx ?? INVALID_INDEX
}

/**
 * Reports the index of the last occurrence in the string of a specified substring.
 * Like C# function String.LastIndexOf().
 * @param {string}  str - Input string.
 * @param {string}  value - Searching substring.
 * @param {integer} [startIndex=0] - Search start index.
 * @return {integer} - index, or -1 if not found.
 */
function lastIndexOf(str, value, startIndex = 0) {
  str = str ?? ""
  value = value ?? ""
  local idx = INVALID_INDEX
  local curIdx = startIndex - 1
  local length = str.len()
  while (curIdx < length - 1) {
    curIdx = str.indexof(value, curIdx + 1)
    if (curIdx == null)
      break
    idx = curIdx
  }
  return idx
}

/**
 * Reports the index of the first occurrence in the string of any substring in a specified array.
 * Like C# function String.IndexOfAny().
 * @param {string}   str - Input string.
 * @param {string[]} anyOf - Array of substrings to search for.
 * @param {integer}  [startIndex=0] - Search start index.
 * @return {integer} - index, or -1 if not found.
 */
function indexOfAny(str, anyOf, startIndex = 0) {
  str = str ?? ""
  anyOf = anyOf ?? [ "" ]
  local idx = INVALID_INDEX
  foreach (value in anyOf) {
    local curIdx = indexOf(str, value, startIndex)
    if (curIdx != INVALID_INDEX && (idx == INVALID_INDEX || curIdx < idx))
      idx = curIdx
  }
  return idx
}

/**
 * Reports the index of the last occurrence in the string of any substring in a specified array.
 * Like C# function String.LastIndexOfAny().
 * @param {string}   str - Input string.
 * @param {string[]} anyOf - Array of substrings to search for.
 * @param {integer}  [startIndex=0] - Search start index.
 * @return {integer} - index, or -1 if not found.
 */
function lastIndexOfAny(str, anyOf, startIndex = 0) {
  str = str ?? ""
  anyOf = anyOf ?? [ "" ]
  local idx = INVALID_INDEX
  foreach (value in anyOf) {
    local curIdx = lastIndexOf(str, value, startIndex)
    if (curIdx != INVALID_INDEX && (idx == INVALID_INDEX || curIdx > idx))
      idx = curIdx
  }
  return idx
}

//returns the number of entries of @substr in @str.
function countSubstrings(str, substr) {
  local res = -1
  local findex = -1
  for(res; findex != 0; res++) {
    findex = str.indexof(substr, ++findex)
  }
  return res
}

function capitalize(str) {
  return "".concat(str.slice(0, 1).toupper(), str.slice(1))
}

function replace(str, from, to) {
  return (str ?? "").replace(from, to)
}

/**
 * Strips leading and trailing whitespace characters.
 * Like Java function trim().
 * @param {string} str - Input string.
 * @return {string} - String without whitespace chars.
 */
function trim(str) {
  str = str ?? ""
  return trimRegExp ? trimRegExp.replace("", str) : str // TODO: Compare with str.strip()
}

/*
  getroottable()["rfsUnitTest"] <- function() {
    local resArr = []
    local testValArray = [0.0, 0.000024325, 0.0001, 0.5, 0.9999, 1.0, 5.126, 12.0, 120.057, 123.0, 6548.0, 72356.0, 1234567.0]
    for (local presize = 1e+6; presize >= 1e-10; presize *= 0.1) {
      local positive = ", ".join(testValArray.map(@(val) floatToStringRounded(val * presize, presize)))
      local negative = ", ".join(testValArray.map(@(val) floatToStringRounded(-val * presize, presize)))
      resArr.append($"presize {presize} -> {positive}, {negative}")
    }
    return "\n".join(resArr)
  }
//presize 1e+06 -> 1000000, 12000000, 123000000, 6547000000, 72356000000, 120000000, 4300000000, 1234567000000
//presize 0.001 -> 0.001, 0.012, 0.123, 6.548, 72.356, 0.120, 4.300, 1234.567
//presize 1e-10 -> 0.0000000001, 0.0000000012, 0.0000000123, 0.0000006548, 0.0000072356, 0.0000000120, 0.0000004300, 0.0001234567'
*/

function [pure] floatToStringRounded(value, presize) {
  if (presize >= 1) {
    local res = (value / presize + (value < 0 ? -0.5 : 0.5)).tointeger()
    return res == 0 ? "0" : "".join([res].extend(array(log10(presize).tointeger(), "0")))
  }
  return format("%.{0}f".subst(-log10(presize).tointeger()), value)
}

function [pure] isStringInteger(str) {
  if (type(str) == "integer")
    return true
  if (type(str) != "string")
    return false
  if (intRegExp != null)
    return intRegExp.match(str)

  if (str.startswith("-"))
    str = str.slice(1)
  if (str == "")
    return false
  for (local i = 0; i < str.len(); i++)
    if (str[i] < '0' || str[i] > '9')
      return false
  return true
}

function [pure] isStringFloat(str, separator=".") {
  if (type(str) == "integer" || type(str) == "float")
    return true
  if (type(str) != "string")
    return false
  if (floatRegExp != null && separator == ".")
    return floatRegExp.match(str)

  if (str.startswith("-"))
    str = str.slice(1)
  local numList = split(str, separator)
  local numListLen = numList.len()
  if (numListLen > 2 || numList[0] == "")
    return false
  if (numListLen == 2 && numList[1] == "")
    numList[1] = "0"
  local lastSeg = numList[numListLen - 1]
  local expMark = lastSeg.indexof("e") != null ? "e"
    : lastSeg.indexof("E") != null ? "E"
    : null
  if (expMark) {
    local eList = split(lastSeg, expMark)
    if (eList.len() != 2)
      return false
    if (numListLen == 2 && eList.len() == 2 && eList[0] == "")
      eList[0] = "0"
    if (eList[1].startswith("-") || eList[1].startswith("+"))
      eList[1] = eList[1].slice(1)
    local expDigits = eList[1].len()
    if (expDigits < 1 || expDigits > 3)
      return false
    numList[numListLen - 1] = eList[0]
    numList.append(eList[1])
  }
  foreach (s in numList) {
    if (s == "")
      return false
    for (local i = 0; i < s.len(); i++)
      if (s[i] < '0' || s[i] > '9')
        return false
  }
  return true
}

function [pure] toIntegerSafe(str, defValue = 0, needAssert = true) {
  if (type(str) == "string")
    str = str.strip()
  if (isStringInteger(str))
    return str.tointeger()
  if (needAssert)
    assert(false, @() $"can't convert '{str}' to integer")
  return defValue
}


local utf8ToUpper
local utf8ToLower
local utf8Capitalize
local utf8CapitalizeWords

if (utf8 != null) {
  utf8ToUpper = function [pure] utf8ToUpperImpl(str) {
    return utf8(str).strtr(CASE_PAIR_LOWER, CASE_PAIR_UPPER)
  }

  utf8ToLower = function [pure] utf8ToLowerImpl(str) {
    return utf8(str).strtr(CASE_PAIR_UPPER, CASE_PAIR_LOWER)
  }

  utf8Capitalize = function [pure] utf8CapitalizeImpl(str) {
    let utf8Str = utf8(str)
    return "".concat(utf8(utf8Str.slice(0, 1)).strtr(CASE_PAIR_LOWER, CASE_PAIR_UPPER), utf8Str.slice(1))
  }
  utf8CapitalizeWords = @[pure] utf8CapitalizeWordsImpl(str) " ".join(str.split(" ").map(utf8Capitalize))
}
else {
  function noUtf8Module(...) { assert("No 'utf8' module") }
  utf8ToUpper = noUtf8Module
  utf8ToLower = noUtf8Module
  utf8Capitalize = noUtf8Module
}

function [pure] intToUtf8Char(c) {
  if (c <= 0x7F)
    return c.tochar()
  if (c <= 0x7FF)
    return "".concat((0xC0 + (c>>6)).tochar(), (0x80 + (c & 0x3F)).tochar())
  if (c <= 0xFFFF)
    return "".concat((0xE0 + (c>>12)).tochar(), (0x80 + ((c>>6) & 0x3F)).tochar(), (0x80 + (c & 0x3F)).tochar())
  if (c <= 0x10FFFF)
    return "".concat((0xF0 + (c>>12)).tochar(), (0x80 + ((c>>12) & 0x3F)).tochar(), (0x80 + ((c>>6) & 0x3F)).tochar(), (0x80 + (c & 0x3F)).tochar())
  return ""
}

let firstOctet = [
  { ofs = 0, mask = 0x7F }
  { ofs = 0xC0, mask = 0x1F }
  { ofs = 0xE0, mask = 0x0F }
  { ofs = 0xF0, mask = 0x07 }
]
let nextOctet = { ofs = 0x80, mask = 0x3F }

function [pure] utf8CharToInt(str) {
  let list = []
  foreach (i in str)
    list.append(i)

  local res = 0
  for (local i = list.len() - 1; i >= 0; i--) {
    let { ofs = null, mask = null } = i > 0 ? nextOctet
      : firstOctet?[list.len() - 1]
    if (ofs == null)
      return 0 //too many chars
    let val = list[i]
    if ((val & ~mask) != ofs)
      return 0 //bad code
    res += (val & mask) << ((list.len() - 1 - i) * 6)
  }

  return res
}

function [pure] hexStringToInt(hexString) {
  // Does the string start with '0x'? If so, remove it
  if (hexString.len() >= 2 && hexString.slice(0, 2) == "0x")
    hexString = hexString.slice(2)

  // Get the integer value of the remaining string
  local res = 0
  foreach (character in hexString) {
    local nibble = character - '0'
    if (nibble > 9)
      nibble = ((nibble & 0x1F) - 7)
    res = (res << 4) + nibble
  }

  return res
}

//Return defValue when incorrect prefix
function [pure] cutPrefix(id, prefix, defValue = null) {
  if (!id)
    return defValue

  local pLen = prefix.len()
  if ((id.len() >= pLen) && (id.slice(0, pLen) == prefix))
    return id.slice(pLen)
  return defValue
}

function [pure] cutPostfix(id, postfix, defValue = null) {
  if (!id)
    return defValue

  local pLen = postfix.len()
  local idLen = id.len()
  if ((idLen >= pLen) && (id.slice(-1 * pLen) == postfix))
    return id.slice(0, idLen - pLen)
  return defValue
}

function [pure] intToStrWithDelimiter(value, delimiter = " ", charsAmount = 3) {
  local res = value.tointeger().tostring()
  local negativeSignCorrection = value < 0 ? 1 : 0
  local idx = res.len()
  while (idx > charsAmount + negativeSignCorrection) {
    idx -= charsAmount
    res = delimiter.concat(res.slice(0, idx), res.slice(idx))
  }
  return res
}


function [pure] stripTags(str) {
  if (!str || !str.len())
    return ""
  if (stripTagsConfig == null)
    assert(stripTagsConfig != null, "stripTags is not working without regexp")
  foreach(test in stripTagsConfig)
    str = test.re2.replace(test.repl, str)
  return str
}

function [pure] escape(str) {
  if (type(str) != "string") {
    assert(false, @() $"wrong escape param type: {type(str)}")
    return ""
  }
  foreach(test in escapeConfig)
    str = test.re2.replace(test.repl, str)
  return str
}

function pprint(...){
  //most of this code should be part of tostring_r probably - at least part of braking long lines
  function findlast(str, substr, startidx=0){
    local ret = null
    for(local i=startidx; i<str.len(); i++) {
      local k = str.indexof(substr, i)
      if (k!=null) {
        i = k
        ret = k
      }
      else
        break;
    }
    return ret
  }
  if (vargv.len()<=1)
    print("".concat(tostring_r(vargv[0]),"\n"))
  else {
    local a = vargv.map(@(i) tostring_r(i))
    local res = ""
    local prev_val_newline = false
    local len = 0
    local maxlen = 50
    foreach(k,i in a) {
      local l = findlast(i,"\n")
      if (l!=null)
        len = len + i.len()-l
      else
        len = len+i.len()
      if (k==0)
        res = i
      else if (prev_val_newline && len<maxlen)
        res = " ".concat(res.slice(0,-1), i)
      else if (len>=maxlen){
        res = "\n ".concat(res, i)
        len = i.len()
      }
      else
        res = " ".concat(res, i)

      prev_val_newline = i.slice(-1) == "\n" && len < maxlen
    }
    print(res)
    print("\n")
  }
}

function validateEmail(no_dump_email) {
  if (type(no_dump_email) != "string")
    return false

  local str = split(no_dump_email,"@")
  if (str.len() < 2)
    return false

  local locpart = str[0]
  if (str.len() > 2)
    locpart = "@".join(str.slice(0,-1))
  if (locpart.len() > 64)
    return false

  local dompart = str[str.len()-1]
  if (dompart.len() > 253 || dompart.len() < 4) //RFC + domain should be at least x.xx
    return false

  local quotes = locpart.indexof("\"")
  if (quotes && quotes != 0)
    return false //quotes only at the begining

  if (quotes == null && locpart.indexof("@")!=null)
    return false //no @ without quotes

  if (dompart.indexof(".") == null || dompart.indexof(".") > dompart.len() - 3) // warning disable: -func-can-return-null -potentially-nulled-ops
    return false  //too short first level domain or no periods

  return true
}

function clearBorderSymbols(value, symList = [" "]) {
  while(value != "" && symList.indexof(value.slice(0,1)) != null)
    value = value.slice(1)
  while(value!="" && symList.indexof(value.slice(-1)) != null)
    value = value.slice(0, -1)
  return value
}

function clearBorderSymbolsMultiline(str) {
  return clearBorderSymbols(str, [" ", 0x0A.tochar(), 0x0D.tochar()])
}

function [pure] splitStringBySize(str, maxSize) {
  if (maxSize <= 0) {
    assert(false, $"maxSize = {maxSize}")
    return [str]
  }
  let result = []
  local start = 0
  let l = str.len()
  while (start < l) {
    let pieceSize = min(l - start, maxSize)
    result.append(str.slice(start, start + pieceSize))
    start += pieceSize
  }
  return result
}

function obj2stringarray(obj, curpath = null){
  let res = []
  curpath = curpath ?? []
  let t = type(obj)
  if (t=="array") {
    foreach(i in obj)
      res.extend(obj2stringarray(i, [].extend(curpath)))
  }
  else if (t=="table") {
    foreach( k, v in obj)
      res.extend(obj2stringarray(v, [].extend(curpath).append(k)))
  }
  else {
    res.append($"{"/".join(curpath)}={tostring_any(obj, null, false)}") // ?? add types?
  }
  return res
}

return freeze(string.__merge({
  INVALID_INDEX
  CASE_PAIR_LOWER
  CASE_PAIR_UPPER
  slice
  substring
  startsWith
  endsWith
  indexOf
  lastIndexOf
  indexOfAny
  lastIndexOfAny
  countSubstrings
  implode
  join
  split
  replace
  trim
  floatToStringRounded
  isStringInteger
  isStringFloat
  isStringLatin = @(str) regexp(@"[a-z,A-Z]*").match(str)
  intToUtf8Char
  utf8CharToInt
  capitalize
  utf8Capitalize
  utf8ToUpper
  utf8ToLower
  utf8CapitalizeWords
  hexStringToInt
  cutPrefix
  cutPostfix
  intToStrWithDelimiter
  stripTags
  escape
  tostring_any
  tostring_r
  pprint
  validateEmail
  clearBorderSymbols
  clearBorderSymbolsMultiline

  toIntegerSafe
  splitStringBySize
  obj2stringarray
}))
