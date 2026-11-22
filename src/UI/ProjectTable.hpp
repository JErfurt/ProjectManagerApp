#pragma once

#include "Models/Project.hpp"
#include "UI/Icons.hpp"
#include "UI/Theme.hpp"

#include <FL/Fl_Input.H>
#include <FL/Fl_Table_Row.H>

#include <functional>
#include <vector>

namespace ProjectManager {

class ProjectTable : public Fl_Table_Row {
public:
    enum class Column {
        Actions = 0,
        Name = 1,
        Description = 2,
        Status = 3,
        Language = 4,
        LastInteraction = 5,
        FolderPath = 6,
        Count = 7
    };

    enum class Action {
        OpenFolder = 0,
        OpenEditor = 1,
        RunScript = 2
    };

    ProjectTable(int X, int Y, int W, int H, const Theme& theme, const IconSet& icons);
    ~ProjectTable() override = default;

    void setProjects(std::vector<Project*> visibleProjects);
    void appendVisibleProject(Project* project);
    void commitEdit();
    void cancelEdit();
    void selectAndScrollToRow(int row);
    int findRowByProjectId(int projectId) const;

    void setColumnWidths(const int* widths);
    void copyColumnWidths(int* widths);
    void setSortIndicator(int column, bool ascending);
    void sortCurrent(const std::function<bool(const Project*, const Project*)>& less);

    std::function<void(int projectId, Action action)> onAction;
    std::function<void(int projectId, Column col, const std::string& newValue)> onCellEdited;
    std::function<void(int projectId)> onDeleteProject;
    std::function<void(int column)> onHeaderClicked;
    std::function<void()> onColumnWidthsChanged;

protected:
    void draw() override;
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;
    int handle(int event) override;
    void resize(int X, int Y, int W, int H) override;

private:
    class CellInput;

    bool isEditableTextColumn(int col) const;
    void beginTextEdit(int row, int col);
    void openLanguagePopup(int row, int col);
    void openContextMenu(int row, int menuX, int menuY);
    void moveEditToNextCell(bool backward);

    bool hitTestActionButton(int row, int mouseX, int mouseY, int& outAction) const;
    void actionButtonRect(int cellX, int cellY, int cellW, int cellH, int actionIdx,
                          int& bx, int& by, int& bw, int& bh) const;
    void updateHover(int row, int col, int mouseX, int mouseY);
    void clearHover();
    void rememberColumnWidths();
    void notifyColumnWidthsChanged();

    const Theme& theme_;
    const IconSet& icons_;
    std::vector<Project*> projects_;

    CellInput* editInput_ = nullptr;
    bool editing_ = false;
    int editRow_ = -1;
    int editCol_ = -1;

    int hoverRow_ = -1;
    int hoverCol_ = -1;
    int hoverAction_ = -1;
    int hoverHeaderCol_ = -1;
    int pressedRow_ = -1;
    int pressedAction_ = -1;

    int headerPressCol_ = -1;
    int headerPressX_ = 0;
    int headerPressY_ = 0;

    int sortColumn_ = -1;
    bool sortAscending_ = true;
    int lastWidths_[static_cast<int>(Column::Count)] = {};
};

} // namespace ProjectManager
