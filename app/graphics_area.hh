#pragma once

#include "message_box.hh"
#include <GL/glew.h>

namespace bzzt {

struct graphics_area {
    using graphics_handle = unsigned int;

    bool init(message_box& msg_box);
    void deinit();

    void render();

    graphics_handle add_text    (const char* text, unsigned int length, float x, float y, float char_width, float char_height);
    void            delete_text (graphics_handle handle);

private:
    struct impl;
    impl* internal;
};

}