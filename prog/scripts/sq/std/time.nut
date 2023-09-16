let { abs } = require("math")
let { format, split_by_chars } = require("string")
let {isStringInteger} = require("string.nut")

const TIME_SECOND_IN_MSEC = 1000
const TIME_SECOND_IN_MSEC_F = 1000.0
const TIME_MINUTE_IN_SECONDS = 60
const TIME_MINUTE_IN_SECONDS_F = 60.0
const TIME_HOUR_IN_SECONDS = 3600
const TIME_HOUR_IN_SECONDS_F = 3600.0
const TIME_DAY_IN_HOURS = 24
const TIME_DAY_IN_SECONDS = 86400
const TIME_DAY_IN_SECONDS_F = 86400.0
const TIME_WEEK_IN_SECONDS = 604800
const TIME_WEEK_IN_SECONDS_F = 604800.0
const DAYS_TO_YEAR_1970 = 719528


let millisecondsToSeconds = @(time) time / TIME_SECOND_IN_MSEC_F
let secondsToMilliseconds = @(time) time * TIME_SECOND_IN_MSEC
let millisecondsToSecondsInt = @(time) time / TIME_SECOND_IN_MSEC
let secondsToMinutes = @(time) time / TIME_MINUTE_IN_SECONDS_F
let minutesToSeconds = @(time) time * TIME_MINUTE_IN_SECONDS
let secondsToHours = @(seconds) seconds / TIME_HOUR_IN_SECONDS_F
let hoursToSeconds = @(seconds) seconds * TIME_HOUR_IN_SECONDS
let daysToSeconds = @(days) days * TIME_DAY_IN_SECONDS
let secondsToDays = @(seconds) seconds / TIME_DAY_IN_SECONDS_F

let function secondsToTime(time){
  if(type(time)=="table" && "seconds" in time)
    return time
  let s = time.tointeger()
  return {
    days = s / TIME_DAY_IN_SECONDS
    hours = (s / TIME_HOUR_IN_SECONDS) % TIME_DAY_IN_HOURS
    minutes = (s % TIME_HOUR_IN_SECONDS) / TIME_MINUTE_IN_SECONDS
    seconds = s % TIME_MINUTE_IN_SECONDS
    milliseconds = ((time-s)*1000).tointeger()
  }
}

let function secondsToTimeSimpleString(time) {
  let sign = time >= 0 ? "" : "-"
  let { days=0, hours=0, minutes=0, seconds=0 } = secondsToTime(abs(time.tointeger()))
  let totalHours = days * TIME_DAY_IN_HOURS + hours
  let hoursStr = totalHours > 0 ? totalHours.tostring() : null
  let minuteStr = totalHours > 0 ? format("%02d", minutes) : minutes.tostring()
  let secondsStr = format("%02d", seconds)
  let res = ":".join([hoursStr,minuteStr,secondsStr].filter(@(v) v != null))
  return $"{sign}{res}"
}

let function roundTime(time){
  let t = (type(time)=="table" && "seconds" in time) ? clone time : secondsToTime(time)
  if (t.days > 2)
    t.hours = 0
  if (t.days > 0)
    t.minutes = 0
  if (t.minutes > 10 || t.hours > 0 || t.days > 0) {
    t.seconds = 0
  }
  return t
}

let timeTbl = {
  s = 1
  m = TIME_MINUTE_IN_SECONDS
  h = TIME_HOUR_IN_SECONDS
  d = TIME_DAY_IN_SECONDS
  w = TIME_WEEK_IN_SECONDS
}

let function getSecondsFromTemplate(str, errorValue = null) {
 // "1w 1d 1h 1m 1s"
  if (!str.len())
    return errorValue

  local seconds = 0
  foreach (val in split_by_chars(str, " ")) {
    let key = val.slice(val.len() - 1)
    if (!(key in timeTbl))
      return errorValue

    let timeVal = val.slice(0, val.len() - 1)
    if (!isStringInteger(timeVal))
      return errorValue

    seconds += timeVal.tointeger() * timeTbl[key]
  }

  return seconds
}

let function secondsToTimeFormatString(time) {
  let {days=0, hours=0, minutes=0, seconds=0} = secondsToTime(time)
  let res = []
  if (days>0)
    res.append("{0}{days}".subst(days))
  if (hours>0)
    res.append("{0}{hours}".subst(hours))
  if (minutes>0)
    res.append("{0}{minutes}".subst(minutes))
  if (seconds>0)
    res.append("{0}{seconds}".subst(minutes+hours > 0 ? format("%02d", seconds) : seconds.tostring()))
  return " ".join(res)
}

return {
  millisecondsToSeconds
  secondsToMilliseconds
  millisecondsToSecondsInt
  secondsToMinutes
  minutesToSeconds
  secondsToHours
  hoursToSeconds
  daysToSeconds
  secondsToDays

  secondsToTimeFormatString
  secondsToTimeSimpleString
  secondsToTime
  roundTime
  getSecondsFromTemplate

  TIME_SECOND_IN_MSEC
  TIME_SECOND_IN_MSEC_F
  TIME_MINUTE_IN_SECONDS
  TIME_MINUTE_IN_SECONDS_F
  TIME_HOUR_IN_SECONDS
  TIME_HOUR_IN_SECONDS_F
  TIME_DAY_IN_HOURS
  TIME_DAY_IN_SECONDS
  TIME_DAY_IN_SECONDS_F
  TIME_WEEK_IN_SECONDS
  TIME_WEEK_IN_SECONDS_F
  DAYS_TO_YEAR_1970
}
