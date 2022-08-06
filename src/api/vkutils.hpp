#pragma once

#include <vector>

namespace VkUtils {
    std::vector<const char*> GetExtensions();
    void CheckLayers();
}