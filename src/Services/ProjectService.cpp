#include "Services/ProjectService.hpp"
#include "Services/PlatformService.hpp"

#include <sqlite3.h>

#include <cstdio>

namespace ProjectManager {

namespace {

struct SqliteDb {
    sqlite3* handle = nullptr;

    explicit SqliteDb(const std::string& path) {
        if (sqlite3_open_v2(path.c_str(), &handle,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                            nullptr) != SQLITE_OK) {
            if (handle) {
                sqlite3_close(handle);
                handle = nullptr;
            }
        }
    }

    ~SqliteDb() {
        if (handle) {
            sqlite3_close(handle);
        }
    }

    SqliteDb(const SqliteDb&) = delete;
    SqliteDb& operator=(const SqliteDb&) = delete;
};

struct SqliteStmt {
    sqlite3_stmt* stmt = nullptr;

    SqliteStmt(sqlite3* db, const char* sql) {
        if (db) {
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        }
    }

    ~SqliteStmt() {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }

    SqliteStmt(const SqliteStmt&) = delete;
    SqliteStmt& operator=(const SqliteStmt&) = delete;
};

std::string columnText(sqlite3_stmt* stmt, int col) {
    const unsigned char* txt = sqlite3_column_text(stmt, col);
    if (!txt) return {};
    int bytes = sqlite3_column_bytes(stmt, col);
    return std::string(reinterpret_cast<const char*>(txt), static_cast<std::size_t>(bytes));
}

void bindText(sqlite3_stmt* stmt, int idx, const std::string& value) {
    sqlite3_bind_text(stmt, idx, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void bindNullableText(sqlite3_stmt* stmt, int idx, const std::string& value) {
    if (value.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        bindText(stmt, idx, value);
    }
}

} // namespace

std::string ProjectService::appDataFolder() {
    static const std::string folder = PlatformService::getAppDataFolder();
    return folder;
}

std::string ProjectService::dbPath() {
    static const std::string path = [] {
        std::string dir = appDataFolder();
#if defined(_WIN32)
        if (!dir.empty() && dir.back() != '\\' && dir.back() != '/') {
            dir += '\\';
        }
#else
        if (!dir.empty() && dir.back() != '/') {
            dir += '/';
        }
#endif
        return dir + "projects.db";
    }();
    return path;
}

void ProjectService::ensureInitialized() {
    PlatformService::createDirectories(appDataFolder());

    SqliteDb db(dbPath());
    if (!db.handle) return;

    const char* createTableSql =
        "CREATE TABLE IF NOT EXISTS Projects ("
        "    Id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    Name TEXT NOT NULL,"
        "    Description TEXT,"
        "    Status TEXT,"
        "    FolderPath TEXT,"
        "    Language TEXT,"
        "    LastInteraction TEXT"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db.handle, createTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
    }
}

std::vector<Project> ProjectService::loadProjects() {
    ensureInitialized();

    std::vector<Project> projects;
    SqliteDb db(dbPath());
    if (!db.handle) return projects;

    const char* selectSql =
        "SELECT Id, Name, Description, Status, FolderPath, Language, LastInteraction FROM Projects";

    SqliteStmt stmt(db.handle, selectSql);
    if (!stmt.stmt) return projects;

    while (sqlite3_step(stmt.stmt) == SQLITE_ROW) {
        Project p;
        p.id = sqlite3_column_int(stmt.stmt, 0);
        p.name = columnText(stmt.stmt, 1);
        p.description = columnText(stmt.stmt, 2);
        p.status = columnText(stmt.stmt, 3);
        p.folderPath = columnText(stmt.stmt, 4);
        p.language = columnText(stmt.stmt, 5);
        p.lastInteractionIso = columnText(stmt.stmt, 6);
        projects.push_back(std::move(p));
    }

    return projects;
}

int ProjectService::addProject(const Project& project) {
    ensureInitialized();

    SqliteDb db(dbPath());
    if (!db.handle) return 0;

    const char* insertSql =
        "INSERT INTO Projects (Name, Description, Status, FolderPath, Language, LastInteraction) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";

    SqliteStmt stmt(db.handle, insertSql);
    if (!stmt.stmt) return 0;

    bindText(stmt.stmt, 1, project.name);
    bindText(stmt.stmt, 2, project.description);
    bindText(stmt.stmt, 3, project.status);
    bindText(stmt.stmt, 4, project.folderPath);
    bindText(stmt.stmt, 5, project.language);
    bindNullableText(stmt.stmt, 6, project.lastInteractionIso);

    if (sqlite3_step(stmt.stmt) == SQLITE_DONE) {
        return static_cast<int>(sqlite3_last_insert_rowid(db.handle));
    }
    return 0;
}

void ProjectService::updateProject(const Project& project) {
    if (project.id <= 0) return;
    ensureInitialized();

    SqliteDb db(dbPath());
    if (!db.handle) return;

    const char* updateSql =
        "UPDATE Projects "
        "SET Name = ?1, Description = ?2, Status = ?3, FolderPath = ?4, Language = ?5, LastInteraction = ?6 "
        "WHERE Id = ?7;";

    SqliteStmt stmt(db.handle, updateSql);
    if (!stmt.stmt) return;

    bindText(stmt.stmt, 1, project.name);
    bindText(stmt.stmt, 2, project.description);
    bindText(stmt.stmt, 3, project.status);
    bindText(stmt.stmt, 4, project.folderPath);
    bindText(stmt.stmt, 5, project.language);
    bindNullableText(stmt.stmt, 6, project.lastInteractionIso);
    sqlite3_bind_int(stmt.stmt, 7, project.id);

    sqlite3_step(stmt.stmt);
}

void ProjectService::updateLastInteraction(int id, const std::string& iso) {
    if (id <= 0 || iso.empty()) return;
    ensureInitialized();

    SqliteDb db(dbPath());
    if (!db.handle) return;

    const char* updateSql =
        "UPDATE Projects SET LastInteraction = ?1 WHERE Id = ?2;";

    SqliteStmt stmt(db.handle, updateSql);
    if (!stmt.stmt) return;

    bindText(stmt.stmt, 1, iso);
    sqlite3_bind_int(stmt.stmt, 2, id);
    sqlite3_step(stmt.stmt);
}

void ProjectService::deleteProject(int id) {
    if (id <= 0) return;
    ensureInitialized();

    SqliteDb db(dbPath());
    if (!db.handle) return;

    const char* deleteSql = "DELETE FROM Projects WHERE Id = ?1;";
    SqliteStmt stmt(db.handle, deleteSql);
    if (!stmt.stmt) return;

    sqlite3_bind_int(stmt.stmt, 1, id);
    sqlite3_step(stmt.stmt);
}

} // namespace ProjectManager
