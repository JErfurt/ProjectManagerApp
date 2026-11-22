#include "Services/SettingsService.hpp"

#include "Services/PlatformService.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>

namespace ProjectManager {

namespace {

std::filesystem::path toFsPath(const std::string& utf8Path) {
#if defined(_WIN32)
    return std::filesystem::path(PlatformService::utf8ToWide(utf8Path));
#else
    return std::filesystem::path(utf8Path);
#endif
}

std::string joinPath(const std::string& dir, const std::string& name) {
    if (dir.empty()) return name;
    char sep =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif
    if (dir.back() == '\\' || dir.back() == '/') {
        return dir + name;
    }
    return dir + sep + name;
}

bool parseInt(const std::string& text, int& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || (end && *end != '\0')) return false;
    out = static_cast<int>(value);
    return true;
}

void insertGitColumn(const std::vector<int>& six, int* widths, int& sortColumn) {
    for (int i = 0; i < 4; ++i) {
        widths[i] = six[static_cast<std::size_t>(i)];
    }
    widths[4] = 58;
    widths[5] = six[4];
    widths[6] = six[5];
    if (sortColumn >= 4) {
        ++sortColumn;
    }
}

bool assignColumnWidths(const std::string& text, int layoutVersion, int* widths, int& sortColumn) {
    std::vector<int> parsed;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t comma = text.find(',', start);
        std::string token = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        int value = 0;
        if (!parseInt(token, value) || value < 36 || value > 4000) return false;
        parsed.push_back(value);
        if (comma == std::string::npos) break;
        start = comma + 1;
        if (start >= text.size()) return false;
    }

    if (layoutVersion >= 2 && static_cast<int>(parsed.size()) == kSettingsColumnCount) {
        for (int i = 0; i < kSettingsColumnCount; ++i) {
            widths[i] = parsed[static_cast<std::size_t>(i)];
        }
        return true;
    }

    if (layoutVersion < 2 && parsed.size() == 6) {
        insertGitColumn(parsed, widths, sortColumn);
        return true;
    }

    if (layoutVersion < 2 && parsed.size() == 7) {
        // Раньше первым столбцом были кнопки действий, столбца Git ещё не было.
        if (sortColumn > 0) {
            --sortColumn;
        }
        insertGitColumn(std::vector<int>(parsed.begin() + 1, parsed.end()), widths, sortColumn);
        return true;
    }

    return false;
}

} // namespace

std::string SettingsService::filePath() {
    return joinPath(PlatformService::getAppDataFolder(), "settings.ini");
}

AppSettings SettingsService::load() {
    AppSettings settings;
    std::ifstream in(toFsPath(filePath()));
    if (!in) return settings;

    bool hasX = false;
    bool hasY = false;
    bool hasW = false;
    bool hasH = false;
    std::string columnsRaw;
    int columnsLayout = 0;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') continue;

        std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if (key == "window_x") {
            hasX = parseInt(value, settings.windowX);
        } else if (key == "window_y") {
            hasY = parseInt(value, settings.windowY);
        } else if (key == "window_w") {
            hasW = parseInt(value, settings.windowW);
        } else if (key == "window_h") {
            hasH = parseInt(value, settings.windowH);
        } else if (key == "columns") {
            columnsRaw = value;
        } else if (key == "columns_layout") {
            parseInt(value, columnsLayout);
        } else if (key == "editor") {
            settings.codeEditorPath = value;
        } else if (key == "git_status") {
            int flag = 1;
            if (parseInt(value, flag)) {
                settings.gitStatusEnabled = flag != 0;
            }
        } else if (key == "sort_column") {
            parseInt(value, settings.sortColumn);
        } else if (key == "sort_ascending") {
            int flag = 1;
            if (parseInt(value, flag)) {
                settings.sortAscending = flag != 0;
            }
        }
    }

    if (hasX && hasY && hasW && hasH && settings.windowW >= 200 && settings.windowH >= 150) {
        settings.hasWindowGeometry = true;
    } else {
        settings.windowW = 1300;
        settings.windowH = 600;
    }

    if (!columnsRaw.empty()) {
        settings.hasColumnWidths = assignColumnWidths(
            columnsRaw, columnsLayout, settings.columnWidths, settings.sortColumn);
    }
    if (settings.sortColumn < 0 || settings.sortColumn >= kSettingsColumnCount) {
        settings.sortColumn = -1;
    }
    return settings;
}

void SettingsService::save(const AppSettings& settings) {
    PlatformService::createDirectories(PlatformService::getAppDataFolder());

    std::string editor = settings.codeEditorPath;
    for (char& ch : editor) {
        if (ch == '\n' || ch == '\r') ch = ' ';
    }

    std::ostringstream out;
    out << "window_x=" << settings.windowX << "\n";
    out << "window_y=" << settings.windowY << "\n";
    out << "window_w=" << settings.windowW << "\n";
    out << "window_h=" << settings.windowH << "\n";
    out << "columns=";
    for (int i = 0; i < kSettingsColumnCount; ++i) {
        if (i > 0) out << ',';
        out << settings.columnWidths[i];
    }
    out << "\n";
    out << "columns_layout=2\n";
    out << "editor=" << editor << "\n";
    out << "git_status=" << (settings.gitStatusEnabled ? 1 : 0) << "\n";
    out << "sort_column=" << settings.sortColumn << "\n";
    out << "sort_ascending=" << (settings.sortAscending ? 1 : 0) << "\n";

    std::ofstream file(toFsPath(filePath()), std::ios::binary | std::ios::trunc);
    if (!file) return;
    file << out.str();
}

} // namespace ProjectManager
