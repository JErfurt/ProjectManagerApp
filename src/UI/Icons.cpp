#include "UI/Icons.hpp"

namespace ProjectManager {

namespace {

constexpr const char* kFolderPath =
    "M19,20H4C2.89,20 2,19.1 2,18V6C2,4.89 2.89,4 4,4H10L12,6H19A2,2 0 0,1 21,8H21L4,8V18L6.14,10H23.21L20.93,18.5C20.7,19.37 19.92,20 19,20Z";

constexpr const char* kPlusPath =
    "M19,13H13V19H11V13H5V11H11V5H13V11H19V13Z";

constexpr const char* kSearchPath =
    "M9.5,3A6.5,6.5 0 0,1 16,9.5C16,11.11 15.41,12.59 14.44,13.73L14.71,14H15.5L20.5,19L19,20.5L14,15.5V14.71L13.73,14.44C12.59,15.41 11.11,16 9.5,16A6.5,6.5 0 0,1 3,9.5A6.5,6.5 0 0,1 9.5,3M9.5,5C7,5 5,7 5,9.5C5,12 7,14 9.5,14C12,14 14,12 14,9.5C14,7 12,5 9.5,5Z";

constexpr const char* kCodePath =
    "M14.6,16.6L19.2,12L14.6,7.4L16,6L22,12L16,18L14.6,16.6M9.4,16.6L4.8,12L9.4,7.4L8,6L2,12L8,18L9.4,16.6Z";

constexpr const char* kPlayPath =
    "M8,5.14V19.14L19,12.14L8,5.14Z";

} // namespace

IconSet::IconSet(const std::string& hexColor, int size)
    : folder_(buildSvg(kFolderPath, hexColor, size))
    , plus_(buildSvg(kPlusPath, hexColor, size))
    , search_(buildSvg(kSearchPath, hexColor, size))
    , code_(buildSvg(kCodePath, hexColor, size))
    , play_(buildSvg(kPlayPath, hexColor, size)) {}

std::unique_ptr<Fl_SVG_Image> IconSet::buildSvg(const char* pathData, const std::string& hexColor, int size) {
    std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"" +
        std::to_string(size) + "\" height=\"" + std::to_string(size) + "\">"
        "<path fill=\"" + hexColor + "\" d=\"" + pathData + "\"/>"
        "</svg>";

    auto img = std::make_unique<Fl_SVG_Image>(nullptr, svg.c_str());
    img->scale(size, size, 1, 1);
    return img;
}

} // namespace ProjectManager
