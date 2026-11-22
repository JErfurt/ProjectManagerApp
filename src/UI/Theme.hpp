#pragma once

#include <FL/Enumerations.H>
#include <FL/Fl.H>

#include <string>

namespace ProjectManager {

struct Theme {
    bool dark = false;

    Fl_Color windowBg = FL_BACKGROUND_COLOR;
    Fl_Color toolbarBg = FL_BACKGROUND_COLOR;
    Fl_Color controlBg = FL_BACKGROUND2_COLOR;
    Fl_Color cellBg = FL_BACKGROUND2_COLOR;
    Fl_Color cellAltBg = FL_BACKGROUND2_COLOR;
    Fl_Color headerBg = FL_BACKGROUND_COLOR;
    Fl_Color gridLine = FL_GRAY;
    Fl_Color separator = FL_GRAY;
    Fl_Color text = FL_FOREGROUND_COLOR;
    Fl_Color mutedText = FL_GRAY;
    Fl_Color selectionBg = FL_SELECTION_COLOR;
    Fl_Color selectionText = FL_FOREGROUND_COLOR;
    Fl_Color buttonBg = FL_BACKGROUND_COLOR;
    Fl_Color buttonHoverBg = FL_LIGHT1;
    Fl_Color buttonActiveBg = FL_DARK1;
    Fl_Color buttonBorder = FL_GRAY;

    std::string iconHexColor = "#2B2B2B";

    static Theme create(bool darkMode) {
        Theme t;
        t.dark = darkMode;
        if (darkMode) {
            t.windowBg       = fl_rgb_color(0x20, 0x20, 0x20);
            t.toolbarBg      = fl_rgb_color(0x25, 0x25, 0x26);
            t.controlBg      = fl_rgb_color(0x2D, 0x2D, 0x30);
            t.cellBg         = fl_rgb_color(0x25, 0x25, 0x26);
            t.cellAltBg      = fl_rgb_color(0x2A, 0x2A, 0x2C);
            t.headerBg       = fl_rgb_color(0x1E, 0x1E, 0x1E);
            t.gridLine       = fl_rgb_color(0x42, 0x42, 0x45);
            t.separator      = fl_rgb_color(0x55, 0x55, 0x58);
            t.text           = fl_rgb_color(0xF0, 0xF0, 0xF0);
            t.mutedText      = fl_rgb_color(0x9A, 0x9A, 0x9E);
            t.selectionBg    = fl_rgb_color(0x26, 0x4F, 0x78);
            t.selectionText  = fl_rgb_color(0xFF, 0xFF, 0xFF);
            t.buttonBg       = fl_rgb_color(0x32, 0x32, 0x35);
            t.buttonHoverBg  = fl_rgb_color(0x42, 0x42, 0x47);
            t.buttonActiveBg = fl_rgb_color(0x23, 0x23, 0x26);
            t.buttonBorder   = fl_rgb_color(0x50, 0x50, 0x54);
            t.iconHexColor   = "#E6E6E6";
        } else {
            t.windowBg       = fl_rgb_color(0xF3, 0xF3, 0xF3);
            t.toolbarBg      = fl_rgb_color(0xF3, 0xF3, 0xF3);
            t.controlBg      = fl_rgb_color(0xFF, 0xFF, 0xFF);
            t.cellBg         = fl_rgb_color(0xFF, 0xFF, 0xFF);
            t.cellAltBg      = fl_rgb_color(0xF9, 0xF9, 0xFA);
            t.headerBg       = fl_rgb_color(0xEB, 0xEB, 0xED);
            t.gridLine       = fl_rgb_color(0xD2, 0xD2, 0xD6);
            t.separator      = fl_rgb_color(0xB8, 0xB8, 0xBC);
            t.text           = fl_rgb_color(0x1A, 0x1A, 0x1A);
            t.mutedText      = fl_rgb_color(0x78, 0x78, 0x7E);
            t.selectionBg    = fl_rgb_color(0xD2, 0xE6, 0xF8);
            t.selectionText  = fl_rgb_color(0x11, 0x11, 0x11);
            t.buttonBg       = fl_rgb_color(0xF7, 0xF7, 0xF8);
            t.buttonHoverBg  = fl_rgb_color(0xE4, 0xE6, 0xEA);
            t.buttonActiveBg = fl_rgb_color(0xD5, 0xD8, 0xDD);
            t.buttonBorder   = fl_rgb_color(0xCC, 0xCC, 0xD0);
            t.iconHexColor   = "#2B2B2B";
        }
        return t;
    }

    void applyGlobal() const {
        uchar r = 0, g = 0, b = 0;

        Fl::get_color(windowBg, r, g, b);
        Fl::background(r, g, b);

        Fl::get_color(controlBg, r, g, b);
        Fl::background2(r, g, b);

        Fl::get_color(text, r, g, b);
        Fl::foreground(r, g, b);

        Fl::set_color(FL_SELECTION_COLOR, selectionBg);
    }
};

} // namespace ProjectManager
