#include "startup_parameters.hh"
#include "window.hh"
#include "audio_process.hh"
#include "storage.hh"
#include <memory>
#include "message_box.hh"
#include "graphics_area.hh"
#include <iostream>

const unsigned int WINDOW_WIDTH {640};
const unsigned int WINDOW_HEIGHT {480};

void debug_console_out(bzzt::message_box& msg_box, std::string const& header) {
    if (msg_box.length() > 0) {
        std::cout << header << std::endl;
        while (msg_box.length() > 0) {
            std::cout << "  " << msg_box.pop() << std::endl;
        }
    }
}

void debug_graphics_out(bzzt::message_box& msg_box, std::string const& header, bzzt::graphics_area& graphics_area) {
    if (msg_box.length() > 0) {
        const float CHAR_WIDTH {16.0f / static_cast<float>(WINDOW_WIDTH)};
        const float CHAR_HEIGHT {16.0f / static_cast<float>(WINDOW_HEIGHT)};
        float line_y {1.0f};
        graphics_area.add_text(header.c_str(), header.size(), -1.0f, line_y, CHAR_WIDTH, CHAR_HEIGHT);
        while (msg_box.length() > 0) {
            line_y -= CHAR_HEIGHT;
            auto msg {msg_box.pop()};
            graphics_area.add_text(msg.c_str(), msg.size(), -1.0f, line_y, CHAR_WIDTH, CHAR_HEIGHT);
        }
    }
}

int main(int argc, char** argv) {
    bzzt::consume_command_line_arguments(argc, argv);
    bzzt::message_box msg_box {};

    bzzt::window window {WINDOW_WIDTH, WINDOW_HEIGHT};

    bzzt::graphics_area graphics_area {};
    auto graphics_inited {graphics_area.init(msg_box)};
    auto header {"There was " + std::to_string(msg_box.length()) + " error" + (msg_box.length() > 1 ? "s" : "") + " when initializing the graphics area:"};
    debug_console_out(msg_box, header);

    auto audio_process {std::make_unique<bzzt::audio_process>()};

    auto pipeline_config_filename {bzzt::get_pipeline_configuration_filename()};
    if (pipeline_config_filename.size() > 0) {
        audio_process = bzzt::load_pipeline_from_file(std::move(audio_process), pipeline_config_filename, msg_box);
        auto header {"There was " + std::to_string(msg_box.length()) + " error" + (msg_box.length() > 1 ? "s" : "") + " when trying to load the pipeline config file " + pipeline_config_filename + ":"};
        if (graphics_inited) {
            debug_graphics_out(msg_box, header, graphics_area);
        } else {
            debug_console_out(msg_box, header);
        }
    }

    while (!window.should_close()) {
        if (graphics_inited) {
            graphics_area.render();
        }
        window.update_screen();
        window.handle_incoming_events();
    }

    graphics_area.deinit();
    return 0;
}