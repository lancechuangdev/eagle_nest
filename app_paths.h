#ifndef EAGLE_NEST_APP_PATHS_H
#define EAGLE_NEST_APP_PATHS_H

#include <filesystem>
#include <stdexcept>

class AppPaths
{
public:
    static const std::filesystem::path Training_WIP_Path;
    static const std::filesystem::path Detection_Projects_Path;
    static const std::filesystem::path Dataset_Images_Path;
private:
    static std::filesystem::path getHomePath() {
        const char* home_env = std::getenv("HOME");
        if (!home_env) {
            throw std::runtime_error("HOME environment variable is not set");
        }
        return std::filesystem::path(home_env);
    }
};

#endif // APP_PATHS_H