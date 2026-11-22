#pragma once

#include <string>

namespace ProjectManager {

constexpr int kSettingsColumnCount = 6;

struct AppSettings {
    int windowX = 0;
    int windowY = 0;
    int windowW = 1300;
    int windowH = 600;
    bool hasWindowGeometry = false;

    int columnWidths[kSettingsColumnCount] = {180, 270, 120, 115, 175, 240};
    bool hasColumnWidths = false;

    std::string codeEditorPath;

    int sortColumn = -1;
    bool sortAscending = true;
};

class SettingsService {
public:
    static std::string filePath();
    static AppSettings load();
    static void save(const AppSettings& settings);
};

} // namespace ProjectManager
