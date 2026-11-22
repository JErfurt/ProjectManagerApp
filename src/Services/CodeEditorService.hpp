#pragma once

#include <string>

namespace ProjectManager {

class CodeEditorService {
public:
    static void openFolderInCodeEditor(const std::string& folderPath);
};

} // namespace ProjectManager
