#include "UI/ProjectTable.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cstdint>
#include <string>

namespace ProjectManager {

namespace {

constexpr const char* kColumnHeaders[] = {
    "Действия",
    "Название проекта",
    "Описание",
    "Статус",
    "Тип",
    "Последнее взаимодействие",
    "Директория"
};

constexpr int kInitialColWidths[] = {
    120, // Действия
    180, // Название проекта
    270, // Описание
    120, // Статус
    115, // Тип
    195, // Последнее взаимодействие
    240  // Директория
};

constexpr int kActionBtnSize = 28;
constexpr int kActionBtnGap = 6;
constexpr int kIconSize = 16;

constexpr const char* kActionTooltips[] = {
    "Открыть папку с проектом",
    "Открыть проект в редакторе",
    "Запустить проект"
};

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
    col_resize_min(60);

    for (int c = 0; c < static_cast<int>(Column::Count); ++c) {
        col_width(c, kInitialColWidths[c]);
    }

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
    cancelEdit();
    clearHover();
    pressedRow_ = -1;
    pressedAction_ = -1;

    projects_ = std::move(visibleProjects);
    rows(static_cast<int>(projects_.size()));
    row_height_all(38);
    redraw();
}

void ProjectTable::appendVisibleProject(Project* project) {
    if (!project) return;
    projects_.push_back(project);
    rows(static_cast<int>(projects_.size()));
    row_height(rows() - 1, 38);
    selectAndScrollToRow(rows() - 1);
}

void ProjectTable::selectAndScrollToRow(int row) {
    if (row < 0 || row >= rows()) return;
    select_all_rows(0);
    select_row(row, 1);
    if (row < toprow || row > botrow) {
        row_position(row);
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

void ProjectTable::openContextMenu(int row, int menuX, int menuY) {
    if (row < 0 || row >= static_cast<int>(projects_.size())) return;
    commitEdit();

    Fl_Menu_Item menu[2] = {};
    menu[0].text = "Удалить строку";
    menu[0].labeltype_ = FL_NORMAL_LABEL;
    menu[0].labelfont_ = FL_HELVETICA;
    menu[0].labelsize_ = 14;
    menu[0].labelcolor_ = theme_.text;

    const Fl_Menu_Item* chosen = menu->popup(menuX, menuY, nullptr, nullptr, nullptr);
    if (chosen == &menu[0] && row < static_cast<int>(projects_.size())) {
        int projectId = projects_[static_cast<std::size_t>(row)]->id;
        if (onDeleteProject) {
            onDeleteProject(projectId);
        }
    }
}

void ProjectTable::actionButtonRect(int cellX, int cellY, int cellW, int cellH, int actionIdx,
                                    int& bx, int& by, int& bw, int& bh) const {
    constexpr int totalW = 3 * kActionBtnSize + 2 * kActionBtnGap;
    int startX = cellX + (cellW - totalW) / 2;
    int startY = cellY + (cellH - kActionBtnSize) / 2;

    bx = startX + actionIdx * (kActionBtnSize + kActionBtnGap);
    by = startY;
    bw = kActionBtnSize;
    bh = kActionBtnSize;
}

bool ProjectTable::hitTestActionButton(int row, int mouseX, int mouseY, int& outAction) const {
    outAction = -1;
    if (row < 0 || row >= static_cast<int>(projects_.size())) return false;

    int X = 0, Y = 0, W = 0, H = 0;
    const_cast<ProjectTable*>(this)->find_cell(CONTEXT_CELL, row, static_cast<int>(Column::Actions), X, Y, W, H);

    for (int i = 0; i < 3; ++i) {
        int bx = 0, by = 0, bw = 0, bh = 0;
        actionButtonRect(X, Y, W, H, i, bx, by, bw, bh);
        if (mouseX >= bx && mouseX < bx + bw && mouseY >= by && mouseY < by + bh) {
            outAction = i;
            return true;
        }
    }
    return false;
}

void ProjectTable::updateHover(int row, int col, int mouseX, int mouseY) {
    int newAction = -1;
    if (row >= 0 && col == static_cast<int>(Column::Actions)) {
        hitTestActionButton(row, mouseX, mouseY, newAction);
    }

    if (row == hoverRow_ && col == hoverCol_ && newAction == hoverAction_) {
        return;
    }

    int oldRow = hoverRow_;
    int oldCol = hoverCol_;
    int oldAction = hoverAction_;
    hoverRow_ = row;
    hoverCol_ = col;
    hoverAction_ = newAction;

    if (hoverAction_ >= 0 && hoverAction_ < 3) {
        int cx = 0, cy = 0, cw = 0, ch = 0;
        find_cell(CONTEXT_CELL, hoverRow_, static_cast<int>(Column::Actions), cx, cy, cw, ch);
        int bx = 0, by = 0, bw = 0, bh = 0;
        actionButtonRect(cx, cy, cw, ch, hoverAction_, bx, by, bw, bh);
        Fl_Tooltip::enter_area(this, bx - x(), by - y(), bw, bh, kActionTooltips[hoverAction_]);
    } else {
        Fl_Tooltip::exit(this);
    }

    auto cellUsesHover = [](int c, int act) {
        return (c == static_cast<int>(Column::Actions) && act >= 0) ||
               c == static_cast<int>(Column::Language);
    };

    if (oldRow >= 0 && oldRow < rows() && oldCol >= 0 && cellUsesHover(oldCol, oldAction)) {
        redraw_range(oldRow, oldRow, oldCol, oldCol);
    }
    if (hoverRow_ >= 0 && hoverRow_ < rows() && hoverCol_ >= 0 && cellUsesHover(hoverCol_, hoverAction_)) {
        redraw_range(hoverRow_, hoverRow_, hoverCol_, hoverCol_);
    }
}

void ProjectTable::clearHover() {
    if (hoverRow_ == -1 && hoverCol_ == -1 && hoverAction_ == -1) return;
    int oldRow = hoverRow_;
    int oldCol = hoverCol_;
    int oldAction = hoverAction_;
    hoverRow_ = -1;
    hoverCol_ = -1;
    hoverAction_ = -1;
    Fl_Tooltip::exit(this);
    if (oldRow >= 0 && oldRow < rows() && oldCol >= 0 &&
        ((oldCol == static_cast<int>(Column::Actions) && oldAction >= 0) ||
         oldCol == static_cast<int>(Column::Language))) {
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
            fl_color(theme_.headerBg);
            fl_rectf(X, Y, W, H);

            fl_color(theme_.gridLine);
            fl_line(X + W - 1, Y, X + W - 1, Y + H - 1);
            fl_line(X, Y + H - 1, X + W - 1, Y + H - 1);

            fl_font(FL_HELVETICA_BOLD, 13);
            fl_color(theme_.text);
            if (C >= 0 && C < static_cast<int>(Column::Count)) {
                Fl_Align align = (C == static_cast<int>(Column::Actions))
                                     ? (FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP)
                                     : (FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
                fl_draw(kColumnHeaders[C], X + 8, Y, W - 16, H, align);
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

            if (C == static_cast<int>(Column::Actions)) {
                Fl_SVG_Image* imgs[3] = {icons_.folder(), icons_.code(), icons_.play()};
                for (int i = 0; i < 3; ++i) {
                    int bx = 0, by = 0, bw = 0, bh = 0;
                    actionButtonRect(X, Y, W, H, i, bx, by, bw, bh);

                    bool isPressed = (R == pressedRow_ && i == pressedAction_);
                    bool isHovered = (R == hoverRow_ && i == hoverAction_);

                    Fl_Color btnBg = isPressed ? theme_.buttonActiveBg
                                               : (isHovered ? theme_.buttonHoverBg : theme_.buttonBg);
                    fl_color(btnBg);
                    fl_rectf(bx, by, bw, bh);

                    fl_color(isHovered ? theme_.separator : theme_.buttonBorder);
                    fl_rect(bx, by, bw, bh);

                    if (imgs[i]) {
                        int ix = bx + (bw - kIconSize) / 2;
                        int iy = by + (bh - kIconSize) / 2;
                        imgs[i]->draw(ix, iy);
                    }
                }
            } else {
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

                // Subtle dropdown arrow on Language column when hovered or row selected
                if (C == static_cast<int>(Column::Language) &&
                    (selected || (R == hoverRow_ && C == hoverCol_))) {
                    int ax = X + W - 14;
                    int ay = Y + H / 2 - 2;
                    fl_color(selected ? theme_.selectionText : theme_.mutedText);
                    fl_polygon(ax, ay, ax + 8, ay, ax + 4, ay + 5);
                }
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
            if (ctx == CONTEXT_CELL && rf == RESIZE_NONE && R >= 0 && R < rows()) {
                updateHover(R, C, Fl::event_x(), Fl::event_y());
            } else {
                clearHover();
            }
            break;
        }

        case FL_LEAVE:
            clearHover();
            break;

        case FL_PUSH: {
            // If clicking inside active editor input, let Fl_Table_Row route to child
            if (editing_ && Fl::event_inside(editInput_)) {
                return Fl_Table_Row::handle(event);
            }

            if (editing_) {
                commitEdit();
            }

            int R = 0, C = 0;
            ResizeFlag rf = RESIZE_NONE;
            TableContext ctx = cursor2rowcol(R, C, rf);

            if (ctx == CONTEXT_CELL && rf == RESIZE_NONE && R >= 0 && R < rows()) {
                if (Fl::event_button() == FL_RIGHT_MOUSE) {
                    select_all_rows(0);
                    select_row(R, 1);
                    redraw();
                    openContextMenu(R, Fl::event_x(), Fl::event_y());
                    return 1;
                }

                if (Fl::event_button() == FL_LEFT_MOUSE) {
                    if (C == static_cast<int>(Column::Actions)) {
                        int actionIdx = -1;
                        if (hitTestActionButton(R, Fl::event_x(), Fl::event_y(), actionIdx)) {
                            select_all_rows(0);
                            select_row(R, 1);
                            pressedRow_ = R;
                            pressedAction_ = actionIdx;
                            redraw_range(R, R, 0, 0);
                            return 1;
                        }
                    } else if (C == static_cast<int>(Column::Language)) {
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
            if (pressedAction_ >= 0) {
                int pRow = pressedRow_;
                int pAct = pressedAction_;
                pressedRow_ = -1;
                pressedAction_ = -1;
                if (pRow >= 0 && pRow < rows()) {
                    redraw_range(pRow, pRow, 0, 0);
                }

                int hitAction = -1;
                if (pRow >= 0 && pRow < static_cast<int>(projects_.size()) &&
                    hitTestActionButton(pRow, Fl::event_x(), Fl::event_y(), hitAction) &&
                    hitAction == pAct) {
                    int projectId = projects_[static_cast<std::size_t>(pRow)]->id;
                    if (onAction) {
                        onAction(projectId, static_cast<Action>(pAct));
                    }
                }
                return 1;
            }
            break;
        }

        case FL_KEYDOWN: {
            if (!editing_) {
                int key = Fl::event_key();
                if (key == FL_Enter || key == FL_KP_Enter || key == (FL_F + 2)) {
                    for (int r = 0; r < rows(); ++r) {
                        if (row_selected(r)) {
                            beginTextEdit(r, static_cast<int>(Column::Name));
                            return 1;
                        }
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    return Fl_Table_Row::handle(event);
}

} // namespace ProjectManager
