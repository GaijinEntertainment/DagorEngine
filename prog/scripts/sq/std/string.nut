from "string" import regexp, format
from "iostream" import blob
from "math" import clamp, log10, min
from "types" import Table, Array, String, Function, Integer, Float
import "string" as string
import "utf8" as utf8

//pairs list taken from http://www.ibm.com/support/knowledgecenter/ssw_ibm_i_72/nls/rbagslowtoupmaptable.htm
const CASE_PAIR_LOWER = "abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿāăąćĉċčďđēĕėęěĝğġģĥħĩīĭįıĳĵķĺļľŀłńņňŋōŏőœŕŗřśŝşšţťŧũūŭůűųŵŷźżžƃƅƈƌƒƙơƣƥƨƭưƴƶƹƽǆǉǌǎǐǒǔǖǘǚǜǟǡǣǥǧǩǫǭǯǳǵǻǽǿȁȃȅȇȉȋȍȏȑȓȕȗɓɔɗɘəɛɠɣɨɩɯɲɵʃʈʊʋʒάέήίαβγδεζηθικλμνξοπρσςτυφχψωϊϋόύώϣϥϧϩϫϭϯабвгдежзийклмнопрстуфхцчшщъыьэюяёђѓєѕіїјљњћќўџѡѣѥѧѩѫѭѯѱѳѵѷѹѻѽѿҁґғҕҗҙқҝҟҡңҥҧҩҫҭүұҳҵҷҹһҽҿӂӄӈӌӑӓӕӗәӛӝӟӡӣӥӧөӫӯӱӳӵӹաբգդեզէըթժիլխծկհձղճմյնշոչպջռսվտրցւփքօֆაბგდევზთიკლმნოპჟრსტუფქღყშჩცძწჭხჯჰჱჲჳჴჵḁḃḅḇḉḋḍḏḑḓḕḗḙḛḝḟḡḣḥḧḩḫḭḯḱḳḵḷḹḻḽḿṁṃṅṇṉṋṍṏṑṓṕṗṙṛṝṟṡṣṥṧṩṫṭṯṱṳṵṷṹṻṽṿẁẃẅẇẉẋẍẏẑẓẕạảấầẩẫậắằẳẵặẹẻẽếềểễệỉịọỏốồổỗộớờởỡợụủứừửữựỳỵỷỹἀἁἂἃἄἅἆἇἐἑἒἓἔἕἠἡἢἣἤἥἦἧἰἱἲἳἴἵἶἷὀὁὂὃὄὅὑὓὕὗὠὡὢὣὤὥὦὧᾀᾁᾂᾃᾄᾅᾆᾇᾐᾑᾒᾓᾔᾕᾖᾗᾠᾡᾢᾣᾤᾥᾦᾧᾰᾱῐῑῠῡⓐⓑⓒⓓⓔⓕⓖⓗⓘⓙⓚⓛⓜⓝⓞⓟⓠⓡⓢⓣⓤⓥⓦⓧⓨⓩａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
const CASE_PAIR_UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞŸĀĂĄĆĈĊČĎĐĒĔĖĘĚĜĞĠĢĤĦĨĪĬĮIĲĴĶĹĻĽĿŁŃŅŇŊŌŎŐŒŔŖŘŚŜŞŠŢŤŦŨŪŬŮŰŲŴŶŹŻŽƂƄƇƋƑƘƠƢƤƧƬƯƳƵƸƼǄǇǊǍǏǑǓǕǗǙǛǞǠǢǤǦǨǪǬǮǱǴǺǼǾȀȂȄȆȈȊȌȎȐȒȔȖƁƆƊƎƏƐƓƔƗƖƜƝƟƩƮƱƲƷΆΈΉΊΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΣΤΥΦΧΨΩΪΫΌΎΏϢϤϦϨϪϬϮАБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯЁЂЃЄЅІЇЈЉЊЋЌЎЏѠѢѤѦѨѪѬѮѰѲѴѶѸѺѼѾҀҐҒҔҖҘҚҜҞҠҢҤҦҨҪҬҮҰҲҴҶҸҺҼҾӁӃӇӋӐӒӔӖӘӚӜӞӠӢӤӦӨӪӮӰӲӴӸԱԲԳԴԵԶԷԸԹԺԻԼԽԾԿՀՁՂՃՄՅՆՇՈՉՊՋՌՍՎՏՐՑՒՓՔՕՖႠႡႢႣႤႥႦႧႨႩႪႫႬႭႮႯႰႱႲႳႴႵႶႷႸႹႺႻႼႽႾႿჀჁჂჃჄჅḀḂḄḆḈḊḌḎḐḒḔḖḘḚḜḞḠḢḤḦḨḪḬḮḰḲḴḶḸḺḼḾṀṂṄṆṈṊṌṎṐṒṔṖṘṚṜṞṠṢṤṦṨṪṬṮṰṲṴṶṸṺṼṾẀẂẄẆẈẊẌẎẐẒẔẠẢẤẦẨẪẬẮẰẲẴẶẸẺẼẾỀỂỄỆỈỊỌỎỐỒỔỖỘỚỜỞỠỢỤỦỨỪỬỮỰỲỴỶỸἈἉἊἋἌἍἎἏἘἙἚἛἜἝἨἩἪἫἬἭἮἯἸἹἺἻἼἽἾἿὈὉὊὋὌὍὙὛὝὟὨὩὪὫὬὭὮὯᾈᾉᾊᾋᾌᾍᾎᾏᾘᾙᾚᾛᾜᾝᾞᾟᾨᾩᾪᾫᾬᾭᾮᾯᾸᾹῘῙῨῩⒶⒷⒸⒹⒺⒻⒼⒽⒾⒿⓀⓁⓂⓃⓄⓅⓆⓇⓈⓉⓊⓋⓌⓍⓎⓏＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
const INVALID_INDEX = -1

function [pure] utf8ToUpper(str:string): string {
  return utf8(str).toUpper()
}

function [pure] utf8ToLower(str:string): string {
  return utf8(str).toLower()
}

let hasNativeCapitalize = "capitalize" in utf8 // an older exe lacks it

let utf8Capitalize = hasNativeCapitalize
  ? @[pure](str:string): string utf8(str).capitalize()
  : function [pure](str:string): string {
      let utf8Str = utf8(str)
      return "".concat(utf8(utf8Str.slice(0, 1)).toUpper(), utf8Str.slice(1))
    }

let utf8CapitalizeWords = hasNativeCapitalize
  ? @[pure](str:string): string utf8(str).capitalizeWords()
  : @[pure](str:string): string " ".join(str.split(" ").map(utf8Capitalize))

/**
 * Joins array elements into a string with the glue string between each element.
 * This function is a reverse operation to g_string.split()
 * @param {string[]} pieces - The array of strings to join.
 * @param {string}   glue - glue string.
 * @return {string} - String containing all the array elements in the same order,
 *                    with the glue string between each element.
 */
// Reverse operation to split()
function implode(pieces: array, glue: string = ""): string {
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
function join(pieces: array, glue: string = ""): string {
  return glue.join(pieces)
}

/**
 * Splits a string into an array of sub-strings.
 * The same as joined.split(glue): empty sub-strings stay, so it is lossless.
 * With isIgnoreEmpty = true the empty sub-strings are removed.
 * This function is a reverse operation to string.join()
 * @param {string} joined - The string to split.
 * @param {string} glue - glue string.
 * @return {string[]} - Array of sub-strings.
 */
function [pure] split(joined: string, glue: string, isIgnoreEmpty = false): array {
  return (!isIgnoreEmpty) ? joined.split(glue)
            : joined.split(glue).filter(@(v) v!="")
}

const intre = @"^-?\d+$"
const floatre = @"^-?\d+\.?\d*([eE][-+]?\d{1,3})?$"
const notstringre = @"(^-?\d+$)|(^-?\d+\.?\d*([eE][-+]?\d{1,3})?$)|^(null|true|false)$"
let intRegExp = regexp(intre)
let possibleNotStrRegExp = regexp(notstringre)
let floatRegExp = regexp(floatre)
let latinRegExp = regexp(@"[a-zA-Z]+")

function [pure] isStringObviousString(str: string): bool {
  return !possibleNotStrRegExp.match(str)
}

const defTostringParams = {
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
}

function func2str_compact(func): string {
  let { native, name } = func.getfuncinfos()
  return native ? $"(nativefunc): {name}"
    : name.slice(0,1) == "(" ? "@()" : $"{name}()"
}

function func2str(func, p: table|null = const {}): string {
  local compact = p?.compact ?? false
  local showsrc = p?.showsrc ?? false
  local showparams = p?.showparams ?? !compact
  local showdefparams = p?.showdefparams ?? compact
  local tostr_func = p?.tostr_func ?? @(v) $"{v}"

  local out = []
  local info = func.getfuncinfos()

  if (!info.native) {
    local defparams = info?.defparams ?? []
    local params = info.parameters.slice(1)
    if (info.varargs) {
      if (params.len()>0 && params.top()=="...")
        params=params.slice(0,-2)
      params.append("...")
    }
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
    if (showparams)
      out.extend(params_str)
    out.append(")")
  } else if (info.native) {
    out.append("(nativefunc): ", info.name)

  } else {
    out.append(func.tostring())
  }
  return "".join(out)
}

const simple_types = ["string", "float", "bool", "integer", "null", "userdata", "thread"].totable()
const function_types = ["function", "generator"].totable()

function tostring_any(input, tostringfunc=null, compact=true) {
  local typ = type(input)
  if (tostringfunc!=null) {
    if (tostringfunc instanceof Table)
      tostringfunc = [tostringfunc]
    if (tostringfunc instanceof Array) {
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
  else if (typ == "weakref"){
    return "#WEAKREF#"
  }
  if (typ=="thread") {
    return $"thread: {input.getstatus()}"
  }
  return input.tostring()
}

const table_types = ["table","class","instance"].totable()

const openSymByType = {
  ["array"] = "[",
  ["class"] = "class {",
  ["instance"] = "instance {",
}

let openSym = @(value) openSymByType?[type(value)] ?? "{"
let closeSym = @(value) value instanceof Array ? "]" : "}"

function tostring_r(inp, params=defTostringParams): string {
  local newline = params?.newline ?? defTostringParams.newline
  local maxdeeplevel = params?.maxdeeplevel ?? defTostringParams.maxdeeplevel
  local separator = params?.separator ?? defTostringParams.separator
  local showArrIdx = params?.showArrIdx ?? defTostringParams.showArrIdx
  local tostringfunc = params?.tostringfunc
  local indentOnNewline = params?.indentOnNewline ?? defTostringParams.indentOnNewline
  local splitlines = params?.splitlines ?? defTostringParams.splitlines
  local compact = params?.compact ?? defTostringParams.compact

  function tostringLeaf(val) {
    local typ = type(val)
    if (tostringfunc!=null) {
      if (tostringfunc instanceof Table)
        tostringfunc = [tostringfunc]
      foreach (tf in tostringfunc)
        if (tf.compare(val))
          return [true, tf.tostring(val)]
    }

    if (typ in simple_types || typ in function_types)
      return [true, tostring_any(val, null, compact)]
    if (typ == "table" && val.$len() == 0)
      return const [true, "{}"]
    if (typ == "array" && val.len() == 0)
      return const [true, "[]"]
    if (typ == "instance") {
      let str = val?.tostring instanceof Function ? val.tostring() : null
      return [str != null && str.indexof("(instance : 0x") != 0, str]
    }
    return const [false, null]
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
      if (input?.len instanceof Function) {
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
          stream.writestring(tostring_any(key,null, compact))
          stream.writestring(" = ")
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
        else if (arrayElem && li < maxind && !(input[li+1] instanceof Table)){
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
function [pure] slice(str:string, start:int = 0, end:int|null = null) {
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
function [pure] substring(str:string, start:int = 0, length:int|null = null) {
  local end = length
  if (length != null && length >= 0) {
    local total = str.len()
    if (start < 0)
      start += total
    start = clamp(start, 0, total).tointeger()
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
function [pure] startsWith(str:string, value:string): bool {
  return str.startswith(value)
}

/**
 * Determines whether the end of the string matches the specified substring.
 * Like C# function String.EndsWith().
 * @param {string}  str - Input string.
 * @param {string}  value - Matching substring.
 * @return {boolean}
 */
function [pure] endsWith(str:string, value:string): bool {
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
function [pure] indexOf(str:string, value:string, startIndex:int = 0) {
  if (value == "")
    return clamp(startIndex, 0, str.len())
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
function [pure] lastIndexOf(str:string, value:string, startIndex:int = 0): int {
  if (value == "")
    return str.len()
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

//returns the number of entries of @substr in @str.
function [pure] countSubstrings(str: string, substr: string): int {
  local res = 0
  local findex = str.indexof(substr)
  while (findex != null) {
    res++
    findex = str.indexof(substr, findex + 1)
  }
  return res
}

function [pure] capitalize(str: string): string {
  return "".concat(str.slice(0, 1).toupper(), str.slice(1))
}

function [pure] replace(str:string, from: string, to: string): string {
  return str.replace(from, to)
}

/**
 * Strips leading and trailing whitespace characters.
 * Like Java function trim().
 * @param {string} str - Input string.
 * @return {string} - String without whitespace chars.
 */
function [pure] trim(str:string): string {
  return str.strip()
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

function [pure] floatToStringRounded(value: number, presize: number): string {
  if (presize >= 1) {
    local res = (value / presize + (value < 0 ? -0.5 : 0.5)).tointeger()
    return res == 0 ? "0" : "".join([res].extend(array(log10(presize).tointeger(), "0")))
  }
  return format("%.{0}f".subst(-log10(presize).tointeger()), value)
}

function [pure] isStringInteger(str): bool {
  if (str instanceof Integer)
    return true
  if (!(str instanceof String))
    return false
  return intRegExp.match(str)
}

function [pure] isStringFloat(str, separator="."): bool {
  if (str instanceof Integer || str instanceof Float)
    return true
  if (!(str instanceof String))
    return false
  if (separator == ".")
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
  local expMark = lastSeg.contains("e") ? "e"
    : lastSeg.contains("E") ? "E"
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
  if (str instanceof String)
    str = str.strip()
  if (isStringInteger(str))
    return str.tointeger()
  if (needAssert)
    assert(false, @() $"can't convert '{str}' to integer")
  return defValue
}

function [pure] toFloatSafe(str, defValue = 0.0, needAssert = true) {
  if (str instanceof String)
    str = str.strip()
  if (isStringFloat(str))
    return str.tofloat()
  if (needAssert)
    assert(false, @() $"can't convert '{str}' to float")
  return defValue
}

function [pure] intToUtf8Char(c: int): string {
  if (c <= 0x7F)
    return c.tochar()
  if (c <= 0x7FF)
    return "".concat((0xC0 + (c>>6)).tochar(), (0x80 + (c & 0x3F)).tochar())
  if (c <= 0xFFFF)
    return "".concat((0xE0 + (c>>12)).tochar(), (0x80 + ((c>>6) & 0x3F)).tochar(), (0x80 + (c & 0x3F)).tochar())
  if (c <= 0x10FFFF)
    return "".concat((0xF0 + (c>>18)).tochar(), (0x80 + ((c>>12) & 0x3F)).tochar(), (0x80 + ((c>>6) & 0x3F)).tochar(), (0x80 + (c & 0x3F)).tochar())
  return ""
}

const firstOctet = [
  { prefix = 0, prefixMask = 0x80, mask = 0x7F }
  { prefix = 0xC0, prefixMask = 0xE0, mask = 0x1F }
  { prefix = 0xE0, prefixMask = 0xF0, mask = 0x0F }
  { prefix = 0xF0, prefixMask = 0xF8, mask = 0x07 }
]

function [pure] utf8CharToInt(str): int {
  let n = str.len()
  if (n == 0 || n > 4)
    return 0 //too many chars
  let { prefix, prefixMask, mask } = firstOctet[n - 1]
  let first = str[0] & 0xFF // constraint: char is signed on some platforms
  if ((first & prefixMask) != prefix)
    return 0 //bad code
  local res = first & mask
  for (local i = 1; i < n; i++) {
    let val = str[i] & 0xFF
    if ((val & 0xC0) != 0x80)
      return 0 //bad code
    res = (res << 6) | (val & 0x3F)
  }
  return res
}

function [pure] hexStringToInt(hexString): int {
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

function [pure] intToStrWithDelimiter(value: number, delimiter = " ", charsAmount = 3): string {
  local res = value.tointeger().tostring()
  local negativeSignCorrection = value < 0 ? 1 : 0
  local idx = res.len()
  while (charsAmount > 0 && idx > charsAmount + negativeSignCorrection) {
    idx -= charsAmount
    res = delimiter.concat(res.slice(0, idx), res.slice(idx))
  }
  return res
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

function [pure] validateEmail(no_dump_email): bool {
  if (!(no_dump_email instanceof String))
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

  if (quotes == null && locpart.contains("@"))
    return false //no @ without quotes

  if (!dompart.contains(".") || lastIndexOf(dompart, ".") > dompart.len() - 3)
    return false  //too short first level domain or no periods

  return true
}

function clearBorderSymbols(value:string, symList = const [" "]) {
  while (value != "" && symList.contains(value.slice(0,1)))
    value = value.slice(1)
  while (value!="" && symList.contains(value.slice(-1)))
    value = value.slice(0, -1)
  return value
}

function [pure] splitStringBySize(str, maxSize): array {
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

function obj2stringarray(obj, curpath = null): array {
  let res = []
  curpath = curpath ?? []
  if (obj instanceof Array) {
    foreach(i in obj)
      res.extend(obj2stringarray(i, [].extend(curpath)))
  }
  else if (obj instanceof Table) {
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
  countSubstrings
  implode
  join
  split
  replace
  trim
  floatToStringRounded
  isStringInteger
  isStringFloat
  isStringLatin = @[pure](str: string): bool latinRegExp.match(str)
  intToUtf8Char = utf8?.intToChar ?? intToUtf8Char
  utf8CharToInt = utf8?.charToInt ?? utf8CharToInt
  capitalize
  utf8Capitalize
  utf8ToUpper
  utf8ToLower
  utf8CapitalizeWords
  hexStringToInt
  cutPrefix
  cutPostfix
  intToStrWithDelimiter
  tostring_any
  tostring_r
  pprint
  validateEmail
  clearBorderSymbols

  toIntegerSafe
  toFloatSafe
  splitStringBySize
  obj2stringarray
}))
