#include "Services/PlatformService.hpp"

#include <FL/fl_ask.H>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
    #include <dwmapi.h>
#elif defined(__APPLE__)
    #include <sys/stat.h>
    #include <unistd.h>
    #include <spawn.h>
    extern char** environ;
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <spawn.h>
    extern char** environ;
#endif

namespace ProjectManager {

#if defined(_WIN32)
std::wstring PlatformService::utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring wide(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &wide[0], len);
    return wide;
}

std::string PlatformService::wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string utf8(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), &utf8[0], len, nullptr, nullptr);
    return utf8;
}
#endif

namespace {

std::filesystem::path makeFsPath(const std::string& utf8Path) {
#if defined(_WIN32)
    return std::filesystem::path(PlatformService::utf8ToWide(utf8Path));
#else
    return std::filesystem::path(utf8Path);
#endif
}

#if !defined(_WIN32)
bool isCommandAvailable(const std::string& cmd) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return false;
    std::string pathStr(pathEnv);
    std::size_t start = 0;
    while (start <= pathStr.size()) {
        std::size_t end = pathStr.find(':', start);
        if (end == std::string::npos) end = pathStr.size();
        std::string dir = pathStr.substr(start, end - start);
        if (!dir.empty()) {
            std::string candidate = dir + "/" + cmd;
            if (access(candidate.c_str(), X_OK) == 0) {
                return true;
            }
        }
        start = end + 1;
    }
    return false;
}

bool spawnDetached(const std::vector<std::string>& args, const std::string& workingDir = {}) {
    if (args.empty()) return false;
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        setsid();
        if (!workingDir.empty()) {
            (void)chdir(workingDir.c_str());
        }
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    return true;
}
#endif

} // namespace

std::string PlatformService::getAppDataFolder() {
#if defined(_WIN32)
    PWSTR appDataPath = nullptr;
    std::string base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath)) && appDataPath) {
        base = wideToUtf8(appDataPath);
        CoTaskMemFree(appDataPath);
    } else {
        const wchar_t* envAppData = _wgetenv(L"APPDATA");
        if (envAppData) {
            base = wideToUtf8(envAppData);
        }
    }
    if (base.empty()) {
        return "jerfurt\\ProjectManagerApp";
    }
    return base + "\\jerfurt\\ProjectManagerApp";
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    std::string base = home ? std::string(home) + "/Library/Application Support" : ".";
    return base + "/jerfurt/ProjectManagerApp";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg) + "/jerfurt/ProjectManagerApp";
    }
    const char* home = std::getenv("HOME");
    std::string base = home ? std::string(home) + "/.config" : ".";
    return base + "/jerfurt/ProjectManagerApp";
#endif
}

bool PlatformService::directoryExists(const std::string& utf8Path) {
    if (utf8Path.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_directory(makeFsPath(utf8Path), ec);
}

bool PlatformService::fileExists(const std::string& utf8Path) {
    if (utf8Path.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(makeFsPath(utf8Path), ec);
}

bool PlatformService::createDirectories(const std::string& utf8Path) {
    if (utf8Path.empty()) return false;
    std::error_code ec;
    std::filesystem::path p = makeFsPath(utf8Path);
    if (std::filesystem::is_directory(p, ec)) return true;
    return std::filesystem::create_directories(p, ec);
}

void PlatformService::showError(const std::string& title, const std::string& message) {
    fl_message_title(title.c_str());
    fl_alert("%s", message.c_str());
}

void PlatformService::openFolder(const std::string& folderPath) {
    if (folderPath.empty() || !directoryExists(folderPath)) {
        showError("Ошибка", "Папка не найдена!");
        return;
    }

#if defined(_WIN32)
    std::wstring widePath = utf8ToWide(folderPath);
    HINSTANCE res = ShellExecuteW(nullptr, L"explore", widePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(res) <= 32) {
        res = ShellExecuteW(nullptr, L"open", L"explorer.exe", widePath.c_str(), nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(res) <= 32) {
            showError("Ошибка", "Не удалось открыть папку.");
        }
    }
#elif defined(__APPLE__)
    if (!spawnDetached({"open", folderPath})) {
        showError("Ошибка", "Не удалось открыть папку.");
    }
#else
    if (isCommandAvailable("xdg-open")) {
        spawnDetached({"xdg-open", folderPath});
    } else if (isCommandAvailable("nautilus")) {
        spawnDetached({"nautilus", folderPath});
    } else if (isCommandAvailable("dolphin")) {
        spawnDetached({"dolphin", folderPath});
    } else {
        showError("Ошибка", "Файловый менеджер не найден");
    }
#endif
}

void PlatformService::runScript(const std::string& folderPath) {
    if (folderPath.empty() || !directoryExists(folderPath)) {
        showError("Ошибка", "Папка проекта не найдена!");
        return;
    }

#if defined(_WIN32)
    std::filesystem::path dirP = makeFsPath(folderPath);
    std::filesystem::path scriptP = dirP / L"run.bat";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(scriptP, ec)) {
        showError("Ошибка", "Файл run.bat не найден!");
        return;
    }

    HINSTANCE res = ShellExecuteW(
        nullptr,
        L"open",
        scriptP.c_str(),
        nullptr,
        dirP.c_str(),
        SW_SHOWNORMAL
    );
    if (reinterpret_cast<INT_PTR>(res) <= 32) {
        showError("Ошибка запуска", "Не удалось запустить run.bat.");
    }
#else
    std::string scriptPath = folderPath;
    if (!scriptPath.empty() && scriptPath.back() != '/') {
        scriptPath += '/';
    }
    scriptPath += "run.sh";
    if (!fileExists(scriptPath)) {
        showError("Ошибка", "Файл run.sh не найден!");
        return;
    }
    chmod(scriptPath.c_str(), 0755);
    if (!spawnDetached({"/bin/bash", scriptPath}, folderPath)) {
        showError("Ошибка запуска", "Не удалось запустить run.sh.");
    }
#endif
}

std::string PlatformService::toLowerUtf8(const std::string& utf8) {
    if (utf8.empty()) return {};
#if defined(_WIN32)
    std::wstring wide = utf8ToWide(utf8);
    if (wide.empty()) return utf8;
    CharLowerBuffW(&wide[0], static_cast<DWORD>(wide.size()));
    return wideToUtf8(wide);
#else
    std::string out;
    out.reserve(utf8.size());
    std::size_t i = 0;
    while (i < utf8.size()) {
        unsigned char c = static_cast<unsigned char>(utf8[i]);
        if (c < 0x80) {
            out.push_back(static_cast<char>(std::tolower(c)));
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) {
            unsigned char c2 = static_cast<unsigned char>(utf8[i + 1]);
            uint32_t cp = ((c & 0x1F) << 6) | (c2 & 0x3F);
            // Cyrillic uppercase: А-Я (0x0410..0x042F -> +0x20), Ё (0x0401 -> 0x0451)
            if (cp >= 0x0410 && cp <= 0x042F) {
                cp += 0x20;
            } else if (cp == 0x0401) {
                cp = 0x0451;
            } else if (cp >= 0x00C0 && cp <= 0x00DE && cp != 0x00D7) {
                cp += 0x20;
            }
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            i += 2;
        } else {
            out.push_back(static_cast<char>(c));
            ++i;
        }
    }
    return out;
#endif
}

bool PlatformService::containsCaseInsensitiveUtf8(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    if (haystack.empty()) return false;
    return toLowerUtf8(haystack).find(toLowerUtf8(needle)) != std::string::npos;
}

std::string PlatformService::trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool PlatformService::isSystemDarkMode() {
#if defined(_WIN32)
    DWORD value = 1;
    DWORD dataSize = sizeof(value);
    LONG status = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &dataSize
    );
    if (status == ERROR_SUCCESS) {
        return value == 0;
    }
#endif
    return false;
}

#if defined(_WIN32)
namespace {

bool applyDwmDarkModeAttribute(HWND hwnd, bool dark) {
    BOOL useDark = dark ? TRUE : FALSE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Win10 20H1+ / Win11), fallback 19 (older Win10)
    constexpr DWORD kDwmwaUseImmersiveDarkMode = 20;
    constexpr DWORD kDwmwaUseImmersiveDarkModeBefore20H1 = 19;
    if (SUCCEEDED(DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkMode, &useDark, sizeof(useDark)))) {
        return true;
    }
    return SUCCEEDED(DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkModeBefore20H1, &useDark, sizeof(useDark)));
}

thread_local bool gDarkTitleBarHookDark = false;
thread_local bool gDarkTitleBarHookApplied = false;

LRESULT CALLBACK darkTitleBarCallWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && lParam) {
        const auto* cwp = reinterpret_cast<const CWPSTRUCT*>(lParam);
        if (cwp->message == WM_CREATE && cwp->hwnd) {
            LONG_PTR style = GetWindowLongPtrW(cwp->hwnd, GWL_STYLE);
            if ((style & WS_CAPTION) != 0 && (style & WS_CHILD) == 0) {
                if (applyDwmDarkModeAttribute(cwp->hwnd, gDarkTitleBarHookDark)) {
                    gDarkTitleBarHookApplied = true;
                }
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace
#endif

void PlatformService::applyWindowDarkTitleBar(void* nativeWindowHandle, bool dark) {
#if defined(_WIN32)
    if (!nativeWindowHandle) return;
    HWND hwnd = static_cast<HWND>(nativeWindowHandle);
    if (!applyDwmDarkModeAttribute(hwnd, dark)) {
        return;
    }
    if (IsWindowVisible(hwnd)) {
        BOOL isActive = (GetActiveWindow() == hwnd);
        SendMessageW(hwnd, WM_NCACTIVATE, isActive ? FALSE : TRUE, 0);
        SendMessageW(hwnd, WM_NCACTIVATE, isActive ? TRUE : FALSE, 0);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
    }
#else
    (void)nativeWindowHandle;
    (void)dark;
#endif
}

bool PlatformService::runWithDarkTitleBarOnCreate(bool dark, const std::function<void()>& createAndShowFn) {
#if defined(_WIN32)
    if (!dark) {
        if (createAndShowFn) createAndShowFn();
        return true;
    }
    gDarkTitleBarHookDark = dark;
    gDarkTitleBarHookApplied = false;
    HHOOK hook = SetWindowsHookExW(WH_CALLWNDPROC, darkTitleBarCallWndProc, nullptr, GetCurrentThreadId());
    if (createAndShowFn) {
        createAndShowFn();
    }
    if (hook) {
        UnhookWindowsHookEx(hook);
    }
    return gDarkTitleBarHookApplied;
#else
    (void)dark;
    if (createAndShowFn) createAndShowFn();
    return true;
#endif
}

} // namespace ProjectManager
