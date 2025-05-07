#include "app_paths.h"

const std::filesystem::path AppPaths::Detection_Projects_Path = AppPaths::getHomePath() / "eagle_eye" / "detection_projects";
const std::filesystem::path AppPaths::Dataset_Path = AppPaths::getHomePath() / "eagle_nest" / "dataset";
const std::filesystem::path AppPaths::Models_Path = AppPaths::getHomePath() / "eagle_nest" / "models";
const std::filesystem::path AppPaths::WIP_Path = AppPaths::getHomePath() / "eagle_nest" / "wip";
const std::filesystem::path AppPaths::WIP_Dataset_Path = AppPaths::WIP_Path / "dataset";
const std::filesystem::path AppPaths::WIP_Model_Path = AppPaths::WIP_Path / "model";