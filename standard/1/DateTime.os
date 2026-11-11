// standard/1/DateTime.os
// Tier 4: calendar date/time (UTC), unix conversion, formatting, ISO-8601 parse, arithmetic.
module Str = import "standard/1/String.os";
module CStr = import "standard/1/c/string.os";
module CIO = import "standard/1/c/stdio_extra.os";

extern "C" fn time(t: i64*) => i64;

struct DateTime {
    year: i32 = 1970; month: i32 = 1; day: i32 = 1;
    hour: i32 = 0; minute: i32 = 0; second: i32 = 0;

    toUnix() => i64 {
        let y: i64 = this.year as i64;
        let m: i64 = this.month as i64;
        if (m <= 2) { y = y - 1; m = m + 12; }
        let days: i64 = 0;
        // days since 1970-01-01 via civil-from-days inverse (Howard Hinnant)
        let era: i64 = y / 400;
        if (y < 0) era = (y - 399) / 400;
        let yoe: i64 = y - era * 400;
        let mp: i64 = m - 3;
        let doy: i64 = (153 * mp + 2) / 5 + (this.day as i64) - 1;
        let doe: i64 = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        days = era * 146097 + doe - 719468 + 0;
        return days * 86400 + (this.hour as i64) * 3600 + (this.minute as i64) * 60 + (this.second as i64);
    }

    fromUnix(t: i64) => void {
        let days: i64 = t / 86400;
        let rem: i64 = t % 86400;
        if (rem < 0) { rem = rem + 86400; days = days - 1; }
        this.hour = (rem / 3600) as i32; this.minute = ((rem % 3600) / 60) as i32; this.second = (rem % 60) as i32;
        let z: i64 = days + 719468;
        let era: i64 = z / 146097;
        if (z < 0) era = (z - 146096) / 146097;
        let doe: i64 = z - era * 146097;
        let yoe: i64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        let y: i64 = yoe + era * 400;
        let doy: i64 = doe - (365 * yoe + yoe / 4 - yoe / 100);
        let mp: i64 = (5 * doy + 2) / 153;
        let d: i64 = doy - (153 * mp + 2) / 5 + 1;
        let m: i64 = mp + 3;
        if (mp >= 10) m = mp - 9;
        if (m <= 2) y = y + 1;
        this.year = y as i32; this.month = m as i32; this.day = d as i32;
    }

    addSeconds(s: i64) => void { this.fromUnix(this.toUnix() + s); }
    addDays(d: i64) => void { this.addSeconds(d * 86400); }

    // 0 = Sunday
    weekday() => int {
        let days: i64 = this.toUnix() / 86400;
        let w: i64 = (days + 4) % 7;
        if (w < 0) w = w + 7;
        return w as int;
    }

    // "2026-11-11T09:30:00Z"
    iso(out: Str.String*) => bool {
        let buf: [32]char;
        CIO.snprintf(&buf as char*, 32, "%04d-%02d-%02dT%02d:%02d:%02dZ", this.year, this.month, this.day, this.hour, this.minute, this.second);
        return out.append(&buf as char*);
    }

    // accepts "YYYY-MM-DD" or "YYYY-MM-DDTHH:MM:SS[Z]"
    parseIso(s: char*) => bool {
        let y: int = 0; let mo: int = 0; let d: int = 0; let h: int = 0; let mi: int = 0; let se: int = 0;
        let n: int = CIO.sscanf(s, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &se);
        if (n < 3) return false;
        this.year = y as i32; this.month = mo as i32; this.day = d as i32;
        this.hour = h as i32; this.minute = mi as i32; this.second = se as i32;
        return true;
    }
}

function isLeap(y: i32) => bool { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
function daysInMonth(y: i32, m: i32) => int {
    if (m == 2) { if (isLeap(y)) return 29; return 28; }
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
}
function now() => DateTime { let d: DateTime; d.fromUnix(time(nullptr)); return d; }
function diffSeconds(a: DateTime*, b: DateTime*) => i64 { return a.toUnix() - b.toUnix(); }
