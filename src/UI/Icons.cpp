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

constexpr const char* kRefreshPath =
    "M17.65,6.35C16.2,4.9 14.21,4 12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12H18A6,6 0 0,1 12,18A6,6 0 0,1 6,12A6,6 0 0,1 12,6C13.66,6 15.14,6.69 16.22,7.78L13,11H20V4L17.65,6.35Z";

constexpr const char* kTrashPath =
    "M9,3V4H4V6H5V19A2,2 0 0,0 7,21H17A2,2 0 0,0 19,19V6H20V4H15V3H9M7,6H17V19H7V6M9,8V17H11V8H9M13,8V17H15V8H13Z";

constexpr const char* kCloudPath =
    "M19.35,10.04C18.67,6.59 15.64,4 12,4C9.11,4 6.6,5.64 5.35,8.04C2.34,8.36 0,10.91 0,14A6,6 0 0,0 6,20H19A5,5 0 0,0 24,15C24,12.36 21.95,10.22 19.35,10.04Z";

constexpr const char* kCloudCheck =
    "M8.5,13.6 L11.2,16.2 L16.1,11.1";

constexpr const char* kCloudAlert =
    "M12,11.1 V14.2";

std::unique_ptr<Fl_SVG_Image> makeCloudIcon(const std::string& color, const char* strokePath, bool alertDot) {
    std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"16\" height=\"16\">"
        "<path fill=\"" + color + "\" d=\"" + std::string(kCloudPath) + "\"/>";
    if (strokePath && strokePath[0] != '\0') {
        svg += "<path fill=\"none\" stroke=\"#FFFFFF\" stroke-width=\"1.8\" "
               "stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"";
        svg += strokePath;
        svg += "\"/>";
    }
    if (alertDot) {
        svg += "<circle cx=\"12\" cy=\"16.6\" r=\"1.05\" fill=\"#FFFFFF\"/>";
    }
    svg += "</svg>";

    auto img = std::make_unique<Fl_SVG_Image>(nullptr, svg.c_str());
    img->scale(16, 16, 1, 1);
    return img;
}

std::unique_ptr<Fl_SVG_Image> makeDirtyCloud() {
    std::string svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" width=\"16\" height=\"16\">"
        "<path fill=\"#E24B3B\" d=\"" + std::string(kCloudPath) + "\"/>"
        "<circle cx=\"12\" cy=\"14.2\" r=\"1.8\" fill=\"#FFFFFF\"/>"
        "</svg>";
    auto img = std::make_unique<Fl_SVG_Image>(nullptr, svg.c_str());
    img->scale(16, 16, 1, 1);
    return img;
}

constexpr const char* kSettingsPath =
    "M12,15.5A3.5,3.5 0 0,1 8.5,12A3.5,3.5 0 0,1 12,8.5A3.5,3.5 0 0,1 15.5,12A3.5,3.5 0 0,1 12,15.5"
    "M19.43,12.97C19.47,12.65 19.5,12.33 19.5,12C19.5,11.67 19.47,11.34 19.43,11L21.54,9.37"
    "C21.73,9.22 21.78,8.95 21.66,8.73L19.66,5.27C19.54,5.05 19.27,4.96 19.05,5.05L16.56,6.05"
    "C16.04,5.66 15.5,5.32 14.87,5.07L14.5,2.42C14.46,2.18 14.25,2 14,2H10C9.75,2 9.54,2.18 9.5,2.42"
    "L9.13,5.07C8.5,5.32 7.96,5.66 7.44,6.05L4.95,5.05C4.73,4.96 4.46,5.05 4.34,5.27L2.34,8.73"
    "C2.21,8.95 2.27,9.22 2.46,9.37L4.57,11C4.53,11.34 4.5,11.67 4.5,12C4.5,12.33 4.53,12.65 4.57,12.97"
    "L2.46,14.63C2.27,14.78 2.21,15.05 2.34,15.27L4.34,18.73C4.46,18.95 4.73,19.03 4.95,18.95L7.44,17.94"
    "C7.96,18.34 8.5,18.68 9.13,18.93L9.5,21.58C9.54,21.82 9.75,22 10,22H14C14.25,22 14.46,21.82 14.5,21.58"
    "L14.87,18.93C15.5,18.67 16.04,18.34 16.56,17.94L19.05,18.95C19.27,19.03 19.54,18.95 19.66,18.73"
    "L21.66,15.27C21.78,15.05 21.73,14.78 21.54,14.63L19.43,12.97Z";

} // namespace

IconSet::IconSet(const std::string& hexColor, int size)
    : folder_(buildSvg(kFolderPath, hexColor, size))
    , plus_(buildSvg(kPlusPath, hexColor, size))
    , search_(buildSvg(kSearchPath, hexColor, size))
    , code_(buildSvg(kCodePath, hexColor, size))
    , play_(buildSvg(kPlayPath, hexColor, size))
    , settings_(buildSvg(kSettingsPath, hexColor, size))
    , refresh_(buildSvg(kRefreshPath, hexColor, size))
    , trash_(buildSvg(kTrashPath, hexColor, size))
    , gitSynced_(makeCloudIcon("#2E9B57", kCloudCheck, false))
    , gitNotSynced_(makeCloudIcon("#D4880A", kCloudAlert, true))
    , gitDirty_(makeDirtyCloud())
    , gitUnavailable_(makeCloudIcon("#8E8E93", nullptr, false)) {}

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
