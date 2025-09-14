/// \brief Internal logging methods
#pragma once

namespace MVRender {
    // Sets the internal logging message when an error occurs so the user
    // can grab it with
    void set_error_message(std::string&& msg);
}