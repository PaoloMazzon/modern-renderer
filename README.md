# Modern Renderer
This project aims to be an attempt at a modern renderer written in C++ with a C interface
utilizing Vulkan for the bulk of the rendering work.

[![CMake Build & Test](https://github.com/PaoloMazzon/modern-renderer/actions/workflows/build-and-test.yaml/badge.svg?branch=main)](https://github.com/PaoloMazzon/modern-renderer/actions/workflows/build-and-test.yaml)

## Project Overview
As it stands, all tests are in `tests/`, the main application used for testing the renderer
is `test-app/`, and the renderer itself is in `renderer/`. These are all built (in a somewhat
goofy way) by the root-level `CMakeLists.txt` file. This is due to change soon.

## 3rd Party
See `LICENSE-3RD-PARTY` for information on 3rd-party tools and their associated licenses.
