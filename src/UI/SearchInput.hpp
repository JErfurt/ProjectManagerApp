#pragma once

#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>

#include <cstring>
#include <string>

namespace ProjectManager {

class SearchInput : public Fl_Input {
public:
    SearchInput(int X, int Y, int W, int H, const char* L = nullptr)
        : Fl_Input(X, Y, W, H, L) {}

    void placeholder(const std::string& text) {
        placeholder_ = text;
        redraw();
    }

    void placeholderColor(Fl_Color c) {
        placeholderColor_ = c;
        redraw();
    }

protected:
    void draw() override {
        Fl_Input::draw();
        if (!placeholder_.empty() && (!value() || value()[0] == '\0')) {
            int dx = Fl::box_dx(box());
            int dy = Fl::box_dy(box());
            int dw = Fl::box_dw(box());
            int dh = Fl::box_dh(box());

            fl_push_clip(x() + dx, y() + dy, w() - dw, h() - dh);
            fl_font(textfont(), textsize());
            fl_color(placeholderColor_);
            fl_draw(placeholder_.c_str(),
                    x() + dx + 4, y() + dy,
                    w() - dw - 8, h() - dh,
                    FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
            fl_pop_clip();
        }
    }

private:
    std::string placeholder_;
    Fl_Color placeholderColor_ = FL_GRAY;
};

} // namespace ProjectManager
