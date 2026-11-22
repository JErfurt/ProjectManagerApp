#pragma once

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace ProjectManager {

enum class GitSync {
    Unknown = 0,
    NotARepo,
    Unavailable,
    Synced,
    NotSynced,
    Disabled
};

struct Project {
    int id = 0;
    std::string name;
    std::string description;
    std::string status;
    std::string folderPath;
    std::string language;
    std::string lastInteractionIso; // ISO-8601 string or empty if null

    GitSync gitSync = GitSync::Unknown;
    bool gitDirty = false;
    std::string gitTooltip;

    std::string lastInteractionDisplay() const;
    std::string lastInteractionTooltip() const;
};

inline const std::vector<std::string>& availableLanguages() {
    static const std::vector<std::string> langs = {
        "",
        "C#",
        "Python",
        "JavaScript",
        "TypeScript",
        "ASM",
        "Java",
        "C++",
        "C",
        "Go",
        "Rust",
        "Ruby",
        "PHP",
        "Swift",
        "Kotlin",
        "Dart",
        "R",
        "Scala",
        "Lua",
        "Shell",
        "PowerShell",
        "SQL",
        "HTML/CSS",
        "Other"
    };
    return langs;
}

inline const std::vector<std::string>& filterLanguages() {
    static const std::vector<std::string> langs = [] {
        std::vector<std::string> result;
        result.push_back("Все");
        for (const auto& l : availableLanguages()) {
            if (!l.empty()) {
                result.push_back(l);
            }
        }
        return result;
    }();
    return langs;
}

namespace TimeUtil {

inline std::time_t timegmPortable(std::tm* tmVal) {
#if defined(_WIN32)
    return _mkgmtime(tmVal);
#else
    return timegm(tmVal);
#endif
}

inline bool localtimePortable(std::time_t t, std::tm& out) {
#if defined(_WIN32)
    return localtime_s(&out, &t) == 0;
#else
    return localtime_r(&t, &out) != nullptr;
#endif
}

inline int computeLocalOffsetSeconds(std::time_t utcTime, const std::tm& localTm) {
    std::tm copy = localTm;
    copy.tm_isdst = 0;
    std::time_t asUtc = timegmPortable(&copy);
    if (asUtc == static_cast<std::time_t>(-1)) {
        return 0;
    }
    return static_cast<int>(asUtc - utcTime);
}

inline std::string timestampIso8601(std::time_t utcTime) {
    std::tm localTm{};
    if (!localtimePortable(utcTime, localTm)) {
        return {};
    }

    int offsetSec = computeLocalOffsetSeconds(utcTime, localTm);
    char sign = offsetSec >= 0 ? '+' : '-';
    int absOff = offsetSec >= 0 ? offsetSec : -offsetSec;
    int offH = absOff / 3600;
    int offM = (absOff % 3600) / 60;

    char buf[64];
    std::snprintf(
        buf, sizeof(buf),
        "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
        localTm.tm_year + 1900,
        localTm.tm_mon + 1,
        localTm.tm_mday,
        localTm.tm_hour,
        localTm.tm_min,
        localTm.tm_sec,
        sign, offH, offM
    );
    return buf;
}

inline std::string currentTimestampIso8601() {
    return timestampIso8601(std::time(nullptr));
}

inline bool parseIso8601ToUtc(const std::string& iso, std::time_t& outUtc, bool& hasTimezone) {
    if (iso.size() < 19) {
        return false;
    }

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    char sepDate1 = 0, sepDate2 = 0, sepMid = 0, sepTime1 = 0, sepTime2 = 0;
    if (std::sscanf(iso.c_str(), "%4d%c%2d%c%2d%c%2d%c%2d%c%2d",
                    &year, &sepDate1, &month, &sepDate2, &day,
                    &sepMid, &hour, &sepTime1, &minute, &sepTime2, &second) != 11) {
        return false;
    }
    if (sepDate1 != '-' || sepDate2 != '-' || (sepMid != 'T' && sepMid != 't' && sepMid != ' ') ||
        sepTime1 != ':' || sepTime2 != ':') {
        return false;
    }

    std::size_t pos = 19;
    // Skip fractional seconds if present (e.g. .1234567)
    if (pos < iso.size() && (iso[pos] == '.' || iso[pos] == ',')) {
        ++pos;
        while (pos < iso.size() && iso[pos] >= '0' && iso[pos] <= '9') {
            ++pos;
        }
    }

    int offsetSec = 0;
    hasTimezone = false;
    if (pos < iso.size()) {
        if (iso[pos] == 'Z' || iso[pos] == 'z') {
            hasTimezone = true;
            offsetSec = 0;
        } else if (iso[pos] == '+' || iso[pos] == '-') {
            int sign = (iso[pos] == '+') ? 1 : -1;
            ++pos;
            int oh = 0, om = 0;
            if (pos + 2 <= iso.size()) {
                oh = (iso[pos] - '0') * 10 + (iso[pos + 1] - '0');
                pos += 2;
                if (pos < iso.size() && iso[pos] == ':') {
                    ++pos;
                }
                if (pos + 2 <= iso.size()) {
                    om = (iso[pos] - '0') * 10 + (iso[pos + 1] - '0');
                }
                hasTimezone = true;
                offsetSec = sign * (oh * 3600 + om * 60);
            }
        }
    }

    std::tm tmVal{};
    tmVal.tm_year = year - 1900;
    tmVal.tm_mon = month - 1;
    tmVal.tm_mday = day;
    tmVal.tm_hour = hour;
    tmVal.tm_min = minute;
    tmVal.tm_sec = second;
    tmVal.tm_isdst = -1;

    if (hasTimezone) {
        tmVal.tm_isdst = 0;
        std::time_t utc = timegmPortable(&tmVal);
        if (utc == static_cast<std::time_t>(-1)) {
            return false;
        }
        outUtc = utc - offsetSec;
    } else {
        outUtc = std::mktime(&tmVal);
        if (outUtc == static_cast<std::time_t>(-1)) {
            return false;
        }
    }
    return true;
}

inline std::string formatTimestampLocalDisplay(const std::string& iso) {
    if (iso.empty()) {
        return "—";
    }

    std::time_t utcTime = 0;
    bool hasTz = false;
    if (!parseIso8601ToUtc(iso, utcTime, hasTz)) {
        return iso;
    }

    std::tm localTm{};
    if (!localtimePortable(utcTime, localTm)) {
        return iso;
    }

    char buf[64];
    std::snprintf(
        buf, sizeof(buf),
        "%02d.%02d.%04d %02d:%02d:%02d",
        localTm.tm_mday,
        localTm.tm_mon + 1,
        localTm.tm_year + 1900,
        localTm.tm_hour,
        localTm.tm_min,
        localTm.tm_sec
    );
    return buf;
}

inline int daysFromCivil(int year, int month, int day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153u * static_cast<unsigned>(month + (month > 2 ? -3 : 9)) + 2) / 5
        + static_cast<unsigned>(day) - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

inline const char* ruPlural(int n, const char* one, const char* few, const char* many) {
    int n100 = n % 100;
    if (n100 < 0) n100 = -n100;
    if (n100 >= 11 && n100 <= 14) return many;
    switch (n100 % 10) {
        case 1: return one;
        case 2:
        case 3:
        case 4: return few;
        default: return many;
    }
}

inline std::string ruAgo(int n, const char* one, const char* few, const char* many) {
    return std::to_string(n) + " " + ruPlural(n, one, few, many) + " назад";
}

inline std::string formatClock(const std::tm& localTm) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", localTm.tm_hour, localTm.tm_min);
    return buf;
}

// Column label: recent moments stay relative, the rest of today is a clock time,
// then yesterday / days / months / years.
inline std::string formatTimestampRelative(const std::string& iso) {
    if (iso.empty()) {
        return "—";
    }

    std::time_t utcTime = 0;
    bool hasTz = false;
    if (!parseIso8601ToUtc(iso, utcTime, hasTz)) {
        return iso;
    }

    std::tm thenLocal{};
    if (!localtimePortable(utcTime, thenLocal)) {
        return iso;
    }

    const std::time_t now = std::time(nullptr);
    std::tm nowLocal{};
    if (!localtimePortable(now, nowLocal)) {
        return formatTimestampLocalDisplay(iso);
    }

    const double diffSec = std::difftime(now, utcTime);
    if (diffSec >= 0.0 && diffSec < 60.0) {
        return "только что";
    }
    if (diffSec >= 60.0 && diffSec < 3600.0) {
        const int minutes = static_cast<int>(diffSec / 60.0);
        return ruAgo(minutes, "минуту", "минуты", "минут");
    }
    constexpr double kRecentHours = 6.0 * 3600.0;
    if (diffSec >= 3600.0 && diffSec < kRecentHours) {
        const int hours = static_cast<int>(diffSec / 3600.0);
        return ruAgo(hours, "час", "часа", "часов");
    }

    const int dayDelta = daysFromCivil(nowLocal.tm_year + 1900, nowLocal.tm_mon + 1, nowLocal.tm_mday)
        - daysFromCivil(thenLocal.tm_year + 1900, thenLocal.tm_mon + 1, thenLocal.tm_mday);
    if (dayDelta < 0) {
        return formatTimestampLocalDisplay(iso);
    }
    if (dayDelta == 0) {
        return formatClock(thenLocal);
    }
    if (dayDelta == 1) {
        return "вчера";
    }

    int months = (nowLocal.tm_year - thenLocal.tm_year) * 12 + (nowLocal.tm_mon - thenLocal.tm_mon);
    if (nowLocal.tm_mday < thenLocal.tm_mday) {
        --months;
    }
    int years = nowLocal.tm_year - thenLocal.tm_year;
    if (nowLocal.tm_mon < thenLocal.tm_mon ||
        (nowLocal.tm_mon == thenLocal.tm_mon && nowLocal.tm_mday < thenLocal.tm_mday)) {
        --years;
    }

    if (years >= 1) {
        return ruAgo(years, "год", "года", "лет");
    }
    if (months >= 1) {
        return ruAgo(months, "месяц", "месяца", "месяцев");
    }
    return ruAgo(dayDelta, "день", "дня", "дней");
}

} // namespace TimeUtil

inline std::string Project::lastInteractionDisplay() const {
    return TimeUtil::formatTimestampRelative(lastInteractionIso);
}

inline std::string Project::lastInteractionTooltip() const {
    if (lastInteractionIso.empty()) {
        return {};
    }
    const std::string full = TimeUtil::formatTimestampLocalDisplay(lastInteractionIso);
    if (full == "—") {
        return {};
    }
    return full;
}

} // namespace ProjectManager
