#pragma once

#include "Models/Project.hpp"

#include <string>

namespace ProjectManager {

struct GitProbe {
    GitSync sync = GitSync::NotARepo;
    bool dirty = false;
    std::string tooltip;
    std::string activityIso;
};

class GitStatusService {
public:
    static GitProbe probe(const std::string& folderPath);
};

} // namespace ProjectManager
