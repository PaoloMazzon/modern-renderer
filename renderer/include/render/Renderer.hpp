/// \brief Singleton renderer that contains all necessary state
#pragma once

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
    };
}