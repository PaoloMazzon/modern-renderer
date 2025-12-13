#include <fstream>
#include <filesystem>
#include "render/Util.hpp"

std::string MVRender::read_file(const char *filename) {
    auto size = std::filesystem::file_size(filename);
    std::string content(size, '\0');
    std::ifstream in(filename);
    in.read(&content[0], size);
    return content;
}

std::vector<uint8_t> MVRender::read_buffer(const char *filename) {
    auto size = std::filesystem::file_size(filename);
    std::vector<uint8_t> content(size);
    std::ifstream in(filename, std::ios::binary);
    in.read(reinterpret_cast<char*>(content.data()), size);
    return content;
}
