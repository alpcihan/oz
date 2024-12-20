#pragma once

#include "oz/base.h"

namespace oz::file {
    
#if defined(_WIN32)
#include <windows.h>
std::string getExecutablePath() {
    char result[MAX_PATH];
    GetModuleFileName(NULL, result, MAX_PATH);
    return std::string(result);
}
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
std::string getExecutablePath() {
    char result[PATH_MAX];
    uint32_t size = sizeof(result);
    if (_NSGetExecutablePath(result, &size) == 0)
        return std::string(result);
    else
        return std::string();
}
#endif

std::string getBuildPath() { return std::string(BUILD_DIR); }

std::string getSourcePath() { return std::string(SOURCE_DIR); }

} // namespace oz