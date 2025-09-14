/// \brief Internal logging methods
#pragma once
#include <iostream>
#include "render/Core.h"

namespace MVRender {
    // Sets the internal logging message when an error occurs so the user
    // can grab it with
    void set_error_message(const std::string& msg);

    // Wrapper for MVR_Result for more C++y error handling, automatically calls set_error_message
    // Please throw this class as the exception so we have the result automatically saved and also
    // an error message visible to the C api right away.
    class Exception : public std::exception {
    public:
        explicit Exception(MVR_Result result, const std::string& msg) : result_(result) {
            MVRender::set_error_message(msg);
        }

        [[nodiscard]] MVR_Result result() const noexcept { return result_; }

    private:
        MVR_Result result_;
    };
}