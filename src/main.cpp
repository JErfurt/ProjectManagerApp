#include "Services/PlatformService.hpp"
#include "UI/MainWindow.hpp"
#include "UI/Theme.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Tooltip.H>

#include <memory>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Fl::scheme("gtk+");
    Fl::visual(FL_RGB);

#if defined(_WIN32)
    Fl::set_font(FL_HELVETICA, " Segoe UI");
    Fl::set_font(FL_HELVETICA_BOLD, "BSegoe UI");
#endif

    Fl_Tooltip::size(13);
    Fl_Tooltip::delay(0.4f);

    bool darkMode = ProjectManager::PlatformService::isSystemDarkMode();
    auto theme = ProjectManager::Theme::create(darkMode);
    theme.applyGlobal();

    Fl_Tooltip::color(theme.controlBg);
    Fl_Tooltip::textcolor(theme.text);

    auto window = std::make_unique<ProjectManager::MainWindow>(theme);
    window->showWindow();

    return Fl::run();
}
