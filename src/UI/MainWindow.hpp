#pragma once

#include "Models/Project.hpp"
#include "UI/Icons.hpp"
#include "UI/ProjectTable.hpp"
#include "UI/SearchInput.hpp"
#include "UI/Theme.hpp"

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>

#include <memory>
#include <string>
#include <vector>

namespace ProjectManager {

class MainWindow : public Fl_Double_Window {
public:
    explicit MainWindow(const Theme& theme);
    ~MainWindow() override = default;

    void showWindow();

private:
    void buildToolbar();
    void applyFilter();
    Project* findProjectById(int id);

    void onOpenConfigFolder();
    void onAddProject();
    void onLanguageFilterChanged();
    void onSearchQueryChanged();
    void onTableAction(int projectId, ProjectTable::Action action);
    void onTableCellEdited(int projectId, ProjectTable::Column col, const std::string& newValue);
    void onDeleteProject(int projectId);

    static void cbOpenConfigFolder(Fl_Widget*, void* userData);
    static void cbAddProject(Fl_Widget*, void* userData);
    static void cbLanguageFilter(Fl_Widget*, void* userData);
    static void cbSearchInput(Fl_Widget*, void* userData);

    Theme theme_;
    IconSet icons_;

    std::vector<std::unique_ptr<Project>> allProjects_;
    std::string selectedLanguageFilter_ = "Все";
    std::string searchQuery_;

    Fl_Group* toolbar_ = nullptr;
    Fl_Button* btnConfigFolder_ = nullptr;
    Fl_Button* btnAddProject_ = nullptr;
    Fl_Choice* languageFilterChoice_ = nullptr;
    SearchInput* searchInput_ = nullptr;
    ProjectTable* table_ = nullptr;
};

} // namespace ProjectManager
