#include "Services/GitStatusService.hpp"

#include "Services/PlatformService.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cstdio>
#endif

namespace ProjectManager {

namespace {

struct CmdResult {
    bool started = false;
    bool timedOut = false;
    int exitCode = -1;
    std::string output;
};

std::filesystem::path fsPath(const std::string& utf8) {
#if defined(_WIN32)
    return std::filesystem::path(PlatformService::utf8ToWide(utf8));
#else
    return std::filesystem::path(utf8);
#endif
}

std::time_t fileMtime(const std::filesystem::path& path) {
    std::error_code ec;
    auto ft = std::filesystem::last_write_time(path, ec);
    if (ec) return 0;
    using FileClock = decltype(ft)::clock;
    const auto sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ft - FileClock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sys);
}

void considerTime(std::time_t& latest, std::time_t candidate) {
    if (candidate > latest) {
        latest = candidate;
    }
}

#if defined(_WIN32)

std::wstring quoteArg(const std::wstring& arg) {
    if (arg.empty()) return L"\"\"";
    if (arg.find_first_of(L" \t\"") == std::wstring::npos) return arg;

    std::wstring out = L"\"";
    unsigned backs = 0;
    for (wchar_t ch : arg) {
        if (ch == L'\\') {
            ++backs;
            continue;
        }
        if (ch == L'"') {
            out.append(static_cast<std::size_t>(backs) * 2 + 1, L'\\');
            out.push_back(L'"');
        } else {
            if (backs > 0) out.append(backs, L'\\');
            out.push_back(ch);
        }
        backs = 0;
    }
    if (backs > 0) out.append(static_cast<std::size_t>(backs) * 2, L'\\');
    out.push_back(L'"');
    return out;
}

CmdResult runProcess(const std::vector<std::string>& args, int timeoutMs) {
    CmdResult result;
    if (args.empty()) return result;

    std::wstring command;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i > 0) command.push_back(L' ');
        command += quoteArg(PlatformService::utf8ToWide(args[i]));
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 1 << 16)) return result;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    HANDLE nul = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = nul;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (nul) CloseHandle(nul);
    CloseHandle(writePipe);
    if (!ok) {
        CloseHandle(readPipe);
        return result;
    }
    result.started = true;

    const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(timeoutMs);
    bool finished = false;
    while (!finished) {
        DWORD avail = 0;
        if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &avail, nullptr)) {
            break;
        }
        if (avail > 0) {
            std::string buf(avail, '\0');
            DWORD read = 0;
            if (ReadFile(readPipe, buf.data(), avail, &read, nullptr) && read > 0) {
                buf.resize(read);
                if (result.output.size() < (1u << 20)) {
                    result.output += buf;
                }
            }
        }
        DWORD wait = WaitForSingleObject(pi.hProcess, avail > 0 ? 0 : 30);
        if (wait == WAIT_OBJECT_0) {
            finished = true;
        } else if (GetTickCount64() >= deadline) {
            result.timedOut = true;
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 1000);
            finished = true;
        }
    }

    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) break;
        std::string buf(avail, '\0');
        DWORD read = 0;
        if (!ReadFile(readPipe, buf.data(), avail, &read, nullptr) || read == 0) break;
        buf.resize(read);
        if (result.output.size() < (1u << 20)) result.output += buf;
    }

    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    result.exitCode = static_cast<int>(code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readPipe);
    return result;
}

#else

std::string shellQuote(const std::string& text) {
    std::string out = "'";
    for (char ch : text) {
        if (ch == '\'') out += "'\\''";
        else out.push_back(ch);
    }
    out.push_back('\'');
    return out;
}

CmdResult runProcess(const std::vector<std::string>& args, int /*timeoutMs*/) {
    CmdResult result;
    if (args.empty()) return result;
    std::string command;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i > 0) command.push_back(' ');
        command += shellQuote(args[i]);
    }
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return result;
    result.started = true;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe)) {
        if (result.output.size() < (1u << 20)) result.output += buf;
    }
    int code = pclose(pipe);
    result.exitCode = code;
    return result;
}

#endif

std::string afterFields(const std::string& line, int fields) {
    std::size_t pos = 0;
    for (int i = 0; i < fields; ++i) {
        if (pos >= line.size()) return {};
        if (i > 0) {
            if (line[pos] != ' ') return {};
            ++pos;
        }
        if (pos >= line.size() || line[pos] == ' ') return {};
        while (pos < line.size() && line[pos] != ' ') ++pos;
    }
    if (pos < line.size() && line[pos] == ' ') ++pos;
    if (pos > line.size()) return {};
    return line.substr(pos);
}

std::string unquoteGit(const std::string& text) {
    if (text.size() < 2 || text.front() != '"') return text;
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 1; i < text.size(); ++i) {
        if (text[i] == '"') break;
        if (text[i] == '\\' && i + 1 < text.size()) {
            char next = text[++i];
            if (next == 'n') out.push_back('\n');
            else if (next == 't') out.push_back('\t');
            else if (next == 'r') out.push_back('\r');
            else out.push_back(next);
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::string pathFromRest(const std::string& rest) {
    if (rest.empty()) return {};
    if (rest.front() == '"') return unquoteGit(rest);
    auto tab = rest.find('\t');
    if (tab == std::string::npos) return rest;
    return rest.substr(0, tab);
}

struct StatusInfo {
    std::string head;
    std::string upstream;
    int ahead = 0;
    int behind = 0;
    bool hasAb = false;
    bool detached = false;
    bool initial = false;
    bool dirty = false;
    std::vector<std::string> paths;
};

bool startsWith(const std::string& line, const char* prefix, std::string& value) {
    const std::size_t n = std::char_traits<char>::length(prefix);
    if (line.size() < n || line.compare(0, n, prefix) != 0) return false;
    value = line.substr(n);
    return true;
}

StatusInfo parseStatus(const std::string& text) {
    StatusInfo info;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(pos, end - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        pos = end + (end < text.size() ? 1 : 0);
        if (end >= text.size()) pos = text.size();
        if (line.empty()) continue;

        std::string value;
        if (startsWith(line, "# branch.head ", value)) {
            info.head = value;
            info.detached = value == "(detached)";
            continue;
        }
        if (startsWith(line, "# branch.upstream ", value)) {
            info.upstream = value;
            continue;
        }
        if (startsWith(line, "# branch.ab ", value)) {
            int ahead = 0;
            int behind = 0;
            if (std::sscanf(value.c_str(), "+%d -%d", &ahead, &behind) == 2) {
                info.ahead = ahead;
                info.behind = behind;
                info.hasAb = true;
            }
            continue;
        }
        if (startsWith(line, "# branch.oid ", value)) {
            info.initial = value.find("(initial)") != std::string::npos;
            continue;
        }
        if (line[0] == '!' || line[0] == '#') continue;

        std::string path;
        if (line[0] == '?') path = pathFromRest(afterFields(line, 1));
        else if (line[0] == '1') path = pathFromRest(afterFields(line, 8));
        else if (line[0] == '2') path = pathFromRest(afterFields(line, 9));
        else if (line[0] == 'u') path = pathFromRest(afterFields(line, 10));
        else continue;

        info.dirty = true;
        if (!path.empty() && info.paths.size() < 300) {
            info.paths.push_back(std::move(path));
        }
    }
    return info;
}

bool isNotRepository(const std::string& output) {
    return output.find("not a git repository") != std::string::npos ||
           output.find("не является репозиторием") != std::string::npos;
}

std::string commitCount(int count) {
    const int n100 = count % 100;
    const int n10 = count % 10;
    const char* word = "коммитов";
    if (n100 < 11 || n100 > 14) {
        if (n10 == 1) word = "коммит";
        else if (n10 >= 2 && n10 <= 4) word = "коммита";
    }
    return std::to_string(count) + " " + word;
}

std::string branchLabel(const StatusInfo& info) {
    if (info.detached) return "Отделённый HEAD";
    if (info.head.empty()) return "Текущая ветка";
    return "Ветка «" + info.head + "»";
}

std::string syncTooltip(const StatusInfo& info) {
    const std::string branch = branchLabel(info);
    if (info.detached) {
        return "Отделённый HEAD, нет связи с удалённой веткой";
    }
    if (info.initial) {
        return "В репозитории ещё нет коммитов";
    }
    if (info.upstream.empty() || !info.hasAb) {
        return branch + " не отслеживает удалённую ветку";
    }
    if (info.ahead == 0 && info.behind == 0) {
        return branch + " совпадает с " + info.upstream;
    }
    if (info.ahead > 0 && info.behind > 0) {
        return branch + " разошлась с " + info.upstream +
               ": впереди " + commitCount(info.ahead) +
               ", позади " + commitCount(info.behind);
    }
    if (info.ahead > 0) {
        return branch + " впереди " + info.upstream + " на " + commitCount(info.ahead);
    }
    return branch + " позади " + info.upstream + " на " + commitCount(info.behind);
}

std::filesystem::path gitDirectory(const std::filesystem::path& folder) {
    const std::filesystem::path git = folder / ".git";
    std::error_code ec;
    if (std::filesystem::is_directory(git, ec)) return git;
    if (!std::filesystem::is_regular_file(git, ec)) return {};

    std::ifstream in(git);
    std::string line;
    if (!std::getline(in, line)) return {};
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const std::string prefix = "gitdir:";
    if (line.compare(0, prefix.size(), prefix) != 0) return {};
    std::string rest = PlatformService::trim(line.substr(prefix.size()));
    if (rest.empty()) return {};
    std::filesystem::path pointed = fsPath(rest);
    if (pointed.is_relative()) pointed = folder / pointed;
    return pointed;
}

std::time_t parseReflogStamp(const std::string& line) {
    std::size_t end = line.find('\t');
    if (end == std::string::npos) end = line.size();
    while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\r')) --end;

    std::size_t tzStart = end;
    while (tzStart > 0 && line[tzStart - 1] != ' ') --tzStart;
    if (tzStart >= end || end - tzStart < 5) return 0;
    if (line[tzStart] != '+' && line[tzStart] != '-') return 0;

    std::size_t numEnd = tzStart;
    while (numEnd > 0 && line[numEnd - 1] == ' ') --numEnd;
    std::size_t numStart = numEnd;
    while (numStart > 0 && line[numStart - 1] >= '0' && line[numStart - 1] <= '9') --numStart;
    if (numStart >= numEnd) return 0;
    return static_cast<std::time_t>(std::strtoll(line.c_str() + numStart, nullptr, 10));
}

std::time_t reflogTime(const std::filesystem::path& gitDir) {
    if (gitDir.empty()) return 0;
    const std::filesystem::path logPath = gitDir / "logs" / "HEAD";
    std::ifstream in(logPath, std::ios::binary);
    if (!in) return 0;
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size <= 0) return 0;
    const std::streamoff chunk = size > 65536 ? 65536 : size;
    in.seekg(size - chunk);
    std::string data(static_cast<std::size_t>(chunk), '\0');
    in.read(data.data(), chunk);
    data.resize(static_cast<std::size_t>(in.gcount()));

    std::size_t end = data.size();
    while (end > 0 && (data[end - 1] == '\n' || data[end - 1] == '\r' || data[end - 1] == '\0')) --end;
    if (end == 0) return 0;
    std::size_t start = end;
    while (start > 0 && data[start - 1] != '\n') --start;
    return parseReflogStamp(data.substr(start, end - start));
}

std::string trimOutput(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }
    std::size_t begin = 0;
    while (begin < text.size() && (text[begin] == '\n' || text[begin] == '\r' || text[begin] == ' ')) {
        ++begin;
    }
    return text.substr(begin);
}

GitProbe unavailable(const std::string& tooltip) {
    GitProbe probe;
    probe.sync = GitSync::Unavailable;
    probe.tooltip = tooltip;
    return probe;
}

GitProbe notARepo(const std::string& tooltip) {
    GitProbe probe;
    probe.sync = GitSync::NotARepo;
    probe.tooltip = tooltip;
    return probe;
}

GitProbe probeImpl(const std::string& folderPath) {
    const std::string folder = PlatformService::trim(folderPath);
    if (folder.empty()) return notARepo("Папка проекта не указана");
    if (!PlatformService::directoryExists(folder)) return notARepo("Папка не найдена");

    CmdResult status = runProcess({
        "git", "--no-pager",
        "-c", "core.quotepath=false",
        "-c", "i18n.logOutputEncoding=utf-8",
        "-C", folder,
        "--no-optional-locks",
        "status", "--porcelain=v2", "--branch", "--untracked-files=normal"
    }, 12000);

    if (!status.started) return unavailable("Git не найден в PATH");
    if (status.timedOut) return unavailable("Проверка Git не успела завершиться");
    if (isNotRepository(status.output)) return notARepo("Не репозиторий Git");
    if (status.exitCode != 0) return unavailable("Не удалось получить статус Git");

    const StatusInfo info = parseStatus(status.output);
    if (info.head.empty() && !info.initial) return unavailable("Не удалось разобрать статус Git");

    const bool synced = !info.detached && !info.initial && !info.upstream.empty() &&
                        info.hasAb && info.ahead == 0 && info.behind == 0;

    GitProbe probe;
    probe.sync = synced ? GitSync::Synced : GitSync::NotSynced;
    probe.dirty = info.dirty;
    probe.tooltip = syncTooltip(info);
    if (info.dirty) {
        if (!probe.tooltip.empty()) probe.tooltip += ". ";
        probe.tooltip += "Есть незакоммиченные изменения";
    }

    std::time_t latest = 0;
    std::filesystem::path gitDir = gitDirectory(fsPath(folder));
    if (gitDir.empty()) {
        CmdResult parsedDir = runProcess(
            {"git", "-C", folder, "rev-parse", "--absolute-git-dir"}, 8000);
        if (parsedDir.started && !parsedDir.timedOut && parsedDir.exitCode == 0) {
            gitDir = fsPath(trimOutput(parsedDir.output));
        }
    }
    considerTime(latest, reflogTime(gitDir));

    if (!info.initial) {
        CmdResult log = runProcess({
            "git", "--no-pager", "-C", folder, "--no-optional-locks",
            "log", "-1", "--format=%cI"
        }, 8000);
        if (log.started && !log.timedOut && log.exitCode == 0) {
            const std::string iso = trimOutput(log.output);
            std::time_t commitTime = 0;
            bool hasTz = false;
            if (TimeUtil::parseIso8601ToUtc(iso, commitTime, hasTz)) {
                considerTime(latest, commitTime);
            }
        }
    }

    if (info.dirty) {
        const std::filesystem::path root = fsPath(folder);
        for (const std::string& rel : info.paths) {
            std::filesystem::path relative = fsPath(rel);
            std::filesystem::path full = relative.is_absolute() ? relative : root / relative;
            considerTime(latest, fileMtime(full));
        }
    }

    if (latest > 0) {
        probe.activityIso = TimeUtil::timestampIso8601(latest);
    }
    return probe;
}

} // namespace

GitProbe GitStatusService::probe(const std::string& folderPath) {
    try {
        return probeImpl(folderPath);
    } catch (...) {
        return unavailable("Не удалось получить статус Git");
    }
}

} // namespace ProjectManager
