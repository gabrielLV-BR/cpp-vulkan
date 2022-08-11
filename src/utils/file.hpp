#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

namespace FileUtils {
  std::vector<char> ReadBinary(const char* path) {
    using namespace std;
    // ios::ate -> start At The End (so we can get the size later)
    ifstream file(path, ios::ate | ios::binary);

    if(!file.is_open()) return {};

    // tellg() will tell the current position, 
    // which will be the end, since we used `std::ios::ate`
    size_t fileSize = static_cast<size_t>(file.tellg());
    vector<char> buffer(fileSize);

    // go to start of file
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }
}