#include "vkutils.hpp"
#include "vkcontext.hpp"

#include <stdexcept>
#include <cstring>
#include <iostream>

std::vector<const char*> VkUtils::GetExtensions() {
    uint32_t extensionCount = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    std::vector<const char*> exts(extensions, extensions + extensionCount);
    return exts;
}

void ListLayers() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto availableLayer : layerProperties) {
        std::cout << "\t- Available Layer (" << availableLayer.layerName << ")\n";
    }
}

void VkUtils::CheckLayers() {
    ListLayers();
    uint32_t layerCount = 0;

    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    for(auto requestedLayer : VALIDATION_LAYERS) {
        bool found = false;

        for(auto availableLayer : layerProperties) {
            if(strcmp(requestedLayer, availableLayer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if(!found) { throw std::runtime_error("Requested Validation Layers could not be found"); }
    }
}