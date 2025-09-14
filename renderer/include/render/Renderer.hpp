/// \brief Singleton renderer that contains all necessary state
#pragma once
#include "render/Structs.h"

namespace Internal_Render {
    // Internal renderer state
    class Renderer {
    private:
        Renderer() {}

    public:
        // Singleton pattern - the class is destroyed at program end
        static Renderer& instance() {
            static Renderer r;
            return r;
        }

        Renderer(Renderer const&)       = delete;
        void operator=(Renderer const&) = delete;

        // Internal rendering methods
        void m_initialize_vulkan(MVR_InitializeParams& params);
        void m_quit_vulkan();
    };
}