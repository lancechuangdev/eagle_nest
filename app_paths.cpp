#include "app_paths.h"

const std::filesystem::path AppPaths::Detection_Projects_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_projects";
const std::filesystem::path AppPaths::Training_WIP_Path = AppPaths::getHomePath() / "eagle_nest" / "wip";
const std::filesystem::path AppPaths::Dataset_Images_Path = AppPaths::getHomePath() / "eagle_nest" / "dataset";