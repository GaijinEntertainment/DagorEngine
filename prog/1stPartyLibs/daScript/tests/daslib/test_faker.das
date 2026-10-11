options gen2

require dastest/testing_boost public
require daslib/faker
require strings
require daslib/strings_boost

let REF_MONTH_SHIFT = fixed_array<int>(0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4)

let MONTH_NAMES = fixed_array<string>("January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December")

let DAY_NAMES = fixed_array<string>("Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday")

def private reference_week_day(year, month, day : int) : int {
    var y = year
    if (month < 3) {
        y --
    }
    return (y + y / 4 - y / 100 + y / 400 + REF_MONTH_SHIFT[month - 1] + day) % 7
}

def private month_index(name : string) : int {
    for (mn, idx in MONTH_NAMES, count(0)) {
        if (mn == name) return idx
    }
    return -1
}

[test]
def test_week_day_known_dates(t : T?) {
    t |> run("dates the algorithm is anchored on") @(t : T?) {
        t |> equal(week_day(1700, 1, 1), 5)
        t |> equal(week_day(2028, 3, 1), 3)
        t |> equal(week_day(2028, 2, 29), 2)
        t |> equal(week_day(2000, 1, 1), 6)
        t |> equal(week_day(1900, 3, 1), 4)
        t |> equal(week_day(2024, 12, 31), 2)
    }
    t |> run("the uint overload agrees with the int one") @(t : T?) {
        t |> equal(week_day(2028u, 3u, 1u), 3)
        t |> equal(week_day(2024u, 12u, 31u), 2)
    }
}

[test]
def test_week_day_matches_reference(t : T?) {
    t |> run("every month of every year from 1700 to 2100") @(t : T?) {
        var mismatch = ""
        for (year in range(1700, 2101)) {
            for (month in range(1, 13)) {
                for (day in fixed_array<int>(1, 15, 28)) {
                    let got = week_day(year, month, day)
                    let want = reference_week_day(year, month, day)
                    if (got != want && empty(mismatch)) {
                        mismatch = "{year}-{month}-{day}: got {got}, want {want}"
                    }
                }
            }
        }
        t |> success(empty(mismatch), "first mismatch at {mismatch}")
    }
    t |> run("the leap-day boundary") @(t : T?) {
        var mismatch = ""
        for (year in range(1700, 2101)) {
            continue if (!is_leap_year(uint(year)))
            for (date in fixed_array<int2>(int2(2, 29), int2(3, 1))) {
                let got = week_day(year, date.x, date.y)
                let want = reference_week_day(year, date.x, date.y)
                if (got != want && empty(mismatch)) {
                    mismatch = "{year}-{date.x}-{date.y}: got {got}, want {want}"
                }
            }
        }
        t |> success(empty(mismatch), "first mismatch at {mismatch}")
    }
}

[test]
def test_faker_date_names_the_right_week_day(t : T?) {
    t |> run("generated dates carry the week day they fall on") @(t : T?) {
        var fake <- Faker()
        var mismatch = ""
        for (_i in range(200)) {
            let text = fake |> date
            let parts <- split(text, ", ")
            t |> equal(length(parts), 3)
            let monthDay <- split(parts[1], " ")
            t |> equal(length(monthDay), 2)
            let month = month_index(monthDay[0]) + 1
            let day = to_int(monthDay[1])
            let year = to_int(parts[2])
            let want = DAY_NAMES[reference_week_day(year, month, day)]
            if (parts[0] != want && empty(mismatch)) {
                mismatch = "{text}: named {parts[0]}, falls on {want}"
            }
        }
        t |> success(empty(mismatch), "{mismatch}")
    }
}

[test]
def test_faker_composite_draws_in_order(t : T?) {
    t |> run("a vector or range draw takes its lanes in the order scalar draws would") @(t : T?) {
        var composite <- Faker()
        var scalar <- Faker()
        let i2 = composite |> random_int2
        let i2x = scalar |> random_int
        let i2y = scalar |> random_int
        t |> equal(i2, int2(i2x, i2y))
        let f4 = composite |> random_float4
        let f4x = scalar |> random_float
        let f4y = scalar |> random_float
        let f4z = scalar |> random_float
        let f4w = scalar |> random_float
        t |> equal(f4, float4(f4x, f4y, f4z, f4w))
        let ur = composite |> random_urange64
        let lo = scalar |> random_uint64
        let hi = scalar |> random_uint64
        t |> equal("{ur}", "{urange64(lo, hi)}")
        let u64 = composite |> random_uint64
        let low_half = scalar |> random_uint
        let high_half = scalar |> random_uint
        t |> equal(u64, uint64(low_half) | (uint64(high_half) << 32ul))
    }
}
