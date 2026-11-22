#include "UI/MainWindow.hpp"

#include "Services/CodeEditorService.hpp"
#include "Services/GitStatusService.hpp"
#include "Services/PlatformService.hpp"
#include "Services/ProjectService.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#include <algorithm>
#include <ctime>
#include <string>

namespace ProjectManager {

std::atomic<MainWindow*> MainWindow::gitUiOwner_{nullptr};

static_assert(static_cast<int>(ProjectTable::Column::Count) == kSettingsColumnCount,
              "Column count and saved settings layout must match");

namespace {

std::string escapeMenuText(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 4);
    for (char ch : text) {
        if (ch == '/' || ch == '\\' || ch == '&' || ch == '_') {
            out.push_back('\\');
        }
        out.push_back(ch);
    }
    return out;
}

class ToolbarButton : public Fl_Button {
public:
    ToolbarButton(int X, int Y, int W, int H, const Theme& theme)
        : Fl_Button(X, Y, W, H)
        , theme_(theme) {
        box(FL_BORDER_BOX);
        color(theme_.buttonBg, theme_.buttonActiveBg);
        clear_visible_focus();
    }

    int handle(int event) override {
        if (event == FL_ENTER) {
            hovered_ = true;
            color(theme_.buttonHoverBg, theme_.buttonActiveBg);
            redraw();
        } else if (event == FL_LEAVE) {
            hovered_ = false;
            color(theme_.buttonBg, theme_.buttonActiveBg);
            redraw();
        }
        return Fl_Button::handle(event);
    }

private:
    const Theme& theme_;
    bool hovered_ = false;
};

bool titleBarOnSomeScreen(int x, int y, int w, int /*h*/) {
    if (Fl::screen_count() <= 0) return true;
    for (int i = 0; i < Fl::screen_count(); ++i) {
        int sx = 0, sy = 0, sw = 0, sh = 0;
        Fl::screen_work_area(sx, sy, sw, sh, i);
        int left = std::max(x, sx);
        int top = std::max(y, sy);
        int right = std::min(x + w, sx + sw);
        int bottom = std::min(y + 36, sy + sh);
        if (right - left >= 80 && bottom - top >= 12) return true;
    }
    return false;
}

bool missingInteraction(const Project* project) {
    if (!project || project->lastInteractionIso.empty()) return true;
    std::time_t parsed = 0;
    bool hasTimezone = false;
    return !TimeUtil::parseIso8601ToUtc(project->lastInteractionIso, parsed, hasTimezone);
}

int gitRank(const Project* project) {
    if (!project || project->gitSync == GitSync::Unknown) return 50;
    if (project->gitSync == GitSync::Unavailable) return 40;
    if (project->gitSync == GitSync::Disabled) return 35;
    if (project->gitSync == GitSync::NotARepo) return 30;
    int rank = 0;
    if (project->gitSync == GitSync::NotSynced) rank += 2;
    if (project->gitDirty) rank += 1;
    return rank;
}

bool isNewerIso(const std::string& candidate, const std::string& current) {
    if (candidate.empty()) return false;
    std::time_t next = 0;
    bool hasNext = false;
    if (!TimeUtil::parseIso8601ToUtc(candidate, next, hasNext)) return false;
    if (current.empty()) return true;
    std::time_t prev = 0;
    bool hasPrev = false;
    if (!TimeUtil::parseIso8601ToUtc(current, prev, hasPrev)) return true;
    return next > prev;
}

int compareText(const std::string& left, const std::string& right) {
    const std::string a = PlatformService::toLowerUtf8(left);
    const std::string b = PlatformService::toLowerUtf8(right);
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int compareProjects(const Project* a, const Project* b, int column) {
    switch (static_cast<ProjectTable::Column>(column)) {
        case ProjectTable::Column::Name:
            return compareText(a->name, b->name);
        case ProjectTable::Column::Description:
            return compareText(a->description, b->description);
        case ProjectTable::Column::Status:
            return compareText(a->status, b->status);
        case ProjectTable::Column::Language:
            return compareText(a->language, b->language);
        case ProjectTable::Column::Git: {
            const int left = gitRank(a);
            const int right = gitRank(b);
            if (left < right) return -1;
            if (left > right) return 1;
            return 0;
        }
        case ProjectTable::Column::FolderPath:
            return compareText(a->folderPath, b->folderPath);
        case ProjectTable::Column::LastInteraction: {
            std::time_t ta = 0;
            std::time_t tb = 0;
            bool hasA = false;
            bool hasB = false;
            TimeUtil::parseIso8601ToUtc(a->lastInteractionIso, ta, hasA);
            TimeUtil::parseIso8601ToUtc(b->lastInteractionIso, tb, hasB);
            if (ta < tb) return -1;
            if (ta > tb) return 1;
            return 0;
        }
        default:
            return 0;
    }
}

std::string normalizeEditorPath(std::string path) {
    path = PlatformService::trim(path);
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = PlatformService::trim(path.substr(1, path.size() - 2));
    }
    return path;
}

bool editCodeEditorPath(const Theme& theme, Fl_Window* parent, std::string& path, bool& gitStatusEnabled) {
    struct State {
        bool accepted = false;
        Fl_Input* input = nullptr;
        Fl_Check_Button* gitStatus = nullptr;
    } state;

    Fl_Window dlg(540, 262, "Настройки");
    dlg.color(theme.windowBg);
    dlg.set_modal();

    auto* label = new Fl_Box(16, 16, 508, 22, "Путь к редактору кода:");
    label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    label->labelsize(14);
    label->labelcolor(theme.text);
    label->box(FL_NO_BOX);

    state.input = new Fl_Input(16, 44, 400, 30);
    state.input->value(path.c_str());
    state.input->textsize(14);
    state.input->textfont(FL_HELVETICA);
    state.input->color(theme.controlBg);
    state.input->textcolor(theme.text);
    state.input->cursor_color(theme.text);

    auto* browse = new Fl_Button(424, 44, 100, 30, "Обзор...");
    browse->labelsize(14);
    browse->labelcolor(theme.text);
    browse->color(theme.buttonBg);
    browse->clear_visible_focus();
    browse->callback([](Fl_Widget*, void* data) {
        auto* input = static_cast<Fl_Input*>(data);
        Fl_Native_File_Chooser chooser;
        chooser.title("Редактор кода");
        chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
#if defined(_WIN32)
        chooser.filter("Программы\t*.exe\nВсе файлы\t*.*");
#else
        chooser.filter("Все файлы\t*");
#endif
        if (input->value() && input->value()[0] != '\0') {
            chooser.preset_file(input->value());
        }
        if (chooser.show() == 0 && chooser.filename()) {
            input->value(chooser.filename());
        }
    }, state.input);

    auto* hint = new Fl_Box(
        16, 82, 508, 32,
        "Если путь не указан, проект открывается в Visual Studio Code."
    );
    hint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    hint->labelsize(13);
    hint->labelcolor(theme.mutedText);
    hint->box(FL_NO_BOX);

    state.gitStatus = new Fl_Check_Button(16, 122, 508, 26, "Проверять статус Git автоматически");
    state.gitStatus->value(gitStatusEnabled ? 1 : 0);
    state.gitStatus->labelsize(14);
    state.gitStatus->labelcolor(theme.text);
    state.gitStatus->color(theme.controlBg);
    state.gitStatus->selection_color(theme.selectionBg);
    state.gitStatus->clear_visible_focus();

    auto* gitHint = new Fl_Box(
        38, 148, 486, 48,
        "Выключите, если проекты лежат на дисках, которые не должны просыпаться. "
        "Кнопка на панели по-прежнему проверяет статус вручную."
    );
    gitHint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    gitHint->labelsize(13);
    gitHint->labelcolor(theme.mutedText);
    gitHint->box(FL_NO_BOX);

    auto* cancel = new Fl_Button(308, 214, 100, 30, "Отмена");
    cancel->labelsize(14);
    cancel->labelcolor(theme.text);
    cancel->color(theme.buttonBg);
    cancel->clear_visible_focus();
    cancel->callback([](Fl_Widget* w, void*) {
        if (w->window()) w->window()->hide();
    });

    auto* save = new Fl_Button(418, 214, 106, 30, "Сохранить");
    save->labelsize(14);
    save->labelcolor(theme.text);
    save->color(theme.buttonBg);
    save->clear_visible_focus();
    save->callback([](Fl_Widget* w, void* data) {
        auto* st = static_cast<State*>(data);
        std::string value = normalizeEditorPath(st->input->value() ? st->input->value() : "");
        if (!value.empty() && !PlatformService::fileExists(value)) {
            PlatformService::showError("Настройки", "Указанный файл редактора не найден.");
            return;
        }
        st->input->value(value.c_str());
        st->accepted = true;
        if (w->window()) w->window()->hide();
    }, &state);

    dlg.end();
    if (parent) {
        dlg.position(
            parent->x() + (parent->w() - dlg.w()) / 2,
            parent->y() + (parent->h() - dlg.h()) / 2
        );
    }
    dlg.show();
    while (dlg.shown()) {
        Fl::wait();
    }
    if (!state.accepted) return false;
    path = state.input->value() ? state.input->value() : "";
    gitStatusEnabled = state.gitStatus->value() != 0;
    return true;
}

bool confirmDeletion(const Theme& theme, Fl_Window* parent, const std::string& message) {
    struct State {
        bool accepted = false;
    } state;

    class Dialog : public Fl_Window {
    public:
        using Fl_Window::Fl_Window;

        int handle(int event) override {
            if (event == FL_KEYBOARD) {
                const int key = Fl::event_key();
                if (key == FL_Enter || key == FL_KP_Enter) {
                    if (auto* button = dynamic_cast<Fl_Button*>(Fl::focus())) {
                        if (button->window() == this) {
                            button->do_callback();
                            return 1;
                        }
                    }
                }
            }
            return Fl_Window::handle(event);
        }
    };

    Dialog dlg(440, 148, "Удаление проекта");
    dlg.color(theme.windowBg);
    dlg.set_modal();
    dlg.callback([](Fl_Widget* w, void*) { w->hide(); });

    auto* text = new Fl_Box(16, 16, 408, 68);
    text->copy_label(message.c_str());
    text->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    text->labelsize(14);
    text->labelcolor(theme.text);
    text->box(FL_NO_BOX);

    auto* removeBtn = new Fl_Button(212, 100, 100, 32, "Удалить");
    removeBtn->labelsize(14);
    removeBtn->labelcolor(theme.text);
    removeBtn->color(theme.buttonBg);
    removeBtn->callback([](Fl_Widget* w, void* data) {
        static_cast<State*>(data)->accepted = true;
        if (w->window()) w->window()->hide();
    }, &state);

    auto* cancelBtn = new Fl_Button(324, 100, 100, 32, "Отмена");
    cancelBtn->labelsize(14);
    cancelBtn->labelcolor(theme.text);
    cancelBtn->color(theme.buttonBg);
    cancelBtn->callback([](Fl_Widget* w, void*) {
        if (w->window()) w->window()->hide();
    });

    dlg.end();
    if (parent) {
        dlg.position(
            parent->x() + (parent->w() - dlg.w()) / 2,
            parent->y() + (parent->h() - dlg.h()) / 2
        );
    }
    dlg.show();
    removeBtn->take_focus();
    while (dlg.shown()) {
        Fl::wait();
    }
    return state.accepted;
}

} // namespace

MainWindow::~MainWindow() {
    Fl::remove_timeout(cbDeferredSave, this);
    Fl::remove_timeout(cbGitRefresh, this);
    stopGitWorker();
    if (uiReady_) {
        persistSettings();
    }
}

MainWindow::MainWindow(const Theme& theme)
    : Fl_Double_Window(1300, 600, "Project Manager")
    , theme_(theme)
    , icons_(theme.iconHexColor, 16) {
    settings_ = SettingsService::load();
    color(theme_.windowBg);
    size_range(800, 420);

    // Не закрывать приложение по клавише Escape
    callback([](Fl_Widget* w, void*) {
        if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
            return;
        }
        static_cast<MainWindow*>(w)->hide();
    });

    applySavedWindowGeometry();

    buildToolbar();

    constexpr int kMarginX = 20;
    constexpr int kTableTop = 58;
    constexpr int kBottomMargin = 20;

    table_ = new ProjectTable(
        kMarginX,
        kTableTop,
        w() - 2 * kMarginX,
        h() - kTableTop - kBottomMargin,
        theme_,
        icons_
    );

    table_->onAction = [this](int id, ProjectTable::Action action) {
        onTableAction(id, action);
    };
    table_->onCellEdited = [this](int id, ProjectTable::Column col, const std::string& val) {
        onTableCellEdited(id, col, val);
    };
    table_->onDeleteProject = [this](int id) {
        onDeleteProject(id);
    };
    table_->onHeaderClicked = [this](int column) {
        onSortColumn(column);
    };
    table_->onColumnWidthsChanged = [this]() {
        saveUiState();
    };
    if (settings_.hasColumnWidths) {
        table_->setColumnWidths(settings_.columnWidths);
    }
    table_->setSortIndicator(settings_.sortColumn, settings_.sortAscending);

    end();
    resizable(table_);

    // Загрузка проектов из SQLite
    auto loaded = ProjectService::loadProjects();
    allProjects_.reserve(loaded.size());
    for (auto& p : loaded) {
        allProjects_.push_back(std::make_unique<Project>(std::move(p)));
    }

    applyFilter();
    startGitWorker();
    if (settings_.gitStatusEnabled) {
        enqueueGitJobs(true);
    } else {
        for (const auto& project : allProjects_) {
            project->gitTooltip = "Автопроверка выключена";
        }
        if (table_) table_->redraw();
    }
}

void MainWindow::applySavedWindowGeometry() {
    if (!settings_.hasWindowGeometry) return;

    int ww = std::max(800, settings_.windowW);
    int hh = std::max(420, settings_.windowH);
    int wx = settings_.windowX;
    int wy = settings_.windowY;

    if (Fl::screen_count() > 0) {
        int screen = 0;
        for (int i = 0; i < Fl::screen_count(); ++i) {
            int sx = 0, sy = 0, sw = 0, sh = 0;
            Fl::screen_work_area(sx, sy, sw, sh, i);
            if (wx >= sx && wx < sx + sw && wy >= sy && wy < sy + sh) {
                screen = i;
                break;
            }
        }
        int sx = 0, sy = 0, sw = 0, sh = 0;
        Fl::screen_work_area(sx, sy, sw, sh, screen);
        if (sw > 0 && ww > sw) ww = std::max(800, sw);
        if (sh > 0 && hh > sh) hh = std::max(420, sh);
    }

    size(ww, hh);
    if (titleBarOnSomeScreen(wx, wy, ww, hh)) {
        position(wx, wy);
    }
}

void MainWindow::showWindow() {
    uiReady_ = true;
#if defined(_WIN32)
    bool appliedOnCreate = PlatformService::runWithDarkTitleBarOnCreate(theme_.dark, [this]() {
        show();
    });
    if (!appliedOnCreate) {
        PlatformService::applyWindowDarkTitleBar(fl_xid(this), theme_.dark);
    }
#else
    show();
#endif
    if (table_) {
        table_->take_focus();
    }
    Fl::remove_timeout(cbGitRefresh, this);
    Fl::add_timeout(45.0, cbGitRefresh, this);
}

void MainWindow::hide() {
    Fl::remove_timeout(cbDeferredSave, this);
    Fl::remove_timeout(cbGitRefresh, this);
    if (uiReady_) {
        persistSettings();
        uiReady_ = false;
    }
    Fl_Double_Window::hide();
}

void MainWindow::resize(int X, int Y, int W, int H) {
    Fl_Double_Window::resize(X, Y, W, H);
    if (!uiReady_) return;
    Fl::remove_timeout(cbDeferredSave, this);
    Fl::add_timeout(0.4, cbDeferredSave, this);
}

void MainWindow::persistSettings() {
    settings_.windowX = x();
    settings_.windowY = y();
    settings_.windowW = w();
    settings_.windowH = h();
    settings_.hasWindowGeometry = true;
    if (table_) {
        table_->copyColumnWidths(settings_.columnWidths);
        settings_.hasColumnWidths = true;
    }
    SettingsService::save(settings_);
}

void MainWindow::saveUiState() {
    if (!uiReady_) return;
    persistSettings();
}

void MainWindow::buildToolbar() {
    toolbar_ = new Fl_Group(0, 0, w(), 52);
    toolbar_->box(FL_FLAT_BOX);
    toolbar_->color(theme_.toolbarBg);

    int curX = 15;
    constexpr int yBtn = 10;
    constexpr int btnH = 32;

    // Кнопка "Открыть папку с конфигом"
    btnConfigFolder_ = new ToolbarButton(curX, yBtn, 32, btnH, theme_);
    btnConfigFolder_->image(icons_.folder());
    btnConfigFolder_->tooltip("Открыть папку с конфигом");
    btnConfigFolder_->callback(cbOpenConfigFolder, this);
    curX += 32 + 10;

    // Кнопка "Добавить строку"
    btnAddProject_ = new ToolbarButton(curX, yBtn, 32, btnH, theme_);
    btnAddProject_->image(icons_.plus());
    btnAddProject_->tooltip("Добавить строку");
    btnAddProject_->callback(cbAddProject, this);
    curX += 32 + 10;

    btnSettings_ = new ToolbarButton(curX, yBtn, 32, btnH, theme_);
    btnSettings_->image(icons_.settings());
    btnSettings_->tooltip("Настройки");
    btnSettings_->callback(cbOpenSettings, this);
    curX += 32 + 10;

    btnRefreshGit_ = new ToolbarButton(curX, yBtn, 32, btnH, theme_);
    btnRefreshGit_->image(icons_.refresh());
    btnRefreshGit_->tooltip("Проверить статус Git");
    btnRefreshGit_->callback(cbRefreshGit, this);
    curX += 32 + 15;

    // Разделитель 1
    auto* sep1 = new Fl_Box(curX, yBtn + 4, 1, btnH - 8);
    sep1->box(FL_FLAT_BOX);
    sep1->color(theme_.separator);
    curX += 1 + 15;

    // Фильтр по языку
    auto* lblFilter = new Fl_Box(curX, yBtn, 122, btnH, "Фильтр по языку:");
    lblFilter->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    lblFilter->labelfont(FL_HELVETICA);
    lblFilter->labelsize(14);
    lblFilter->labelcolor(theme_.text);
    curX += 122 + 6;

    languageFilterChoice_ = new Fl_Choice(curX, yBtn, 150, btnH);
    languageFilterChoice_->box(FL_BORDER_BOX);
    languageFilterChoice_->color(theme_.controlBg);
    languageFilterChoice_->textcolor(theme_.text);
    languageFilterChoice_->textfont(FL_HELVETICA);
    languageFilterChoice_->textsize(14);
    for (const auto& lang : filterLanguages()) {
        languageFilterChoice_->add(escapeMenuText(lang).c_str());
    }
    languageFilterChoice_->value(0); // "Все"
    languageFilterChoice_->callback(cbLanguageFilter, this);
    curX += 150 + 15;

    // Разделитель 2
    auto* sep2 = new Fl_Box(curX, yBtn + 4, 1, btnH - 8);
    sep2->box(FL_FLAT_BOX);
    sep2->color(theme_.separator);
    curX += 1 + 12;

    // Иконка поиска
    auto* searchIconBox = new Fl_Box(curX, yBtn, 24, btnH);
    searchIconBox->image(icons_.search());
    searchIconBox->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    curX += 24 + 4;

    // Поле поиска
    searchInput_ = new SearchInput(curX, yBtn, 260, btnH);
    searchInput_->box(FL_BORDER_BOX);
    searchInput_->color(theme_.controlBg);
    searchInput_->textcolor(theme_.text);
    searchInput_->cursor_color(theme_.text);
    searchInput_->textfont(FL_HELVETICA);
    searchInput_->textsize(14);
    searchInput_->placeholder("Поиск по названию и описанию...");
    searchInput_->placeholderColor(theme_.mutedText);
    searchInput_->when(FL_WHEN_CHANGED);
    searchInput_->callback(cbSearchInput, this);
    curX += 260;

    // Невидимый растягиваемый спейсер справа, чтобы виджеты тулбара не растягивались при ресайзе окна
    auto* spacer = new Fl_Box(curX, 0, std::max(10, w() - curX), 52);
    spacer->box(FL_NO_BOX);

    toolbar_->end();
    toolbar_->resizable(spacer);
}

void MainWindow::applyFilter() {
    std::vector<Project*> filtered;
    filtered.reserve(allProjects_.size());

    const std::string query = PlatformService::trim(searchQuery_);

    for (const auto& ptr : allProjects_) {
        Project* p = ptr.get();
        if (selectedLanguageFilter_ != "Все" && p->language != selectedLanguageFilter_) {
            continue;
        }
        if (!query.empty()) {
            bool matchName = PlatformService::containsCaseInsensitiveUtf8(p->name, query);
            bool matchDesc = PlatformService::containsCaseInsensitiveUtf8(p->description, query);
            if (!matchName && !matchDesc) {
                continue;
            }
        }
        filtered.push_back(p);
    }

    auto less = makeSortLess();
    if (less) {
        std::stable_sort(filtered.begin(), filtered.end(), less);
    }

    table_->setProjects(std::move(filtered));
}

Project* MainWindow::findProjectById(int id) {
    for (const auto& ptr : allProjects_) {
        if (ptr->id == id) {
            return ptr.get();
        }
    }
    return nullptr;
}

void MainWindow::onOpenConfigFolder() {
    table_->commitEdit();
    PlatformService::openFolder(ProjectService::appDataFolder());
}

void MainWindow::onOpenSettings() {
    table_->commitEdit();
    std::string path = settings_.codeEditorPath;
    bool gitStatusEnabled = settings_.gitStatusEnabled;
    if (!editCodeEditorPath(theme_, this, path, gitStatusEnabled)) return;
    settings_.codeEditorPath = std::move(path);
    const bool gitChanged = settings_.gitStatusEnabled != gitStatusEnabled;
    settings_.gitStatusEnabled = gitStatusEnabled;
    persistSettings();
    if (gitChanged) {
        setGitStatusEnabled(gitStatusEnabled);
    }
}

void MainWindow::onRefreshGit() {
    table_->commitEdit();
    enqueueGitJobs(true);
}

void MainWindow::onAddProject() {
    table_->commitEdit();

    auto newProject = std::make_unique<Project>();
    newProject->name = "Новый проект";
    newProject->description = "Описание";
    newProject->status = "Новый";
    newProject->folderPath.clear();
    newProject->language.clear();
    newProject->lastInteractionIso.clear();

    newProject->id = ProjectService::addProject(*newProject);
    int addedId = newProject->id;
    Project* addedPtr = newProject.get();
    allProjects_.push_back(std::move(newProject));

    applyFilter();
    if (table_->findRowByProjectId(addedId) < 0) {
        table_->appendVisibleProject(addedPtr);
        table_->sortCurrent(makeSortLess());
    }
    int row = table_->findRowByProjectId(addedId);
    if (row >= 0) {
        table_->selectAndScrollToRow(row);
    }
}

void MainWindow::onSortColumn(int column) {
    if (column < 0 || column >= static_cast<int>(ProjectTable::Column::Count)) {
        return;
    }
    if (settings_.sortColumn == column) {
        settings_.sortAscending = !settings_.sortAscending;
    } else {
        settings_.sortColumn = column;
        settings_.sortAscending = true;
    }
    table_->setSortIndicator(settings_.sortColumn, settings_.sortAscending);
    applyFilter();
    saveUiState();
}

std::function<bool(const Project*, const Project*)> MainWindow::makeSortLess() const {
    if (settings_.sortColumn < 0 || settings_.sortColumn >= static_cast<int>(ProjectTable::Column::Count)) {
        return {};
    }
    const int column = settings_.sortColumn;
    const bool ascending = settings_.sortAscending;
    const bool byTime = column == static_cast<int>(ProjectTable::Column::LastInteraction);
    return [column, ascending, byTime](const Project* a, const Project* b) {
        if (byTime) {
            const bool missingA = missingInteraction(a);
            const bool missingB = missingInteraction(b);
            if (missingA || missingB) {
                if (missingA == missingB) return false;
                return !missingA;
            }
        }
        int cmp = compareProjects(a, b, column);
        if (!ascending) cmp = -cmp;
        return cmp < 0;
    };
}

void MainWindow::onLanguageFilterChanged() {
    table_->commitEdit();
    int idx = languageFilterChoice_->value();
    const auto& langs = filterLanguages();
    if (idx >= 0 && static_cast<std::size_t>(idx) < langs.size()) {
        if (selectedLanguageFilter_ != langs[static_cast<std::size_t>(idx)]) {
            selectedLanguageFilter_ = langs[static_cast<std::size_t>(idx)];
            applyFilter();
        }
    }
}

void MainWindow::onSearchQueryChanged() {
    std::string val = searchInput_->value() ? searchInput_->value() : "";
    if (searchQuery_ != val) {
        searchQuery_ = std::move(val);
        applyFilter();
    }
}

void MainWindow::onTableAction(int projectId, ProjectTable::Action action) {
    Project* p = findProjectById(projectId);
    if (!p || p->folderPath.empty()) return;

    p->lastInteractionIso = TimeUtil::currentTimestampIso8601();
    ProjectService::updateProject(*p);
    table_->redraw();

    switch (action) {
        case ProjectTable::Action::OpenFolder:
            PlatformService::openFolder(p->folderPath);
            break;
        case ProjectTable::Action::OpenEditor:
            CodeEditorService::openFolderInCodeEditor(p->folderPath);
            break;
        case ProjectTable::Action::RunScript:
            PlatformService::runScript(p->folderPath);
            break;
    }
}

void MainWindow::onTableCellEdited(int projectId, ProjectTable::Column col, const std::string& /*newValue*/) {
    Project* p = findProjectById(projectId);
    if (!p) return;

    ProjectService::updateProject(*p);

    if (col == ProjectTable::Column::FolderPath) {
        if (!settings_.gitStatusEnabled) {
            p->gitSync = GitSync::Unknown;
            p->gitDirty = false;
            p->gitTooltip = "Автопроверка выключена";
        } else if (p->folderPath.empty() || !PlatformService::directoryExists(p->folderPath)) {
            p->gitSync = GitSync::NotARepo;
            p->gitDirty = false;
            p->gitTooltip = p->folderPath.empty() ? "Папка проекта не указана" : "Папка не найдена";
        } else {
            p->gitSync = GitSync::Unknown;
            p->gitDirty = false;
            p->gitTooltip = "Проверка Git…";
            enqueueGitJob(p->id, p->folderPath);
        }
        if (table_) {
            table_->redraw();
            table_->refreshHoverTooltip();
        }
    }

    if (col == ProjectTable::Column::Language) {
        applyFilter();
    }
}

void MainWindow::onDeleteProject(int projectId) {
    Project* project = findProjectById(projectId);
    if (!project) return;

    const std::string name = project->name.empty() ? "этот проект" : project->name;
    const std::string message = "Удалить проект \"" + name + "\"?";
    if (!confirmDeletion(theme_, this, message)) {
        if (table_) table_->take_focus();
        return;
    }

    ProjectService::deleteProject(projectId);

    allProjects_.erase(
        std::remove_if(allProjects_.begin(), allProjects_.end(),
                       [projectId](const std::unique_ptr<Project>& p) { return p->id == projectId; }),
        allProjects_.end()
    );

    applyFilter();
    if (table_) table_->take_focus();
}

void MainWindow::cbOpenConfigFolder(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onOpenConfigFolder();
}

void MainWindow::cbOpenSettings(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onOpenSettings();
}

void MainWindow::cbRefreshGit(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onRefreshGit();
}

void MainWindow::cbDeferredSave(void* userData) {
    static_cast<MainWindow*>(userData)->saveUiState();
}

void MainWindow::cbAddProject(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onAddProject();
}

void MainWindow::cbLanguageFilter(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onLanguageFilterChanged();
}

void MainWindow::cbSearchInput(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onSearchQueryChanged();
}

struct GitUiResult {
    int id = 0;
    std::string folder;
    GitProbe probe;
};

void MainWindow::gitAwakeCallback(void* data) {
    std::unique_ptr<GitUiResult> result(static_cast<GitUiResult*>(data));
    MainWindow* self = gitUiOwner_.load();
    if (!self || !result) return;
    self->onGitResult(result->id, result->folder, result->probe);
}

void MainWindow::cbGitRefresh(void* userData) {
    auto* self = static_cast<MainWindow*>(userData);
    if (!self->uiReady_ || self->gitStop_.load()) return;
    if (self->settings_.gitStatusEnabled) {
        self->enqueueGitJobs(false);
    }
    Fl::add_timeout(45.0, cbGitRefresh, userData);
}

int MainWindow::handle(int event) {
    const int result = Fl_Double_Window::handle(event);
    if (event == FL_ACTIVATE && uiReady_ && !gitStop_.load() && settings_.gitStatusEnabled) {
        enqueueGitJobs(false);
    }
    return result;
}

void MainWindow::startGitWorker() {
    gitUiOwner_.store(this);
    gitThread_ = std::thread([this] { gitWorkerLoop(); });
}

void MainWindow::setGitStatusEnabled(bool enabled) {
    settings_.gitStatusEnabled = enabled;
    if (!enabled) {
        std::lock_guard<std::mutex> lock(gitMutex_);
        gitQueue_.clear();
        return;
    }
    enqueueGitJobs(true);
}

void MainWindow::stopGitWorker() {
    {
        std::lock_guard<std::mutex> lock(gitMutex_);
        gitStop_.store(true);
        gitQueue_.clear();
    }
    gitUiOwner_.store(nullptr);
    gitCv_.notify_all();
    if (gitThread_.joinable()) {
        gitThread_.join();
    }
}

void MainWindow::enqueueGitJobs(bool force) {
    std::vector<GitJob> jobs;
    jobs.reserve(allProjects_.size());
    bool visual = false;
    for (const auto& ptr : allProjects_) {
        Project* project = ptr.get();
        if (project->folderPath.empty() || !PlatformService::directoryExists(project->folderPath)) {
            const std::string tip = project->folderPath.empty() ? "Папка проекта не указана" : "Папка не найдена";
            if (project->gitSync != GitSync::NotARepo || project->gitDirty || project->gitTooltip != tip) {
                project->gitSync = GitSync::NotARepo;
                project->gitDirty = false;
                project->gitTooltip = tip;
                visual = true;
            }
            continue;
        }
        if (project->gitSync == GitSync::Unknown || project->gitSync == GitSync::Disabled) {
            project->gitSync = GitSync::Unknown;
            if (project->gitTooltip != "Проверка Git…") {
                project->gitTooltip = "Проверка Git…";
                visual = true;
            }
        }
        jobs.push_back(GitJob{project->id, project->folderPath});
    }

    bool queued = false;
    {
        std::lock_guard<std::mutex> lock(gitMutex_);
        if (!gitStop_.load()) {
            const auto now = std::chrono::steady_clock::now();
            const bool tooSoon = lastGitEnqueue_.time_since_epoch().count() != 0 &&
                                 now - lastGitEnqueue_ < std::chrono::seconds(15);
            if (force || (!tooSoon && gitQueue_.empty())) {
                gitQueue_ = std::move(jobs);
                lastGitEnqueue_ = now;
                queued = true;
            }
        }
    }
    if (queued) gitCv_.notify_one();
    if (visual && table_) {
        table_->redraw();
        table_->refreshHoverTooltip();
    }
}

void MainWindow::enqueueGitJob(int id, const std::string& folder) {
    {
        std::lock_guard<std::mutex> lock(gitMutex_);
        if (gitStop_.load()) return;
        gitQueue_.erase(std::remove_if(gitQueue_.begin(), gitQueue_.end(),
                                       [id](const GitJob& job) { return job.id == id; }),
                        gitQueue_.end());
        gitQueue_.push_back(GitJob{id, folder});
    }
    gitCv_.notify_one();
}

void MainWindow::gitWorkerLoop() {
    while (!gitStop_.load()) {
        GitJob job;
        {
            std::unique_lock<std::mutex> lock(gitMutex_);
            gitCv_.wait(lock, [this] { return gitStop_.load() || !gitQueue_.empty(); });
            if (gitStop_.load()) return;
            job = std::move(gitQueue_.front());
            gitQueue_.erase(gitQueue_.begin());
        }

        if (gitStop_.load()) return;

        GitProbe probe = GitStatusService::probe(job.folder);
        if (gitStop_.load()) return;

        auto* result = new GitUiResult();
        result->id = job.id;
        result->folder = std::move(job.folder);
        result->probe = std::move(probe);
        Fl::awake(gitAwakeCallback, result);
    }
}

void MainWindow::onGitResult(int projectId, const std::string& folder, const GitProbe& probe) {
    Project* project = findProjectById(projectId);
    if (!project || project->folderPath != folder) return;

    project->gitSync = probe.sync;
    project->gitDirty = probe.dirty;
    project->gitTooltip = probe.tooltip;

    const bool dateChanged = isNewerIso(probe.activityIso, project->lastInteractionIso);
    if (dateChanged) {
        project->lastInteractionIso = probe.activityIso;
        ProjectService::updateLastInteraction(project->id, project->lastInteractionIso);
    }

    const bool sortByDate = settings_.sortColumn == static_cast<int>(ProjectTable::Column::LastInteraction);
    const bool sortByGit = settings_.sortColumn == static_cast<int>(ProjectTable::Column::Git);
    if (table_ && ((dateChanged && sortByDate) || sortByGit) && !table_->isEditing()) {
        table_->sortCurrent(makeSortLess());
    }
    if (table_) {
        table_->redraw();
        table_->refreshHoverTooltip();
    }
}

} // namespace ProjectManager
