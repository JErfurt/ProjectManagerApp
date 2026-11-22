#pragma once

#include "Models/Project.hpp"

#include <string>
#include <vector>

namespace ProjectManager {

class ProjectService {
public:
    static std::string appDataFolder();
    static std::string dbPath();

    static std::vector<Project> loadProjects();
    static int addProject(const Project& project);
    static void updateProject(const Project& project);
    static void deleteProject(int id);

private:
    static void ensureInitialized();
};

} // namespace ProjectManager
