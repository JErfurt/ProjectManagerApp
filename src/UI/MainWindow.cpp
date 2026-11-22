#include "UI/MainWindow.hpp"

#include "Services/CodeEditorService.hpp"
#include "Services/PlatformService.hpp"
#include "Services/ProjectService.hpp"

#include <FL/Fl.H>
#include <FL/x.H>

#include <algorithm>
#include <string>

namespace ProjectManager {

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

} // namespace

MainWindow::MainWindow(const Theme& theme)
    : Fl_Double_Window(1300, 600, "Project Manager")
    , theme_(theme)
    , icons_(theme.iconHexColor, 16) {
    color(theme_.windowBg);
    size_range(800, 420);

    // Не закрывать приложение по клавише Escape
    callback([](Fl_Widget* w, void*) {
        if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
            return;
        }
        static_cast<Fl_Double_Window*>(w)->hide();
    });

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

    end();
    resizable(table_);

    // Загрузка проектов из SQLite
    auto loaded = ProjectService::loadProjects();
    allProjects_.reserve(loaded.size());
    for (auto& p : loaded) {
        allProjects_.push_back(std::make_unique<Project>(std::move(p)));
    }

    applyFilter();
}

void MainWindow::showWindow() {
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
    Project* addedPtr = newProject.get();
    allProjects_.push_back(std::move(newProject));

    // Показываем добавленный проект (как в оригинальной версии)
    table_->appendVisibleProject(addedPtr);
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

    if (col == ProjectTable::Column::Language) {
        applyFilter();
    }
}

void MainWindow::onDeleteProject(int projectId) {
    ProjectService::deleteProject(projectId);

    allProjects_.erase(
        std::remove_if(allProjects_.begin(), allProjects_.end(),
                       [projectId](const std::unique_ptr<Project>& p) { return p->id == projectId; }),
        allProjects_.end()
    );

    applyFilter();
}

void MainWindow::cbOpenConfigFolder(Fl_Widget*, void* userData) {
    static_cast<MainWindow*>(userData)->onOpenConfigFolder();
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

} // namespace ProjectManager
