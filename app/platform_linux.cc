#include "startup_parameters.hh"
#include "window.hh"
#include "audio_process.hh"
#include "storage.hh"
#include <memory>
#include "message_box.hh"
#include <iostream>

int main(int argc, char** argv) {
    bzzt::consume_command_line_arguments(argc, argv);

    auto audio_process {std::make_unique<bzzt::audio_process>()};

    auto pipeline_config_filename {bzzt::get_pipeline_configuration_filename()};
    if (pipeline_config_filename.size() > 0) {
        bzzt::message_box msg_box {};
        audio_process = bzzt::load_pipeline_from_file(std::move(audio_process), pipeline_config_filename, msg_box);
        if (msg_box.length() > 0) {
            std::cout << "There was " << msg_box.length() << " error" << (msg_box.length() > 1 ? "s" : "") << " when trying to load the pipeline config file " << pipeline_config_filename << ":" << std::endl;
            while (msg_box.length() > 0) {
                std::cout << "  " << msg_box.pop() << std::endl;
            }
        }
    }

    auto window {bzzt::window{}};

    while (!window.should_close()) {
        // glClear(GL_COLOR_BUFFER_BIT);
        window.update_screen();
        window.handle_incoming_events();
    }

    return 0;
}