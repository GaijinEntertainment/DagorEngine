options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options strict_smart_pointers = true

module faker shared private

// TODO: add control for the RNG source

require daslib/regex_boost
require daslib/random
require daslib/static_let
require daslib/strings
require daslib/defer
require math
require daslib/math_bits

struct MonthNameAndDay
    name: string
    days: int

var private g_months <- [[MonthNameAndDay
    name="January",days=31;
    name="February",days=28;
    name="March",days=31;
    name="April",days=30;
    name="May",days=31;
    name="June",days=30;
    name="July",days=31;
    name="August",days=31;
    name="September",days=30;
    name="October",days=31;
    name="November",days=30;
    name="December",days=31
]]

var private g_days <- [[string
    "Sunday";
    "Monday";
    "Tuesday";
    "Wednesday";
    "Thursday";
    "Friday";
    "Saturday"
]]

struct public Faker
    //! Instance of the faker with all the settings inside.
    min_year : uint = 1980u                     //! minimal faker's year
    total_years : uint = (2022u-1980u)          //! faker year's range
    rnd : iterator<uint> <- each_random_uint()  //! fakers random number generator
    max_long_string : uint = 4096u              //! maximal length of generated string

def public Faker ( var rng : iterator<uint> )
    return <- [[ Faker() rnd<-rng ]]

var private re_positive_int <- %regex~[0-9][0-9]*%%
var private re_int <- %regex~[\+\-]?[0-9][0-9]*%%
var private re_uint <- %regex~[0-9][0-9]*%%
var private re_hex <- %regex~[0-9a-zA-Z][0-9a-zA-Z]*%%
var private re_string <- %regex~.*%%
var private re_file_name <- %regex~[0-9a-zA-Z_\.]+%%
var private re_float <- %regex~[\+\-]?[0-9][0-9]*(\.[0-9]*)?%%
var re_number <- %regex~(zero|one|two|three|four|five|six|seven|eight|nine|ten)%%

def public long_string ( var faker:Faker ) : string
    //! Generates a long string of random characters. The string is anywhere between 0 and faker.max_long_string characters long.
    var len = 0u
    faker.rnd |> next(len)
    len %= faker.max_long_string
    var inscope arr : array<uint8>
    arr |> reserve(int(len))
    for t in urange(len)
        var Ch = 0u
        faker.rnd |> next(Ch)
        arr |> push(uint8(Ch))
    return string(arr)

def public any_string ( var faker:Faker ) : string
    //! Generates a string of random characters. The string is anywhere between 0 and regex::re_gen_get_rep_limit() characters long.
    return re_gen(re_string,faker.rnd)

def public any_file_name ( var faker:Faker ) : string
    //! Generates random file name.
    return re_gen(re_file_name,faker.rnd)

def public random_int ( var faker:Faker ) : int
    //! Generates random integer.
    var i = 0u
    faker.rnd |> next(i)
    return int(i)

def public random_uint ( var faker:Faker ) : uint
    //! Generates random unsigned integer.
    var i = 0u
    faker.rnd |> next(i)
    return i

def public random_int8 ( var faker:Faker ) : int8
    //! Generates random int8.
    var i = 0u
    faker.rnd |> next(i)
    return int8(i)

def public random_uint8 ( var faker:Faker ) : uint8
    //! Generates random uint8.
    var i = 0u
    faker.rnd |> next(i)
    return uint8(i)

def public random_int16 ( var faker:Faker ) : int16
    //! Generates random int16.
    var i = 0u
    faker.rnd |> next(i)
    return int16(i)

def public random_uint16 ( var faker:Faker ) : uint16
    //! Generates random uint16.
    var i = 0u
    faker.rnd |> next(i)
    return uint16(i)

def public random_float ( var faker:Faker ) : float
    //! Generates random float.
    var i = 0u
    faker.rnd |> next(i)
    return uint_bits_to_float(i)

def public random_int2 ( var faker:Faker ) : int2
    //! Generates random int2.
    return int2(faker |> random_int, faker |> random_int)

def public random_range ( var faker:Faker ) : range
    //! Generates random range.
    return range(faker |> random_int, faker |> random_int)

def public random_range64 ( var faker:Faker ) : range64
    //! Generates random range64.
    return range64(faker |> random_int64, faker |> random_int64)

def public random_int3 ( var faker:Faker ) : int3
    //! Generates random int3.
    return int3(faker |> random_int, faker |> random_int, faker |> random_int)

def public random_int4 ( var faker:Faker ) : int4
    //! Generates random int4.
    return int4(faker |> random_int, faker |> random_int, faker |> random_int, faker |> random_int)

def public random_uint2 ( var faker:Faker ) : uint2
    //! Generates random uint2.
    return uint2(faker |> random_uint, faker |> random_uint)

def public random_urange ( var faker:Faker ) : urange
    //! Generates random urange.
    return urange(faker |> random_uint, faker |> random_uint)

def public random_urange64 ( var faker:Faker ) : urange64
    //! Generates random urange64.
    return urange64(faker |> random_uint64, faker |> random_uint64)

def public random_uint3 ( var faker:Faker ) : uint3
    //! Generates random uint3.
    return uint3(faker |> random_uint, faker |> random_uint, faker |> random_uint)

def public random_uint4 ( var faker:Faker ) : uint4
    //! Generates random uint4.
    return uint4(faker |> random_uint, faker |> random_uint, faker |> random_uint, faker |> random_uint)

def public random_float2 ( var faker:Faker ) : float2
    //! Generates random float2.
    return float2(faker |> random_float, faker |> random_float)

def public random_float3 ( var faker:Faker ) : float3
    //! Generates random float3.
    return float3(faker |> random_float, faker |> random_float, faker |> random_float)

def public random_float4 ( var faker:Faker ) : float4
    //! Generates random float4.
    return float4(faker |> random_float, faker |> random_float, faker |> random_float, faker |> random_float)

def public random_float3x3 ( var faker:Faker ) : float3x3
    //! Generates random float3x3.
    var m : float3x3
    m[0] = faker |> random_float3
    m[1] = faker |> random_float3
    m[2] = faker |> random_float3
    return m

def public random_float3x4 ( var faker:Faker ) : float3x4
    //! Generates random float3x4.
    var m : float3x4
    m[0] = faker |> random_float3
    m[1] = faker |> random_float3
    m[2] = faker |> random_float3
    m[3] = faker |> random_float3
    return m

def public random_float4x4 ( var faker:Faker ) : float4x4
    //! Generates random float4x4.
    var m : float4x4
    m[0] = faker |> random_float4
    m[1] = faker |> random_float4
    m[2] = faker |> random_float4
    m[3] = faker |> random_float4
    return m

variant BitRepresentation64 //! 64-bit representation of a float
    ui2 : uint[2]
    d   : double
    i64 : int64
    u64 : uint64

def public random_int64 ( var faker:Faker ) : int64
    //! Generates random int64
    var b = [[ BitRepresentation64 ui2=[[uint[2] faker |> random_uint; faker |> random_uint]] ]]
    return unsafe(b.i64)

def public random_uint64 ( var faker:Faker ) : uint64
    //! Generates random uint64
    var b = [[ BitRepresentation64 ui2=[[uint[2] faker |> random_uint; faker |> random_uint]] ]]
    return unsafe(b.u64)

def public random_double ( var faker:Faker ) : double
    //! Generates random double.
    var b = [[ BitRepresentation64 ui2=[[uint[2] faker |> random_uint; faker |> random_uint]] ]]
    return unsafe(b.d)

def public any_set ( var faker:Faker ) : uint[8]
    //! Generates random set (uint[8])
    var ch : uint[8]
    for c in ch
        faker.rnd |> next( c)
    return ch

def public any_char ( var faker:Faker ) : int
    //! Generates random char. (1 to 255 range)
    var ch = 0u
    faker.rnd |> next(ch)
    return int((ch % 255u)+1u)

def public number ( var faker:Faker ) : string
    //! Generates random number string.
    return re_gen(re_number,faker.rnd)

def public positive_int ( var faker:Faker ) : string
    //! Generates random positive integer string.
    return re_gen(re_positive_int,faker.rnd)

def public any_int ( var faker:Faker ) : string
    //! Generates random integer string.
    return re_gen(re_int,faker.rnd)

def public any_hex ( var faker:Faker ) : string
    //! Generates random integer hex string.
    return re_gen(re_hex,faker.rnd)

def public any_float ( var faker:Faker ) : string
    //! Generates random float string.
    return re_gen(re_float,faker.rnd)

def public any_uint ( var faker:Faker ) : string
    //! Generates random unsigned integer string.
    return re_gen(re_uint,faker.rnd)

def public month ( var faker:Faker )
    //! Generates random month string.
    var month = 0u
    faker.rnd |> next(month)
    month = month % 12u
    return g_months[month].name

def public day ( var faker:Faker )
    //! Generates random day string.
    var day = 0u
    faker.rnd |> next(day)
    day = day % 7u
    return g_days[day]

def public is_leap_year ( year:uint )
    //! Returns true if year is leap year.
    return (year % 4u == 0u) && ( year % 100u != 0u || year % 400u == 0u )

def public week_day ( year, month, day : uint )
    //! Returns week day for given date.
    return week_day(int(year),int(month),int(day))

def public week_day ( year, month, day : int )
    //! Returns week day for given date.
    static_let <|
        let offset <- [{auto 0; 31; 59; 90; 120; 151; 181; 212; 243; 273; 304; 334}]
    var afterFeb = 1
    if month > 2
        afterFeb = 0
    let aux = year - 1700 - afterFeb
    // dayOfWeek for 1700/1/1 = 5, Friday
    var dayOfWeek  = 5
    // partial sum of days betweem current date and 1700/1/1
    dayOfWeek += (aux + afterFeb) * 365
    // leap year correction
    dayOfWeek += aux / 4 - aux / 100 + (aux + 100) / 400
    // sum monthly and day offsets
    dayOfWeek += offset[month] + (day - 1)
    dayOfWeek %= 7
    return dayOfWeek

def public date ( var faker:Faker )
    //! Generates random date string.
    var month = 0u
    var day = 0u
    var year = 0u
    faker.rnd |> next(month)
    faker.rnd |> next(year)
    faker.rnd |> next(day)
    month = month % 12u
    year = faker.min_year + (year % faker.total_years)
    var max_days = uint(g_months[month].days)
    if ( is_leap_year(year) && month == 1u )
        max_days = 29u
    day = 1u + (day % max_days)
    let dow = week_day(year,month,day)
    return "{g_days[dow]}, {g_months[month].name} {int(day)}, {int(year)}"
