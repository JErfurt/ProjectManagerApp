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

} // namespace TimeUtil

inline std::string Project::lastInteractionDisplay() const {
    return TimeUtil::formatTimestampLocalDisplay(lastInteractionIso);
}

} // namespace ProjectManager
