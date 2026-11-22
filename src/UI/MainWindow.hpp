#pragma once

#include "Models/Project.hpp"
#include "Services/GitStatusService.hpp"
#include "Services/SettingsService.hpp"
#include "UI/Icons.hpp"
#include "UI/ProjectTable.hpp"
#include "UI/SearchInput.hpp"
#include "UI/Theme.hpp"

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ProjectManager {

class MainWindow : public Fl_Double_Window {
public:
    explicit MainWindow(const Theme& theme);
    ~MainWindow() override;

    void showWindow();
    void hide() override;
    void resize(int X, int Y, int W, int H) override;
    int handle(int event) override;

private:
    void buildToolbar();
    void applySavedWindowGeometry();
    void applyFilter();
    void persistSettings();
    void saveUiState();
    Project* findProjectById(int id);
    std::function<bool(const Project*, const Project*)> makeSortLess() const;

    void onOpenConfigFolder();
    void onOpenSettings();
    void onRefreshGit();
    void onAddProject();
    void onLanguageFilterChanged();
    void onSearchQueryChanged();
    void onSortColumn(int column);
    void onTableAction(int projectId, ProjectTable::Action action);
    void onTableCellEdited(int projectId, ProjectTable::Column col, const std::string& newValue);
    void onDeleteProject(int projectId);

    struct GitJob {
        int id = 0;
        std::string folder;
    };

    void startGitWorker();
    void stopGitWorker();
    void setGitStatusEnabled(bool enabled);
    void enqueueGitJobs(bool force);
    void enqueueGitJob(int id, const std::string& folder);
    void gitWorkerLoop();
    void onGitResult(int projectId, const std::string& folder, const GitProbe& probe);

    static void cbOpenConfigFolder(Fl_Widget*, void* userData);
    static void cbOpenSettings(Fl_Widget*, void* userData);
    static void cbRefreshGit(Fl_Widget*, void* userData);
    static void cbAddProject(Fl_Widget*, void* userData);
    static void cbLanguageFilter(Fl_Widget*, void* userData);
    static void cbSearchInput(Fl_Widget*, void* userData);
    static void cbDeferredSave(void* userData);
    static void cbGitRefresh(void* userData);
    static void gitAwakeCallback(void* data);

    Theme theme_;
    IconSet icons_;
    AppSettings settings_;
    bool uiReady_ = false;

    std::vector<std::unique_ptr<Project>> allProjects_;
    std::string selectedLanguageFilter_ = "Все";
    std::string searchQuery_;

    Fl_Group* toolbar_ = nullptr;
    Fl_Button* btnConfigFolder_ = nullptr;
    Fl_Button* btnSettings_ = nullptr;
    Fl_Button* btnRefreshGit_ = nullptr;
    Fl_Button* btnAddProject_ = nullptr;
    Fl_Choice* languageFilterChoice_ = nullptr;
    SearchInput* searchInput_ = nullptr;
    ProjectTable* table_ = nullptr;

    std::mutex gitMutex_;
    std::condition_variable gitCv_;
    std::vector<GitJob> gitQueue_;
    std::atomic<bool> gitStop_{false};
    std::thread gitThread_;
    std::chrono::steady_clock::time_point lastGitEnqueue_{};
    static std::atomic<MainWindow*> gitUiOwner_;
};

} // namespace ProjectManager
