#pragma once

#include <functional>
#include <string>

namespace ProjectManager {

class PlatformService {
public:
    static std::string getAppDataFolder();
    static bool directoryExists(const std::string& utf8Path);
    static bool fileExists(const std::string& utf8Path);
    static bool createDirectories(const std::string& utf8Path);

    static void openFolder(const std::string& folderPath);
    static void runScript(const std::string& folderPath);
    static void showError(const std::string& title, const std::string& message);

    static std::string toLowerUtf8(const std::string& utf8);
    static bool containsCaseInsensitiveUtf8(const std::string& haystack, const std::string& needle);
    static std::string trim(const std::string& s);

    static bool isSystemDarkMode();
    static void applyWindowDarkTitleBar(void* nativeWindowHandle, bool dark);
    static bool runWithDarkTitleBarOnCreate(bool dark, const std::function<void()>& createAndShowFn);

#if defined(_WIN32)
    static std::wstring utf8ToWide(const std::string& utf8);
    static std::string wideToUtf8(const std::wstring& wide);
#endif
};

} // namespace ProjectManager
