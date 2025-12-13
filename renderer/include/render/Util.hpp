/// \brief Common utility functions needed by the renderer
#pragma once
#include <string>
#include <vector>

namespace MVRender {
    // Reads an entire file into a string and returns it
    std::string read_file(const char *filename);

    // Reads an entire file as an uint8_t buffer
    std::vector<uint8_t> read_buffer(const char *filename);
}
