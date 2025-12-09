# Modern Renderer
This project aims to be an attempt at a modern renderer written in C++ with a C interface
utilizing Vulkan for the bulk of the rendering work.

[![CMake Build & Test](https://github.com/PaoloMazzon/modern-renderer/actions/workflows/build-and-test.yaml/badge.svg?branch=main)](https://github.com/PaoloMazzon/modern-renderer/actions/workflows/build-and-test.yaml)
![coverage](https://gist.githubusercontent.com/PaoloMazzon/227fdd0e548061bf552b6fe1e51b14d4/raw/test.svg)

## Project Overview
As it stands, all tests are in `tests/`, the main application used for testing the renderer
is `test-app/`, and the renderer itself is in `renderer/`. To use the renderer in your own
project, add this repository as a Git submodule and add the following to your `CMakeLists.txt`
file.
```cmake
add_subdirectory(modern-renderer)
# add your executable
target_link_libraries(my-game PRIVATE modern_renderer)
```
Don't forget to recursively clone your submodules, as this project depends on several submodules
in `3rd/`. To build tests, use `-DBUILD_TESTS=ON`, to build the sample app use `-DBUILD_SAMPLE_APP=ON`.

## 3rd Party
See `LICENSE-3RD-PARTY` for information on 3rd-party tools and their associated licenses.
