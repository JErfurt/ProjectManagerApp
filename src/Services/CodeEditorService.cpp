#include "Services/CodeEditorService.hpp"
#include "Services/PlatformService.hpp"

#include <cstdlib>
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
#else
    #include <unistd.h>
#endif

namespace ProjectManager {

namespace {

#if defined(_WIN32)
std::wstring expandEnvWin(const wchar_t* pattern) {
    DWORD needed = ExpandEnvironmentStringsW(pattern, nullptr, 0);
    if (needed == 0) return {};
    std::wstring buf(needed, L'\0');
    DWORD written = ExpandEnvironmentStringsW(pattern, &buf[0], needed);
    if (written == 0) return {};
    if (!buf.empty() && buf.back() == L'\0') {
        buf.pop_back();
    }
    return buf;
}

std::wstring findVSCodeWindows() {
    const wchar_t* patterns[] = {
        L"%LOCALAPPDATA%\\Programs\\Microsoft VS Code\\Code.exe",
        L"%ProgramFiles%\\Microsoft VS Code\\Code.exe",
        L"%ProgramFiles(x86)%\\Microsoft VS Code\\Code.exe"
    };

    for (const wchar_t* pattern : patterns) {
        std::wstring expanded = expandEnvWin(pattern);
        if (!expanded.empty()) {
            DWORD attrs = GetFileAttributesW(expanded.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                return expanded;
            }
        }
    }
    return {};
}
#else
std::string findExecutableInPath(const std::string& name) {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return {};
    std::string pathStr(pathEnv);
    std::size_t start = 0;
    while (start <= pathStr.size()) {
        std::size_t end = pathStr.find(':', start);
        if (end == std::string::npos) end = pathStr.size();
        std::string dir = pathStr.substr(start, end - start);
        if (!dir.empty()) {
            std::string candidate = dir + "/" + name;
            if (access(candidate.c_str(), X_OK) == 0) {
                return candidate;
            }
        }
        start = end + 1;
    }
    return {};
}

bool spawnDetachedEditor(const std::vector<std::string>& args) {
    if (args.empty()) return false;
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        setsid();
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

void CodeEditorService::openFolderInCodeEditor(const std::string& folderPath) {
    if (folderPath.empty()) return;

#if defined(_WIN32)
    std::wstring folderW = PlatformService::utf8ToWide(folderPath);
    std::wstring vscode = findVSCodeWindows();
    if (!vscode.empty()) {
        std::wstring args = L"\"" + folderW + L"\"";
        ShellExecuteW(nullptr, L"open", vscode.c_str(), args.c_str(), nullptr, SW_SHOWNORMAL);
        return;
    }

    // Fallback — открыть папку системно
    ShellExecuteW(nullptr, L"open", folderW.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    if (spawnDetachedEditor({"open", "-a", "Visual Studio Code", folderPath})) {
        return;
    }
    PlatformService::openFolder(folderPath);
#else
    const char* candidates[] = {"code", "code-insiders", "codium"};
    for (const char* cmd : candidates) {
        std::string found = findExecutableInPath(cmd);
        if (!found.empty()) {
            spawnDetachedEditor({found, folderPath});
            return;
        }
    }
    PlatformService::openFolder(folderPath);
#endif
}

} // namespace ProjectManager
