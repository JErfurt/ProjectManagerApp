#pragma once

#include <FL/Fl_SVG_Image.H>

#include <memory>
#include <string>

namespace ProjectManager {

class IconSet {
public:
    explicit IconSet(const std::string& hexColor, int size = 16);

    Fl_SVG_Image* folder() const { return folder_.get(); }
    Fl_SVG_Image* plus() const { return plus_.get(); }
    Fl_SVG_Image* search() const { return search_.get(); }
    Fl_SVG_Image* code() const { return code_.get(); }
    Fl_SVG_Image* play() const { return play_.get(); }
    Fl_SVG_Image* settings() const { return settings_.get(); }
    Fl_SVG_Image* refresh() const { return refresh_.get(); }
    Fl_SVG_Image* trash() const { return trash_.get(); }
    Fl_SVG_Image* gitSynced() const { return gitSynced_.get(); }
    Fl_SVG_Image* gitNotSynced() const { return gitNotSynced_.get(); }
    Fl_SVG_Image* gitDirty() const { return gitDirty_.get(); }
    Fl_SVG_Image* gitUnavailable() const { return gitUnavailable_.get(); }

private:
    static std::unique_ptr<Fl_SVG_Image> buildSvg(const char* pathData, const std::string& hexColor, int size);

    std::unique_ptr<Fl_SVG_Image> folder_;
    std::unique_ptr<Fl_SVG_Image> plus_;
    std::unique_ptr<Fl_SVG_Image> search_;
    std::unique_ptr<Fl_SVG_Image> code_;
    std::unique_ptr<Fl_SVG_Image> play_;
    std::unique_ptr<Fl_SVG_Image> settings_;
    std::unique_ptr<Fl_SVG_Image> refresh_;
    std::unique_ptr<Fl_SVG_Image> trash_;
    std::unique_ptr<Fl_SVG_Image> gitSynced_;
    std::unique_ptr<Fl_SVG_Image> gitNotSynced_;
    std::unique_ptr<Fl_SVG_Image> gitDirty_;
    std::unique_ptr<Fl_SVG_Image> gitUnavailable_;
};

} // namespace ProjectManager
