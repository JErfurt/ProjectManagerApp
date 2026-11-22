#include "UI/ProjectTable.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multi_Label.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <string>

namespace ProjectManager {

namespace {

constexpr const char* kColumnHeaders[] = {
    "Название",
    "Описание",
    "Статус",
    "Тип",
    "Git",
    "Взаимодействие",
    "Директория"
};

constexpr int kInitialColWidths[] = {
    180, // Название проекта
    270, // Описание
    120, // Статус
    115, // Тип
    58,  // Git
    175, // Последнее взаимодействие
    240  // Директория
};

void drawGitStatus(const Project* project, bool selected, int X, int Y, int W, int H,
                   const Theme& theme, const IconSet& icons) {
    if (!project || project->gitSync == GitSync::NotARepo || project->gitSync == GitSync::Disabled) {
        fl_font(FL_HELVETICA, 14);
        fl_color(selected ? theme.selectionText : theme.mutedText);
        fl_draw("—", X, Y, W, H, FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
        return;
    }

    Fl_SVG_Image* cloud = icons.gitUnavailable();
    if (project->gitSync != GitSync::Unknown) {
        if (project->gitDirty) {
            cloud = icons.gitDirty();
        } else if (project->gitSync == GitSync::Synced) {
            cloud = icons.gitSynced();
        } else if (project->gitSync == GitSync::NotSynced) {
            cloud = icons.gitNotSynced();
        }
    }
    if (!cloud) return;

    const int size = cloud->w() > 0 ? cloud->w() : 16;
    const int ix = X + std::max(0, (W - size) / 2);
    const int iy = Y + std::max(0, (H - size) / 2);
    cloud->draw(ix, iy);
}

} // namespace

class ProjectTable::CellInput : public Fl_Input {
public:
    CellInput(int X, int Y, int W, int H, ProjectTable* owner)
        : Fl_Input(X, Y, W, H)
        , owner_(owner) {}

    int handle(int event) override {
        if (event == FL_KEYDOWN) {
            int key = Fl::event_key();
            if (key == FL_Enter || key == FL_KP_Enter) {
                owner_->commitEdit();
                return 1;
            }
            if (key == FL_Escape) {
                owner_->cancelEdit();
                return 1;
            }
            if (key == FL_Tab) {
                bool shift = (Fl::event_state() & FL_SHIFT) != 0;
                owner_->moveEditToNextCell(shift);
                return 1;
            }
            bool shift = (Fl::event_state() & FL_SHIFT) != 0;
            if (key == FL_Menu || (key == (FL_F + 10) && shift)) {
                owner_->commitEdit();
                owner_->openContextMenuForKeyboard();
                return 1;
            }
        } else if (event == FL_UNFOCUS) {
            int res = Fl_Input::handle(event);
            owner_->commitEdit();
            return res;
        }
        return Fl_Input::handle(event);
    }

private:
    ProjectTable* owner_;
};

ProjectTable::ProjectTable(int X, int Y, int W, int H, const Theme& theme, const IconSet& icons)
    : Fl_Table_Row(X, Y, W, H)
    , theme_(theme)
    , icons_(icons) {
    type(SELECT_SINGLE);
    box(FL_BORDER_FRAME);
    color(theme_.cellBg);
    selection_color(theme_.selectionBg);

    rows(0);
    cols(static_cast<int>(Column::Count));

    col_header(1);
    col_header_height(32);
    col_header_color(theme_.headerBg);

    row_header(0);
    row_height_all(38);

    col_resize(1);
    col_resize_min(48);

    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        col_width(c, kInitialColWidths[c]);
    }
    rememberColumnWidths();

    // Inline editor child widget
    editInput_ = new CellInput(0, 0, 100, 30, this);
    editInput_->box(FL_BORDER_BOX);
    editInput_->color(theme_.controlBg);
    editInput_->textcolor(theme_.text);
    editInput_->cursor_color(theme_.text);
    editInput_->textfont(FL_HELVETICA);
    editInput_->textsize(14);
    editInput_->hide();

    end();
}

void ProjectTable::setProjects(std::vector<Project*> visibleProjects) {
    int keepId = -1;
    int selected = selectedRow();
    if (selected >= 0 && selected < static_cast<int>(projects_.size())) {
        keepId = projects_[static_cast<std::size_t>(selected)]->id;
    }

    cancelEdit();
    clearHover();

    projects_ = std::move(visibleProjects);
    rows(static_cast<int>(projects_.size()));
    row_height_all(38);

    int row = keepId >= 0 ? findRowByProjectId(keepId) : -1;
    if (row < 0 && !projects_.empty()) {
        row = 0;
    }
    if (row >= 0) {
        selectAndScrollToRow(row);
    } else {
        select_all_rows(0);
        redraw();
    }
}

void ProjectTable::appendVisibleProject(Project* project) {
    if (!project) return;
    projects_.push_back(project);
    rows(static_cast<int>(projects_.size()));
    row_height(rows() - 1, 38);
    selectAndScrollToRow(rows() - 1);
}

int ProjectTable::findRowByProjectId(int projectId) const {
    for (int i = 0; i < static_cast<int>(projects_.size()); ++i) {
        if (projects_[static_cast<std::size_t>(i)]->id == projectId) {
            return i;
        }
    }
    return -1;
}

void ProjectTable::setColumnWidths(const int* widths) {
    if (!widths) return;
    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        int width = std::max(48, widths[c]);
        col_width(c, width);
    }
    rememberColumnWidths();
    redraw();
}

void ProjectTable::copyColumnWidths(int* widths) {
    if (!widths) return;
    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        widths[c] = col_width(c);
    }
}

void ProjectTable::setSortIndicator(int column, bool ascending) {
    sortColumn_ = column;
    sortAscending_ = ascending;
    redraw();
}

void ProjectTable::sortCurrent(const std::function<bool(const Project*, const Project*)>& less) {
    if (!less) return;
    if (editing_) {
        commitEdit();
    }
    std::stable_sort(projects_.begin(), projects_.end(), less);
    redraw();
}

void ProjectTable::rememberColumnWidths() {
    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        lastWidths_[c] = col_width(c);
    }
}

void ProjectTable::notifyColumnWidthsChanged() {
    bool changed = false;
    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        if (col_width(c) != lastWidths_[c]) {
            changed = true;
            break;
        }
    }
    if (!changed) return;
    rememberColumnWidths();
    if (onColumnWidthsChanged) {
        onColumnWidthsChanged();
    }
}

int ProjectTable::selectedRow() {
    for (int r = 0; r < rows(); ++r) {
        if (row_selected(r)) return r;
    }
    return -1;
}

void ProjectTable::moveSelectionBy(int delta) {
    if (rows() <= 0 || delta == 0) return;
    int cur = selectedRow();
    int next = cur < 0 ? (delta < 0 ? rows() - 1 : 0) : cur + delta;
    if (next < 0) next = 0;
    if (next >= rows()) next = rows() - 1;
    if (next != cur) {
        selectAndScrollToRow(next);
    }
}

void ProjectTable::moveSelectionPage(int direction) {
    if (rows() <= 0 || direction == 0) return;
    int page = 1;
    if (botrow >= toprow) {
        page = std::max(1, botrow - toprow);
        if (page > 1) --page;
    }
    moveSelectionBy(direction * page);
}

void ProjectTable::selectAndScrollToRow(int row) {
    if (row < 0 || row >= rows()) return;
    select_all_rows(0);
    select_row(row, 1);
    current_row = row;
    Fl_Table::select_row = row;
    current_col = static_cast<int>(Column::Name);
    select_col = current_col;
    if (botrow >= toprow) {
        if (row < toprow) {
            row_position(row);
        } else if (row > botrow) {
            int visible = std::max(1, botrow - toprow);
            row_position(std::max(0, row - visible + 1));
        }
    }
    redraw();
}

bool ProjectTable::isEditableTextColumn(int col) const {
    return col == static_cast<int>(Column::Name) ||
           col == static_cast<int>(Column::Description) ||
           col == static_cast<int>(Column::Status) ||
           col == static_cast<int>(Column::FolderPath);
}

void ProjectTable::beginTextEdit(int row, int col) {
    if (row < 0 || row >= static_cast<int>(projects_.size())) return;
    if (!isEditableTextColumn(col)) return;

    if (editing_) {
        commitEdit();
    }

    int X = 0, Y = 0, W = 0, H = 0;
    find_cell(CONTEXT_CELL, row, col, X, Y, W, H);
    if (W <= 2 || H <= 2) return;

    const Project* p = projects_[static_cast<std::size_t>(row)];
    std::string currentText;
    switch (static_cast<Column>(col)) {
        case Column::Name:        currentText = p->name; break;
        case Column::Description: currentText = p->description; break;
        case Column::Status:      currentText = p->status; break;
        case Column::FolderPath:  currentText = p->folderPath; break;
        default: break;
    }

    editing_ = true;
    editRow_ = row;
    editCol_ = col;

    editInput_->resize(X + 1, Y + 1, W - 2, H - 2);
    editInput_->value(currentText.c_str());
    editInput_->insert_position(static_cast<int>(currentText.size()), 0);
    editInput_->show();
    editInput_->take_focus();
    redraw();
}

void ProjectTable::commitEdit() {
    if (!editing_) return;

    int row = editRow_;
    int col = editCol_;
    std::string newVal = editInput_->value() ? editInput_->value() : "";

    editing_ = false;
    editRow_ = -1;
    editCol_ = -1;
    editInput_->hide();
    // Поле ввода прячет курсор на время набора. Таблица помнит прошлый
    // курсор и после скрытия поля сама его не возвращает.
    if (Fl_Window* top = window()) {
        top->cursor(FL_CURSOR_ARROW);
    }

    if (row >= 0 && row < static_cast<int>(projects_.size())) {
        Project* p = projects_[static_cast<std::size_t>(row)];
        bool changed = false;
        switch (static_cast<Column>(col)) {
            case Column::Name:
                if (p->name != newVal) { p->name = newVal; changed = true; }
                break;
            case Column::Description:
                if (p->description != newVal) { p->description = newVal; changed = true; }
                break;
            case Column::Status:
                if (p->status != newVal) { p->status = newVal; changed = true; }
                break;
            case Column::FolderPath:
                if (p->folderPath != newVal) { p->folderPath = newVal; changed = true; }
                break;
            default:
                break;
        }

        int projectId = p->id;
        if (changed && onCellEdited) {
            onCellEdited(projectId, static_cast<Column>(col), newVal);
        }
    }

    redraw();
}

void ProjectTable::cancelEdit() {
    if (!editing_) return;
    editing_ = false;
    editRow_ = -1;
    editCol_ = -1;
    editInput_->hide();
    if (Fl_Window* top = window()) {
        top->cursor(FL_CURSOR_ARROW);
    }
    take_focus();
    redraw();
}

void ProjectTable::moveEditToNextCell(bool backward) {
    if (!editing_) return;
    int row = editRow_;
    int col = editCol_;
    commitEdit();

    static const int editableCols[] = {
        static_cast<int>(Column::Name),
        static_cast<int>(Column::Description),
        static_cast<int>(Column::Status),
        static_cast<int>(Column::FolderPath)
    };
    constexpr int numEditable = 4;

    int idx = 0;
    for (int i = 0; i < numEditable; ++i) {
        if (editableCols[i] == col) {
            idx = i;
            break;
        }
    }

    if (backward) {
        if (idx > 0) {
            beginTextEdit(row, editableCols[idx - 1]);
        } else if (row > 0) {
            select_all_rows(0);
            select_row(row - 1, 1);
            beginTextEdit(row - 1, editableCols[numEditable - 1]);
        }
    } else {
        if (idx + 1 < numEditable) {
            beginTextEdit(row, editableCols[idx + 1]);
        } else if (row + 1 < static_cast<int>(projects_.size())) {
            select_all_rows(0);
            select_row(row + 1, 1);
            beginTextEdit(row + 1, editableCols[0]);
        }
    }
}

void ProjectTable::openLanguagePopup(int row, int col) {
    if (row < 0 || row >= static_cast<int>(projects_.size())) return;
    commitEdit();

    int X = 0, Y = 0, W = 0, H = 0;
    find_cell(CONTEXT_CELL, row, col, X, Y, W, H);

    const std::string& currentLang = projects_[static_cast<std::size_t>(row)]->language;
    const auto& langs = availableLanguages();
    std::vector<Fl_Menu_Item> items(langs.size() + 1);
    const Fl_Menu_Item* initialItem = nullptr;
    for (std::size_t i = 0; i < langs.size(); ++i) {
        items[i] = {};
        items[i].text = langs[i].empty() ? "(не выбран)" : langs[i].c_str();
        items[i].user_data_ = reinterpret_cast<void*>(static_cast<std::intptr_t>(i));
        items[i].labeltype_ = FL_NORMAL_LABEL;
        items[i].labelfont_ = FL_HELVETICA;
        items[i].labelsize_ = 14;
        items[i].labelcolor_ = theme_.text;
        if (langs[i] == currentLang) {
            initialItem = &items[i];
        }
    }
    items.back() = {};

    const Fl_Menu_Item* chosen = items.data()->popup(X, Y + H, nullptr, initialItem, nullptr);
    if (!chosen) return;

    std::size_t idx = static_cast<std::size_t>(reinterpret_cast<std::intptr_t>(chosen->user_data_));
    if (idx < langs.size() && row < static_cast<int>(projects_.size())) {
        Project* p = projects_[static_cast<std::size_t>(row)];
        const std::string& newLang = langs[idx];
        if (p->language != newLang) {
            p->language = newLang;
            int projectId = p->id;
            if (onCellEdited) {
                onCellEdited(projectId, Column::Language, newLang);
            }
            redraw();
        }
    }
}

void ProjectTable::openContextMenuForKeyboard() {
    if (projects_.empty()) return;
    int row = selectedRow();
    if (row < 0) {
        row = 0;
        selectAndScrollToRow(row);
    } else if (botrow >= toprow && (row < toprow || row > botrow)) {
        selectAndScrollToRow(row);
    }

    int X = 0, Y = 0, W = 0, H = 0;
    find_cell(CONTEXT_CELL, row, static_cast<int>(Column::Name), X, Y, W, H);
    int menuX = X + 16;
    int menuY = Y + H - 2;
    int left = x() + 8;
    int right = x() + w() - 32;
    if (menuX < left) menuX = left;
    if (menuX > right) menuX = right;
    openContextMenu(row, menuX, menuY);
}

void ProjectTable::openContextMenu(int row, int menuX, int menuY) {
    if (row < 0 || row >= static_cast<int>(projects_.size())) return;
    commitEdit();
    if (row < 0 || row >= static_cast<int>(projects_.size())) return;

    const Project* project = projects_[static_cast<std::size_t>(row)];
    const bool hasFolder = !project->folderPath.empty();

    struct ItemDef {
        const char* text;
        Fl_Image* image;
        int code;
        int flags;
    };
    const ItemDef defs[] = {
        {" Открыть папку", icons_.folder(), static_cast<int>(Action::OpenFolder), hasFolder ? 0 : FL_MENU_INACTIVE},
        {" Открыть в редакторе", icons_.code(), static_cast<int>(Action::OpenEditor), hasFolder ? 0 : FL_MENU_INACTIVE},
        {" Запустить", icons_.play(), static_cast<int>(Action::RunScript),
         (hasFolder ? 0 : FL_MENU_INACTIVE) | FL_MENU_DIVIDER},
        {" Удалить", icons_.trash(), -1, 0},
    };
    constexpr int kItemCount = 4;

    Fl_Menu_Item menu[kItemCount + 1] = {};
    Fl_Multi_Label labels[kItemCount] = {};
    for (int i = 0; i < kItemCount; ++i) {
        labels[i].labela = reinterpret_cast<const char*>(defs[i].image);
        labels[i].typea = FL_IMAGE_LABEL;
        labels[i].labelb = defs[i].text;
        labels[i].typeb = FL_NORMAL_LABEL;

        menu[i].multi_label(&labels[i]);
        menu[i].labelfont_ = FL_HELVETICA;
        menu[i].labelsize_ = 14;
        menu[i].labelcolor_ = theme_.text;
        menu[i].flags = defs[i].flags;
        menu[i].user_data_ = reinterpret_cast<void*>(static_cast<std::intptr_t>(defs[i].code));
    }

    const Fl_Menu_Item* chosen = menu->popup(menuX, menuY, nullptr, nullptr, nullptr);
    if (!chosen || row < 0 || row >= static_cast<int>(projects_.size())) return;

    int projectId = projects_[static_cast<std::size_t>(row)]->id;
    int code = static_cast<int>(reinterpret_cast<std::intptr_t>(chosen->user_data_));
    if (code < 0) {
        if (onDeleteProject) {
            onDeleteProject(projectId);
        }
        return;
    }
    if (onAction) {
        onAction(projectId, static_cast<Action>(code));
    }
}

void ProjectTable::updateHover(int row, int col) {
    if (row == hoverRow_ && col == hoverCol_) return;

    int oldRow = hoverRow_;
    int oldCol = hoverCol_;
    hoverRow_ = row;
    hoverCol_ = col;

    auto isLanguage = [](int c) { return c == static_cast<int>(Column::Language); };
    if (oldRow >= 0 && oldRow < rows() && isLanguage(oldCol)) {
        redraw_range(oldRow, oldRow, oldCol, oldCol);
    }
    if (hoverRow_ >= 0 && hoverRow_ < rows() && isLanguage(hoverCol_)) {
        redraw_range(hoverRow_, hoverRow_, hoverCol_, hoverCol_);
    }

    if (col == static_cast<int>(Column::Git) && row >= 0 && row < static_cast<int>(projects_.size())) {
        copy_tooltip(projects_[static_cast<std::size_t>(row)]->gitTooltip.c_str());
    } else if (oldCol == static_cast<int>(Column::Git)) {
        copy_tooltip(nullptr);
    }
}

void ProjectTable::refreshHoverTooltip() {
    if (hoverCol_ != static_cast<int>(Column::Git)) return;
    if (hoverRow_ < 0 || hoverRow_ >= static_cast<int>(projects_.size())) return;
    copy_tooltip(projects_[static_cast<std::size_t>(hoverRow_)]->gitTooltip.c_str());
}

void ProjectTable::clearHover() {
    if (hoverRow_ == -1 && hoverCol_ == -1) return;
    int oldRow = hoverRow_;
    int oldCol = hoverCol_;
    hoverRow_ = -1;
    hoverCol_ = -1;
    if (oldCol == static_cast<int>(Column::Git)) {
        copy_tooltip(nullptr);
    }
    if (oldRow >= 0 && oldRow < rows() && oldCol == static_cast<int>(Column::Language)) {
        redraw_range(oldRow, oldRow, oldCol, oldCol);
    }
}

void ProjectTable::draw() {
    Fl_Table_Row::draw();
    fl_color(theme_.gridLine);
    fl_rect(x(), y(), w(), h());
}

void ProjectTable::resize(int X, int Y, int W, int H) {
    if (editing_) {
        commitEdit();
    }
    Fl_Table_Row::resize(X, Y, W, H);
}

void ProjectTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    switch (context) {
        case CONTEXT_STARTPAGE:
            fl_font(FL_HELVETICA, 14);
            return;

        case CONTEXT_COL_HEADER: {
            fl_push_clip(X, Y, W, H);
            bool sortable = C >= 0 && C < static_cast<int>(Column::Count);
            bool headerHot = sortable && C == hoverHeaderCol_;
            fl_color(headerHot ? theme_.buttonHoverBg : theme_.headerBg);
            fl_rectf(X, Y, W, H);

            fl_color(theme_.gridLine);
            fl_line(X + W - 1, Y, X + W - 1, Y + H - 1);
            fl_line(X, Y + H - 1, X + W - 1, Y + H - 1);

            fl_font(FL_HELVETICA_BOLD, 13);
            fl_color(theme_.text);
            if (C >= 0 && C < static_cast<int>(Column::Count)) {
                bool sorted = C == sortColumn_;
                int rightPad = sorted ? 22 : 8;
                fl_draw(kColumnHeaders[C], X + 8, Y, std::max(0, W - 8 - rightPad), H,
                        FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

                if (sorted && W > 28) {
                    int ax = X + W - 16;
                    int mid = Y + H / 2;
                    if (sortAscending_) {
                        fl_polygon(ax, mid + 3, ax + 8, mid + 3, ax + 4, mid - 4);
                    } else {
                        fl_polygon(ax, mid - 3, ax + 8, mid - 3, ax + 4, mid + 4);
                    }
                }
            }
            fl_pop_clip();
            return;
        }

        case CONTEXT_CELL: {
            if (R < 0 || R >= static_cast<int>(projects_.size())) return;

            if (editing_ && R == editRow_ && C == editCol_) {
                if (editInput_->x() != X + 1 || editInput_->y() != Y + 1 ||
                    editInput_->w() != W - 2 || editInput_->h() != H - 2) {
                    editInput_->resize(X + 1, Y + 1, std::max(10, W - 2), std::max(10, H - 2));
                }
                return;
            }

            const Project* p = projects_[static_cast<std::size_t>(R)];
            bool selected = row_selected(R) != 0;

            fl_push_clip(X, Y, W, H);

            // Background
            Fl_Color bg = selected ? theme_.selectionBg : ((R % 2 == 0) ? theme_.cellBg : theme_.cellAltBg);
            fl_color(bg);
            fl_rectf(X, Y, W, H);

            // Grid lines (right and bottom)
            fl_color(theme_.gridLine);
            fl_line(X + W - 1, Y, X + W - 1, Y + H - 1);
            fl_line(X, Y + H - 1, X + W - 1, Y + H - 1);

            if (static_cast<Column>(C) == Column::Git) {
                drawGitStatus(p, selected, X, Y, W, H, theme_, icons_);
                fl_pop_clip();
                return;
            }

            std::string text;
            bool isMuted = false;
            switch (static_cast<Column>(C)) {
                case Column::Name:            text = p->name; break;
                case Column::Description:     text = p->description; break;
                case Column::Status:          text = p->status; break;
                case Column::Language:        text = p->language; break;
                case Column::LastInteraction:
                    text = p->lastInteractionDisplay();
                    isMuted = p->lastInteractionIso.empty();
                    break;
                case Column::FolderPath:      text = p->folderPath; break;
                default: break;
            }

            fl_font(FL_HELVETICA, 14);
            fl_color(selected ? theme_.selectionText : (isMuted ? theme_.mutedText : theme_.text));

            int rightPad = (C == static_cast<int>(Column::Language)) ? 22 : 8;
            fl_draw(text.c_str(), X + 8, Y, std::max(0, W - 8 - rightPad), H,
                    FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

            if (C == static_cast<int>(Column::Language) &&
                (selected || (R == hoverRow_ && C == hoverCol_))) {
                int ax = X + W - 14;
                int ay = Y + H / 2 - 2;
                fl_color(selected ? theme_.selectionText : theme_.mutedText);
                fl_polygon(ax, ay, ax + 8, ay, ax + 4, ay + 5);
            }

            fl_pop_clip();
            return;
        }

        default:
            return;
    }
}

int ProjectTable::handle(int event) {
    if (event == FL_MOUSEWHEEL && editing_) {
        commitEdit();
    }

    switch (event) {
        case FL_MOVE: {
            int R = 0, C = 0;
            ResizeFlag rf = RESIZE_NONE;
            TableContext ctx = cursor2rowcol(R, C, rf);
            int newHeader = -1;
            if (ctx == CONTEXT_COL_HEADER && rf == RESIZE_NONE &&
                C >= 0 && C < static_cast<int>(Column::Count)) {
                newHeader = C;
            }
            if (newHeader != hoverHeaderCol_) {
                hoverHeaderCol_ = newHeader;
                redraw();
            }
            if (ctx == CONTEXT_CELL && rf == RESIZE_NONE && R >= 0 && R < rows()) {
                updateHover(R, C);
            } else {
                clearHover();
            }
            break;
        }

        case FL_LEAVE:
            if (hoverHeaderCol_ != -1) {
                hoverHeaderCol_ = -1;
                redraw();
            }
            clearHover();
            break;

        case FL_PUSH: {
            headerPressCol_ = -1;

            // If clicking inside active editor input, let Fl_Table_Row route to child
            if (editing_ && Fl::event_inside(editInput_)) {
                return Fl_Table_Row::handle(event);
            }

            {
                int R = 0, C = 0;
                ResizeFlag rf = RESIZE_NONE;
                TableContext ctx = cursor2rowcol(R, C, rf);
                if (ctx == CONTEXT_COL_HEADER && rf == RESIZE_NONE && Fl::event_button() == FL_LEFT_MOUSE) {
                    if (editing_) {
                        commitEdit();
                    }
                    if (C >= 0 && C < static_cast<int>(Column::Count)) {
                        headerPressCol_ = C;
                        headerPressX_ = Fl::event_x();
                        headerPressY_ = Fl::event_y();
                        return 1;
                    }
                }
            }

            if (editing_) {
                commitEdit();
            }

            int R = 0, C = 0;
            ResizeFlag rf = RESIZE_NONE;
            TableContext ctx = cursor2rowcol(R, C, rf);

            if (ctx == CONTEXT_CELL && rf == RESIZE_NONE && R >= 0 && R < rows()) {
                if (Fl::event_button() == FL_RIGHT_MOUSE) {
                    take_focus();
                    selectAndScrollToRow(R);
                    openContextMenu(R, Fl::event_x(), Fl::event_y());
                    return 1;
                }

                if (Fl::event_button() == FL_LEFT_MOUSE) {
                    take_focus();
                    if (C == static_cast<int>(Column::Language)) {
                        bool wasSelected = row_selected(R) != 0;
                        select_all_rows(0);
                        select_row(R, 1);
                        redraw();
                        if (Fl::event_clicks() > 0 || wasSelected) {
                            openLanguagePopup(R, C);
                        }
                        return 1;
                    } else if (isEditableTextColumn(C) && Fl::event_clicks() > 0) {
                        select_all_rows(0);
                        select_row(R, 1);
                        beginTextEdit(R, C);
                        return 1;
                    }
                }
            }
            break;
        }

        case FL_RELEASE: {
            if (headerPressCol_ >= 0) {
                int col = headerPressCol_;
                headerPressCol_ = -1;
                int dx = std::abs(Fl::event_x() - headerPressX_);
                int dy = std::abs(Fl::event_y() - headerPressY_);
                if (dx < 5 && dy < 5 && onHeaderClicked) {
                    onHeaderClicked(col);
                }
                return 1;
            }
            break;
        }

        case FL_KEYDOWN: {
            if (!editing_) {
                int key = Fl::event_key();
                bool shift = (Fl::event_state() & FL_SHIFT) != 0;
                if (key == FL_Menu || (key == (FL_F + 10) && shift)) {
                    openContextMenuForKeyboard();
                    return 1;
                }
                if (key == FL_Up) {
                    moveSelectionBy(-1);
                    return 1;
                }
                if (key == FL_Down) {
                    moveSelectionBy(1);
                    return 1;
                }
                if (key == FL_Home) {
                    if (rows() > 0) selectAndScrollToRow(0);
                    return 1;
                }
                if (key == FL_End) {
                    if (rows() > 0) selectAndScrollToRow(rows() - 1);
                    return 1;
                }
                if (key == FL_Page_Up) {
                    moveSelectionPage(-1);
                    return 1;
                }
                if (key == FL_Page_Down) {
                    moveSelectionPage(1);
                    return 1;
                }
                if (key == FL_Enter || key == FL_KP_Enter || key == (FL_F + 2)) {
                    int row = selectedRow();
                    if (row >= 0) {
                        beginTextEdit(row, static_cast<int>(Column::Name));
                        return 1;
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    int handled = Fl_Table_Row::handle(event);
    if (event == FL_RELEASE) {
        notifyColumnWidthsChanged();
    } else if (event == FL_MOVE) {
        int R = 0, C = 0;
        ResizeFlag rf = RESIZE_NONE;
        TableContext ctx = cursor2rowcol(R, C, rf);
        if (ctx == CONTEXT_COL_HEADER && rf == RESIZE_NONE &&
            C >= 0 && C < static_cast<int>(Column::Count)) {
            fl_cursor(FL_CURSOR_HAND);
        }
    }
    return handled;
}

} // namespace ProjectManager
